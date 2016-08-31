#!/bin/bash

rm -f *.root
export CALMODE=-1; go4analysis -rate -user $1
rm -f *.root $2
export CALMODE=0; export BMASK=0x354; export BSHIFT=0; go4analysis -rate -user $1 -asf $2