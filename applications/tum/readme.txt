This is copy of email, where brief how-to is described.

First of all, I recommend to install software as described here:

  http://dabc.gsi.de/doc/dabc2/hadaq_trb3_package.html

In such case you will be able to reproduce all steps I doing.
Do not forget ". /your/path/to/trb3login" to initialize shell variables.

================ RAW data printout =========================

I check your files with hldprint and found one TRB with ID 0x8000 and four
TDCs with IDS 1000, 1001, 1002, 1003. This you could see when doing:

[shell] hldprint cc15239151526.hld -sub

Here I could see all subsubevents like:
*** Event #0x000009 fullid=0x2001 runid=0x0e58816e size 232 ***
   *** Subevent size  196+4 decoding 0x020011 id 0x8000 trig 0x572d9894 swapped align 4 ***
      *** Subsubevent size  16 id 0x1000 full 00101000
      *** Subsubevent size   3 id 0x1001 full 00031001
      *** Subsubevent size   3 id 0x1002 full 00031002
      *** Subsubevent size   3 id 0x1003 full 00031003
      *** Subsubevent size  13 id 0x8000 full 000d8000
      *** Subsubevent size   1 id 0x5555 full 00015555

And to see how data looks like:

[shell] hldprint cc15239151526.hld -sub -tdc 0x100F

*** Event #0x000009 fullid=0x2001 runid=0x0e58816e size 232 ***
   *** Subevent size  196+4 decoding 0x020011 id 0x8000 trig 0x572d9894 swapped align 4 ***
      *** Subsubevent size  16 id 0x1000 full 00101000
         [ 1] 21940000  tdc header
         [ 2] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [ 3] 80141ae9  hit  ch: 0 isrising:1 tc:0x2e9 tf:0x141 tm:12653735561.848 ns
         [ 4] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [ 5] 82c93b7c  hit  ch:11 isrising:1 tc:0x37c tf:0x093 tm:736.891 ns
         [ 6] 82d7ab80  hit  ch:11 isrising:1 tc:0x380 tf:0x17a tm:754.380 ns
         [ 7] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [ 8] 8313db7d  hit  ch:12 isrising:1 tc:0x37d tf:0x13d tm:740.043 ns
         [ 9] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [10] 8350ab7b  hit  ch:13 isrising:1 tc:0x37b tf:0x10a tm:730.598 ns
         [11] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [12] 838e8b7d  hit  ch:14 isrising:1 tc:0x37d tf:0x0e8 tm:740.967 ns
         [13] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [14] 83c6fb7c  hit  ch:15 isrising:1 tc:0x37c tf:0x06f tm:737.283 ns
         [15] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [16] 8402eb7d  hit  ch:16 isrising:1 tc:0x37d tf:0x02e tm:742.989 ns
      *** Subsubevent size   3 id 0x1001 full 00031001
         [18] 21940000  tdc header
         [19] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [20] 80092ae9  hit  ch: 0 isrising:1 tc:0x2e9 tf:0x092 tm:12653735563.750 ns
      *** Subsubevent size   3 id 0x1002 full 00031002
         [22] 21940000  tdc header
         [23] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [24] 800c8ae9  hit  ch: 0 isrising:1 tc:0x2e9 tf:0x0c8 tm:12653735563.163 ns
      *** Subsubevent size   3 id 0x1003 full 00031003
         [26] 21940000  tdc header
         [27] 6232db04  epoch 36887300 tm 12653731840.000 ns
         [28] 8012eae9  hit  ch: 0 isrising:1 tc:0x2e9 tf:0x12e tm:12653735562.054 ns
      *** Subsubevent size  13 id 0x8000 full 000d8000
           [30] 00067859  [31] 5cfb65c8  [32] 00000000  [33] 664daa1c  [34] 00000000  [35] 00000000  [36] 00000000  [37]
00000000
           [38] 00000000  [39] 5cfb65c8  [40] 00000000  [41] 664daa1c  [42] 00000000
      *** Subsubevent size   1 id 0x5555 full 00015555
           [44] 00000001


Actually, only TDC 0x1000 has any data, all other not.


================ TDC calibration =========================

Next action - produce TDC calibration. I used data from cc15239151526.hld file.
For that you need macro like https://subversion.gsi.de/go4/app/stream/applications/tum/calibr/first.C.
Copy it in separate directory and call:

[shell] go4analysis -user cc15239151526.hld

It runs ~30 s and produce four files: run11000.cal,  run11001.cal, run11002.cal, run11003.cal.
These are files with calibrations for TDC channels. Again, I saw hits only in TDC 1000.


================ TDC unpacking =========================

Now you could run unpacker for any other files, including same file again.
Configuration for unpacker is in https://subversion.gsi.de/go4/app/stream/applications/tum/first.C.
Just copy it to some other directory, copy there run1*.cal files and run there

[shell] rm -f *.root
[shell] go4analysis -user cc15239151526.hld -asf tum.root

These will produce two ROOT files.
   "tum.root"  with different histograms, typically used in monitoring.
   "tree.root" with output tree

Name "tree.root" is specified in the first.C macro.

Content of the tum.root file you could see, using ROOT or go4 like:
[shell]  root -l tum.root
[shell]  go4 tum.root

Another alternative is to use JavaScript ROOT. I copy file on my web server:
   http://web-docs.gsi.de/~linev/js/dev/?file=../files/trb3/tum.root

For instance, all channels for TDC_1000:
   http://web-docs.gsi.de/~linev/js/dev/?file=../files/trb3/tum.root&item=Histograms/TDC_1000/TDC_1000_Channels;1


================ TTree reading =========================

To read data from tree.root, you should use macro:

   https://subversion.gsi.de/go4/app/stream/applications/tum/read.C

It is rather simple. You iterate over entries in TTree.
Each entry is vector of messages.
For each message you get time stamp (with all calibration applied)
and original TDC message, where only channel id is important.
All time stamps are relative to trigger time, which is stored with channel 0.

Here is definition of hadaq::TdcMessage:
   https://subversion.gsi.de/go4/app/stream/include/hadaq/TdcMessage.h


================ Doing user analysis in stream framework =========================

As I mentioned, you could access TDC data immediately at the moment when they created.
It is done with https://subversion.gsi.de/go4/app/stream/applications/tum/second.C macro.
It runs together with first.C when you start go4analysis.
Major advantages - you can use it online and you do not need any intermediate TTree for your analysis.
At the end you access same data. Here is example histogram (event multiplicity), accumulated with second.C

http://web-docs.gsi.de/~linev/js/dev/?file=../files/trb3/tum.root&item=Histograms/Second1/Second1_NumHits;1

I hope it is enough for beginning.
