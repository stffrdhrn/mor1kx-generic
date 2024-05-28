## mor1kx Generic

This repo provides a basic CPU test harness for the [mor1kx](https://github.com/openrisc/mor1kx)
OpenRISC CPU core.  It is intended to be used with [fusesoc](https://github.com/olofk/fusesoc).

This is mainly used for testing the functionality of the verilog core using a
suite link [or1k-tests](https://github.com/openrisc/or1k-tests).

### Getting Started

To run software on the `mor1kx-generic` soc we will:

 - Install Fusesoc
 - Add required cores and `mor1kx-generic` to your fusesoc library
 - Installing backends
   - iverilog
   - verilator
   - sim
 - Install an OpenRISC compiler
 - Compile a program
 - Run the program

#### Installing Fusesoc

Fusesoc is a build and dependency management tool for verilog cores.
It is written in python so it can be installed using `pip` as below:

Current version: [fusesoc/2.3](https://pypi.org/project/fusesoc/)

```
pip install fusesoc
```

To verify the installation try:

```
fusesoc --version

# Example output:
# 2.3
```

#### Adding IP cores to your fusesoc library

Once we have fusesoc installed we need to configure a workspace and
add cores into the workspace.
Below we create a fusesoc workspace directory and install a few cores.

Note: Once all of this is done you can look at `fusesoc.conf` to understand
better how fusesoc works.

```
mkdir -p /tmp/openrisc/src
cd /tmp/openrisc/src

git clone https://github.com/stffrdhrn/mor1kx-generic.git
git clone https://github.com/openrisc/or1k_marocchino.git
git clone https://github.com/openrisc/mor1kx.git

fusesoc library add fusesoc-cores https://github.com/fusesoc/fusesoc-cores
fusesoc library add intgen https://github.com/stffrdhrn/intgen.git
fusesoc library add elf-loader https://github.com/fusesoc/elf-loader.git
fusesoc library add mor1kx-generic /tmp/openrisc/src/mor1kx-generic
fusesoc library add or1k_marocchino /tmp/openrisc/src/or1k_marocchino
fusesoc library add or1k_marocchino /tmp/openrisc/src/mor1kx
```

To verify the installation we can run:

```
fusesoc core list

# Example output:
# Available cores:
#
# Core                                    Cache status  Description
# ================================================================================
# ::SD-card-controller:0-r2              :      empty : <No description>
# ::SD-card-controller:0-r3              :      empty : <No description>
# ::ac97:1.2-r1                          :      empty : OpenCores AC97 Controller core
# ::adv_debug_sys:3.1.0-r1               : downloaded : <No description>
# ::altera_virtual_jtag:1.0-r1           :      empty : Advanced Debug System wrapper for altera virtual jtag
# ...

fusesoc core show mor1kx-generic

# Example output:
# CORE INFO
# Name:        ::mor1kx-generic:1.1
# Description: Minimal mor1kx simulation environment
# Core root:   /tmp/openrisc/src/mor1kx-generic
# Core file:   mor1kx-generic.core
#
# Targets:
# marocchino_tb : <No description>
# mor1kx_tb     : <No description>
# sim           : <No description>
```

### Installing backends

The `mor1kx-generic` fusesoc core is setup to be allow running
verilog simulations using either:

 - [incarus verilog](https://steveicarus.github.io/iverilog/)
 - [verilator](https://www.veripool.org/verilator/)
 - [modelsim](https://en.wikipedia.org/wiki/ModelSim)

You will need to have your preferred backend installed to run the
simulation.  If you do not know which to choose use **icarus verilog** it
is the easiest to get running.

#### Installing Icarus Verilog

Follow the [installation guide](https://steveicarus.github.io/iverilog/usage/installation.html) on the Icarus website.
Using your distribution's binary is usually the easiest solution.

#### Installing Verilator

Follow the [installation manual](https://verilator.org/guide/latest/install.html)
in the verilator user's guide.
Using your distribution's binary is usually the easiest solution.

### Install an OpenRISC compiler

To compile C/C++ and assembly programs to run on our SoC we need a compiler.
[GCC](https://gcc.gnu.org) is a popular compiler that has an OpenRISC port.
A version of GCC setup to target OpenRISC may be available from your Linux distribution
or you can download the GCC sources and compiler it yourself.

The binaries provided at the [Embecosm compiler tool chain downloads](https://www.embecosm.com/resources/tool-chain-downloads/) page
are fit for our purpose so we will use those.

To install the toolchain we can run:

```
cd /tmp/openrisc
curl https://buildbot.embecosm.com/job/or1k-gcc-centos7-release/18/artifact/or1k-embecosm-centos7-gcc13.2.0.tar.gz -o or1k-embecosm-centos7-gcc13.2.0.tar.gz
tar -xf or1k-embecosm-centos7-gcc13.2.0.tar.gz

export PATH=$PATH:/tmp/openrisc/or1k-embecosm-centos7-gcc13.2.0/bin
```

To verify the compiler run:

```
or1k-elf-gcc --version

# Example output:
# or1k-elf-gcc ('or1k-embecosm-centos7-gcc13.2.0') 13.2.0
# Copyright (C) 2023 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

### Compile a Program

The below program is from Stafford's blog entry [Marocchino in Action](http://stffrdhrn.github.io/hardware/embedded/openrisc/2019/06/11/or1k_marocchino.html)
it explains in more detail how this works.

Create a source file `asm-openrisc.s` as follows:

```asm
/* Exception vectors section.  */
	.section .vectors, "ax"

/* 0x100: OpenRISC RESET reset vector. */
        .org 0x100

	/* Jump to program initialisation code */
	.global _main
	l.movhi r4, hi(_main)
	l.ori r4, r4, lo(_main)
	l.jr    r4
	 l.nop

/* Main executable section.  */
	.section .text

	.global _main
_main:
	l.movhi	r0,0x0
	l.addi	r1,r0,0x1
	l.addi	r2,r1,0x2
	l.addi	r3,r2,0x4
	l.addi	r4,r3,0x8
	l.addi	r5,r4,0x10
	l.addi	r6,r5,0x20
	l.addi	r7,r6,0x40
	l.addi	r8,r7,0x80
	l.addi	r9,r8,0x100
	l.addi	r10,r9,0x200
	l.addi	r11,r10,0x400
	l.addi	r12,r11,0x800
	l.addi	r13,r12,0x1000
	l.addi	r14,r13,0x2000
	l.addi	r15,r14,0x4000
	l.addi	r16,r15,0x8000

	l.sub	r31,r0,r1
	l.sub	r30,r31,r2
	l.sub	r29,r30,r3
	l.sub	r28,r29,r4
	l.sub	r27,r28,r5
	l.sub	r26,r27,r6
	l.sub	r25,r26,r7
	l.sub	r24,r25,r8
	l.sub	r23,r24,r9
	l.sub	r22,r23,r10
	l.sub	r21,r22,r11
	l.sub	r20,r21,r12
	l.sub	r19,r20,r13
	l.sub	r18,r19,r14
	l.sub	r17,r18,r15
	l.sub	r16,r17,r16

	/* Set sim return code to 0 - meaning OK. */
	l.movhi	r3, 0x0
	l.nop 	0x1 /* Exit simulation */
	l.nop
```

Create and compile the source file as follows.

```
cd /tmp/openrisc/src
vim openrisc-asm.s

or1k-elf-gcc -nostartfiles openrisc-asm.s -o openrisc-asm
```

### Run the program

To run a program we have a few different options for example:

**Running the Marocchino Core with Icatus backend**

```
fusesoc run --target marocchino_tb --tool icarus mor1kx-generic \
  --elf_load ./openrisc-asm --trace_enable --trace_to_screen --vcd
```

**Running the Mork1x Core with Icatus backend**

```
fusesoc run --target mor1kx_tb --tool icarus mor1kx-generic \
  --elf_load ./openrisc-asm --trace_enable --trace_to_screen --vcd
```

**Running the Mork1x Core with verilator backend**

```
fusesoc run --target mor1kx_tb --tool varilator mor1kx-generic \
  --elf_load ./openrisc-asm --trace_enable --trace_to_screen --vcd testbench.vcd
```

All combinations should work.  Please report any issues via github issues.



