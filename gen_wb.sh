#!/bin/sh

WB_INTERCON_URL=https://raw.githubusercontent.com/openrisc/orpsoc-cores/master/cores/wb_intercon

download() {
  local url=$1
  local filename=`basename $url`

  curl $url --output $filename
}


pushd $FILES_ROOT
  download ${WB_INTERCON_URL}/sw/wb_intercon_gen
  download ${WB_INTERCON_URL}/sw/verilogwriter.py

  chmod 755 wb_intercon_gen

  ./wb_intercon_gen ./data/wb_intercon.conf ./rtl/verilog/wb_intercon.v

  rm wb_intercon_gen verilogwriter.py verilogwriter.pyc
popd
