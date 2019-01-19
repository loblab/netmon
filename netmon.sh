#!/bin/bash
set -e

if [ -z "$1" ]; then
  DATAFILE="netmon.csv"
else
  DATAFILE="$1.csv"
fi

DATADIR=data
test -d $DATADIR || mkdir -p $DATADIR

OUTPUT="-o $DATADIR/$DATAFILE"
TIMEOPT="-c 500 -d 700000"
TRIGGER="-t 8000 -f min"
#COUNTERS="-n rx_bytes -n tx_bytes -n rx_packets -n tx_packets"
#COUNTERS="-n rx_bytes -n tx_bytes"
COUNTERS="-n rx_bytes"
#NICS="enp1s0"
NICS="enp1s0f0 enp1s0f1 enp1s0f2 enp1s0f3"
DBG="-D 0"
echo bin/netmon $DBG $TIMEOPT $TRIGGER $OUTPUT $COUNTERS $NICS
bin/netmon $DBG $TIMEOPT $TRIGGER $OUTPUT $COUNTERS $NICS

