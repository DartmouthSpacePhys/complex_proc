#! /bin/bash

RTD_DIR="/tmp/rtd"
RTDF="rtd.data"

OPREFIX="test"

if [ $1 ] && [ $1 -eq 1 ]; then CPRTD="cprtd_1ch"; else CPRTD="cprtd"; fi

if [ $2 ];
then if [ `expr match ${2} "f"` -eq 1 ]; then 
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    elif [ `expr match ${2} "t"` -eq 1 ]; then
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    elif [ `expr match ${2} "d"` -eq 1 ]; then
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    else echo "$0 [0|1 (use cprtd|cprtd_1ch)] <d|f|t (\"digitizer\"|\"fileplayer\"|\"tcp\")> "
	exit
    fi
fi

RTD_DIRF=${RTD_DIR}/${RTDF}

echo "Using ${CPRTD} for \"${OPREFIX}\" display with rtd file ${RTD_DIRF}..."

/usr/src/rtdgui/rtdgui /usr/src/rtdgui/hf2_config.input ${OPREFIX}&
/usr/src/complex_proc/${CPRTD} -f 0 -F 5000 -m ${RTD_DIRF} -o ${OPREFIX}
