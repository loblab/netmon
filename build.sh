#!/bin/bash

OUTNAME=netmon
OUTDIR=bin

test -d $OUTDIR || mkdir $OUTDIR
g++ -std=c++11 netmon.cpp -o $OUTDIR/$OUTNAME
$OUTDIR/$OUTNAME -h

