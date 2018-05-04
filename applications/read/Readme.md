# Reading created ROOT trees

Stream analysis can create output ROOT TTree with separate branch for each processor.
Content of the branch depends on the processor - typically it is vector of some messages.

For TRB3-TDC analysis there are three different storage formats, configured with the command:

       base::ProcMgr::instance()->SetStoreKind(2);
 
 Values meaning:
    
     0 - disable store
     1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
     2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
     3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double

Also following configurations should be applied:

    // configure triggered analysis - enables any kind of output
    base::ProcMgr::instance()->SetTriggeredAnalysis(true);
    
    // create ROOT file store
    base::ProcMgr::instance()->CreateStore("td.root");

Folder contains several examples how different formats can be read: 

    read1.C - reading of hadaq::TdcMessageExt
    read2.C - reading of hadaq::MessageFloat
    
With the ROOT6 rootlogon.C should be configured properly. 
It configures include path to `$STREAMSYS/include`
     