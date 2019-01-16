#!/bin/bash

if [ -z "$1" ]; then
  DATAFILE="netmon.csv"
else
  DATAFILE="$1.csv"
fi

DATADIR=data
test -d $DATADIR || mkdir -p $DATADIR

OUTPUT="-o $DATADIR/$DATAFILE"
TIMEOPT="-c 1000 -d 1000000"
TRIGGER="-t 16000"
#COUNTERS="-n rx_bytes -n tx_bytes -n rx_packets -n tx_packets"
COUNTERS="-n rx_bytes -n tx_bytes"
NICS="enp1s0"
#NICS="enp1s0f0 enp1s0f1 enp1s0f2 enp1f0f3"
DBG="-D 0"
echo bin/netmon $DBG $TIMEOPT $TRIGGER $OUTPUT $COUNTERS $NICS
bin/netmon $DBG $TIMEOPT $TRIGGER $OUTPUT $COUNTERS $NICS

