#! /bin/bash

#plot '/tmp/rtd/rtd.data' binary skip = 72 format="%int16%int16" using 2 every ::100:0:200:0 with lines, '/tmp/rtd/rtd.data' binary skip = 72 format="%int16%int16" using 1 every ::100:0:200:0 with lines

IORQ=0 # I=0 Q=1

OPREFIX="test"

RTD_DIR="/tmp/rtd"
RTDF="rtd_tcpl.data"

if [ $1 ] 
then if [ $1 -le 1 ]
    then IORQ=${1} 
    else echo "$0 <NUM CHANS (MAX 4)> <d|f|t (\"digitizer\"|\"fileplayer\"|\"tcp\")> "
	exit
    fi
fi

if [ $2 ];
then if [ `expr match ${2} "f"` -eq 1 ]; then 
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    elif [ `expr match ${2} "t"` -eq 1 ]; then
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    elif [ `expr match ${2} "d"` -eq 1 ]; then
	RTDF="rtd_${2}.data"; OPREFIX=${2}
    else echo "$0 <NUM CHANS (MAX 4)> <d|f|t (\"digitizer\"|\"fileplayer\"|\"tcp\")> "
	exit
    fi
fi

RTDFILE=${RTD_DIR}/${RTDF}

BYTESKIP=72
DEFCHANFORMAT="%int16%int16"
CHANFORMAT=${DEFCHANFORMAT}

OFFSET=000
NUMPOINTS=200

#WITHLINES=""
WITHLINES="with lines"

PLOTSTR_I="'${RTDFILE}' binary skip=${BYTESKIP} format=\"${CHANFORMAT}\" using 1 every ::${OFFSET}:0:$((OFFSET + NUMPOINTS)):0 ${WITHLINES} title \"I's\""

PLOTSTR_Q="'${RTDFILE}' binary skip=${BYTESKIP} format=\"${CHANFORMAT}\" using 2 every ::${OFFSET}:0:$((OFFSET + NUMPOINTS)):0 ${WITHLINES} title \"Q's\""

PLOTSTR="plot ${PLOTSTR_I}, ${PLOTSTR_Q}"

#echo ${PLOTSTR}

echo "Gnuplotting ${RTDFILE}.."

#gnuplot -persist -e "${GNUPLOT_OPTSTR}" -e "${PLOTSTR}" rtd_loop.gnu
gnuplot -persist plot_opts.gnu rtd_multiplot_loop.gnu
