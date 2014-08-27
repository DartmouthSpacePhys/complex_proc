#! /bin/bash

#File to plot
RTDFILE="/tmp/rtd/rtd_fileplayer.data"

#File format stuff
DEFIQFORMAT="%int16"
IQFORMAT=${DEFIQFORMAT}

#Plot stuff
IORQ=${1:-0}
BYTESKIP=200
OFFSET=0
NUMPOINTS=10000
#WITHLINES=""
WITHLINES="with lines"


for((i = 1; i < 2; i++))
do IQFORMAT+=${DEFIQFORMAT}
done


#-e "set terminal wxt"

if [ $1 ];
then if [ $IORQ -le 1 ];
    then gnuplot -persist -e "plot '/tmp/rtd/rtd_fileplayer.data' binary skip=${BYTESKIP} format=\"${IQFORMAT}\" using $((1 + ${IORQ})) every ::${OFFSET}:0:$((OFFSET + NUMPOINTS)):0 ${WITHLINES} title \"Channel ${IORQ}\"" rtd_loop.gnu;
    elif [ $IORQ -eq 2 ];
    then gnuplot -persist -e "plot '/tmp/rtd/rtd_fileplayer.data' binary skip=${BYTESKIP} format=\"${IQFORMAT}\" using 1 every ::${OFFSET}:0:$((OFFSET + NUMPOINTS)):0 ${WITHLINES} title \"I's\", '/tmp/rtd/rtd_fileplayer.data' binary skip=${BYTESKIP} format=\"${IQFORMAT}\" using 2 every ::${OFFSET}:0:$((OFFSET + NUMPOINTS)):0 ${WITHLINES} title \"Q's\"" rtd_loop.gnu;
    else
	echo "$0 [0 for I's|1 for Q's]";
    fi
fi

#Example command for bbe, defining block sizes
#bbe -s -b "/aDtromtu/:131072" -e "J 1; D" stripped.data > bbeout.data 
