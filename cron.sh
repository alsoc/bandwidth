#! /bin/sh


MONOCORE_LOG=1c.log
MONOSOCKET_LOG=1s.log
FULL_LOG=2s.log

{ echo -n "[$(date)] " ; numactl -N 0 -m 0 -C 0 ./bandwidth -s 1GiB -c 1 | head -2 | tail -1 ; } >> $MONOCORE_LOG
{ echo -n "[$(date)] " ; numactl -N 0 -m 0 ./bandwidth -s 4GiB -c 1 | head -2 | tail -1 ; } >> $MONOSOCKET_LOG
{ echo -n "[$(date)] " ; ./bandwidth -s 8GiB -c 1 | head -2 | tail -1 ; } >> $FULL_LOG
