set terminal wxt size 800,600
set multiplot layout 2,1

#For rtd_fileplayer data, set endian=big!
#For slave, set skip =
#sbytes=160
sbytes=131282

plot '/tmp/rtd/rtd_tcpl.data' binary skip=sbytes endian=big format="%int16%int16" u 1 \
every ::000:0:400:0 with lines title "I's"
plot '/tmp/rtd/rtd_tcpl.data' binary skip=sbytes endian=big format="%int16%int16" u 2 \
every ::000:0:400:0 with lines title "Q's" ls 2
unset multiplot