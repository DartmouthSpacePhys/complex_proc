#! /bin/bash

if [ $1 ]; then CPRTD="cprtd"; else CPRTD="cprtd_1ch"; fi
echo "Using ${CPRTD}..."

#/daq/rtdgui/rtdgui -g 640x480+0+0 /daq/rtdgui/hf2_config.input&
/usr/src/rtdgui/rtdgui /usr/src/rtdgui/hf2_config.input&
/usr/src/complex_proc/${CPRTD} -E -f 0 -F 5000 -m /tmp/rtd/rtd_tcpl.data
