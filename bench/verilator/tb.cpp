/*
 * mor1kx-generic system Verilator testbench
 *
 * Author: Olof Kindgren <olof.kindgren@gmail.com>
 * Author: Franck Jullien <franck.jullien@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <stdint.h>
#include <signal.h>
#include <argp.h>
#include <verilator_tb_utils.h>
#include <or1ksim_trace.h>

#include "Vorpsoc_top__Syms.h"

static bool done;

#define NOP_NOP			0x0000      /* Normal nop instruction */
#define NOP_EXIT		0x0001      /* End of simulation */
#define NOP_REPORT		0x0002      /* Simple report */
#define NOP_PUTC		0x0004      /* Simputc instruction */
#define NOP_CNT_RESET		0x0005      /* Reset statistics counters */
#define NOP_GET_TICKS		0x0006      /* Get # ticks running */
#define NOP_GET_PS		0x0007      /* Get picosecs/cycle */
#define NOP_TRACE_ON		0x0008      /* Turn on tracing */
#define NOP_TRACE_OFF		0x0009      /* Turn off tracing */
#define NOP_RANDOM		0x000a      /* Return 4 random bytes */
#define NOP_OR1KSIM		0x000b      /* Return non-zero if this is Or1ksim */
#define NOP_EXIT_SILENT		0x000c      /* End of simulation, quiet version */

#define SPR_SR_SM          0x00000001  /* Supervisor Mode */
#define SPR_SR_F           0x00000200  /* Condition Flag */

#define RESET_TIME		2

vluint64_t main_time = 0;       // Current simulation time
// This is a 64-bit integer to reduce wrap over issues and
// allow modulus.  You can also use a double, if you wish.

double sc_time_stamp () {       // Called by $time in Verilog
  return main_time;           // converts to double, to match
  // what SystemC does
}

void INThandler(int signal)
{
	printf("\nCaught ctrl-c\n");
	done = true;
}

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case ARGP_KEY_INIT:
		state->child_inputs[0] = state->input;
		break;
	// Add parsing of custom options here
	}

	return 0;
}

static int parse_args(int argc, char **argv, VerilatorTbUtils* tbUtils)
{
	struct argp_option options[] = {
		// Add custom options here
		{ 0 }
	};
	struct argp_child child_parsers[] = {
		{ &verilator_tb_utils_argp, 0, "", 0 },
		{ &or1ksim_trace_argp, 0, "", 0 },
		{ 0 }
	};
	struct argp argp = { options, parse_opt, 0, 0, child_parsers };

	return argp_parse(&argp, argc, argv, 0, 0, tbUtils);
}

static uint32_t get_gpr(Vorpsoc_top* top, uint32_t n)
{
#ifdef MAROCCHINO
	return top->orpsoc_top->gencpu__DOT__or1k_marocchino0->u_cpu->get_gpr(n);
#else
	return top->orpsoc_top->gencpu__DOT__mor1kx0->mor1kx_cpu->monitor__DOT__get_gpr(n);
#endif
}

static void print_trace(Vorpsoc_top* top, uint32_t sr, uint32_t pc, uint32_t insn)
{
	char buf[50] = "";

	if (or1ksim_trace_enable ()) {
		or1ksim_trace (buf, sizeof(buf), insn);

		int trace_dest_reg = or1ksim_trace_dest_reg_get();
		int trace_src_reg = or1ksim_trace_src_reg_get();
		int trace_store_addr_reg = or1ksim_trace_store_addr_reg_get();
		unsigned int trace_store_imm = or1ksim_trace_store_imm_get();
		int trace_store_val_reg = or1ksim_trace_store_val_reg_get();
		int trace_store_width = or1ksim_trace_store_width_get();
		int trace_dest_spr = or1ksim_trace_dest_spr_get();

		printf ("%c ", (sr & SPR_SR_SM) ? 'S' : 'U');

		printf("%08x: %08x %-24s", pc, insn, buf);

		/* Put either the register assignment, SPR value, or store */
		if (-1 != trace_dest_spr) {
			printf("SPR[%04x]  = %08x",
			       trace_dest_spr | get_gpr(top, trace_dest_reg),
			       get_gpr(top, trace_src_reg));
		} else if (-1 != trace_dest_reg) {
			printf("r%-2u        = %08x",
			       trace_dest_reg,
			       get_gpr(top, trace_dest_reg));
		} else {
			uint32_t store_val = 0;
			uint32_t store_addr = 0;

			if (0 != trace_store_width) {
				store_val = get_gpr(top, trace_store_val_reg);
				store_addr = get_gpr(top, trace_store_addr_reg) +
					     trace_store_imm;
			}

			switch (trace_store_width) {
			case 1:
				printf("[%08x] = %02x      ", store_addr,
				       store_val);
				break;
			case 2:
				printf("[%08x] = %04x    ", store_addr,
				       store_val);
				break;
			case 4:
				printf("[%08x] = %08x", store_addr,
				       store_val);
				break;
			default:
				printf("                     ");
				break;
			}
		}

		/* Print the flag */
		printf("  flag: %u\n", sr & SPR_SR_F ? 1 : 0);
	}
}

int main(int argc, char **argv, char **env)
{
	uint32_t insn = 0;
	uint32_t adv = 0;
	uint32_t sr = 0;
	uint32_t ex_pc = 0;

	Verilated::commandArgs(argc, argv);

	Vorpsoc_top* top = new Vorpsoc_top;
	VerilatorTbUtils* tbUtils =
		new VerilatorTbUtils(top->orpsoc_top->wb_bfm_memory0->ram0->mem.data());

	parse_args(argc, argv, tbUtils);

	signal(SIGINT, INThandler);

	top->wb_clk_i = 0;
	top->wb_rst_i = 1;

	top->trace(tbUtils->tfp, 99);

	while (tbUtils->doCycle() && !done) {
		if (tbUtils->getTime() > RESET_TIME)
			top->wb_rst_i = 0;

		top->eval();

		top->wb_clk_i = !top->wb_clk_i;

		tbUtils->doJTAG(&top->tms_pad_i, &top->tdi_pad_i, &top->tck_pad_i, top->tdo_pad_o);

#ifdef MAROCCHINO
		insn = top->orpsoc_top->gencpu__DOT__or1k_marocchino0->u_cpu->monitor_execute_insn;
		adv = top->orpsoc_top->gencpu__DOT__or1k_marocchino0->u_cpu->monitor_execute_advance;
		sr = top->orpsoc_top->gencpu__DOT__or1k_marocchino0->u_cpu->monitor_spr_sr;
		ex_pc = top->orpsoc_top->gencpu__DOT__or1k_marocchino0->u_cpu->monitor_execute_pc;
#else
		insn = top->orpsoc_top->gencpu__DOT__mor1kx0->mor1kx_cpu->monitor_execute_insn;
		adv = top->orpsoc_top->gencpu__DOT__mor1kx0->mor1kx_cpu->monitor_execute_advance;
		sr = top->orpsoc_top->gencpu__DOT__mor1kx0->mor1kx_cpu->monitor_spr_sr;
		ex_pc = top->orpsoc_top->gencpu__DOT__mor1kx0->mor1kx_cpu->monitor_execute_pc;
#endif
		if (adv && tbUtils->clkPosEdge(top->wb_clk_i)) {
			/* Trace instruction to screen if enabled */
			print_trace(top, sr, ex_pc, insn);

			if (insn == (0x15000000 | NOP_PUTC)) {
				printf("%c", get_gpr(top, 3));
			} else if (insn == (0x15000000 | NOP_EXIT) || insn == (0x15000000 | NOP_EXIT_SILENT)) {
				printf("Success! Got NOP_EXIT. Exiting (%lu)\n",
				       tbUtils->getTime());
				done = true;
			}
		}
	}

	printf("Simulation ended at PC = %08x (%lu)\n",
	       ex_pc, tbUtils->getTime());

	delete tbUtils;
	exit(0);
}
