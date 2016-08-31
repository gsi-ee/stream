#!/bin/bash

tgt=res/scan.root
txt=res/scan.txt

# first generate normal calibration
#rm -f *.root
#export CALMODE=-1; go4analysis -rate -user $1

rm -f $txt
echo "Scan data" >> $txt

for ((k=2; k<=3; k++)); do

for ((i=-3; i<=3; i++)); do

   echo "Produce calibration maskid $k shift $i"
   
   export CALMODE=-1; export BMASK=$k; export BSHIFT=$i; go4analysis -rate -disable-asf -user $1  
   
   rm -f $tgt go4.log post.log
   
   echo "Process maskid $k shift $i"
   
   export CALMODE=0; export BMASK=$k; export BSHIFT=$i; go4analysis -user $1 -asf $tgt >> go4.log
   cat go4.log | grep "SHIFT MASK" >> $txt
     
   root -l post_chain.C -q >> post.log
   cat post.log | grep "Entries" >> $txt  
 
done
done
