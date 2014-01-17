This is code to analyze data from MAMI beamtime, 7-8 january 2014
There were several Padiwas, connected to TDC_8a00 and TDC_8a03.
One of the Padiwa was used for trigger signals, 
where trigger number was coded as delay between two channels.

Here one can found:

first.C - 
configuration of standard unpacker (hadaq::Trb3Processor) of TRB3/TDC data 
into vectors of hit messages. Here one just configure TDC ids, 
mode of TDC calibrations, some debug parameters. 
Idea to keep this functions very similar to functionality of TDC unpacker in hydra2

second.C - using unpacked data from TDCs, one produces Padiwa-specific data 
(PadiwaProc class) and also stores them in the tree. As last stage,
TestProc class produce correlation data between two Padiwas 
and also store this data in the tree. 

Main advantages of code in second.C:
 - runs in compiled mode (via ACLiC). 
 - full access to unpacked data;
 - one can build any kind of correlations in the user code;
 - one can create any kind of custom histograms;
 - one can store any kind of data in the TTree (also simple ntuple can be used).
