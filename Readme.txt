This is new analysis framework, which should be able to work with many
parallel data streams. 

Main ideas behind:
 1) Event loop decoupled from the source - number of produced output entities 
    does not depend from input
 2) One could have arbitrary number of input sources 
 3) At the same time one could work with arbitrary number of input buffers and
    arbitrary number of output events, no any N->1 or 1->1 or 1->N limitations.
 4) Each data source is a stream of messages, stamped with the time or with trigger number
 5) Streams can be synchronized either by time, by special sync markers, or by trigger number
 6) One always scan each stream at least once for raw analysis, statistics, indexing
 7) By first scan different kinds of reference markers should be collected
 8) Base on such markers central unit (manager) decides how different streams 
    should be synchronized
 9) Manager also collects and decides about region-of-interests - 
    where to collect data for next steps of the analysis
10) Selection of data for each region is performed by second scan of the data,
    data is discarded immediately afterwards 
11) Produced sub-data (SubEvent) combined together and delivered as independent 
    entities to the next analysis steps
12) Data buffers are stored in queues until they are scanned, 
    number of preserved buffers is independent for each stream
13) Triggered source (like MBS or SPADIC 0.3 or EPICS) will be treated 
    like data assigned directly with reference marker - trigger. 
    Time for such marker will be derived from other branches like ROC


Some implementation details and ideas:
1) No any external dependencies - no any Go4, DABC or ROOT
2) Simple go4 wrapper allows to run code in the go4 environment and 
   use histograms and conditions for visualization
3) Pure ROOT wrapper can (will) be implemented as well
4) All classes provided with ROOT dictionaries and therefore can be used in ROOT scripts
5) Configuration of particular analysis setup is done via ROOT script, 
   same setup script planed to be used for all kinds of wrappers
6) base::Buffer class provides simplified functionality of dabc::Buffer class
   Same raw buffer can have many references and automatically destroyed when
   last reference is removed. Allows to use STL collections for base::Buffer class  
7) Code will be designed to be able run separate streams in different threads 
   with only minimal synchronization
8) Way how current beamtime/test setups software organized should be preserved.
   Means new framework will only replace first step with standard raw analysis,
   all consequent steps like detector mapping, efficiency calculations, tracking
   should look very-very similar. 


Current state:
1) Preliminary working for ROC/nXYTER and ROC/GET4 combination
2) First try for nXYTER problem correction
2) Capable to correctly resolve multiple AUX problem!
3) Runs with lmd files from beamtimes. 
   See setup.C macros in applications directory


To be done in near future:
1) Precise time calibration, was missing in onlinemonitor
   For a moment linear interpolation between two syncs are used
2) Region-of-interests
3) No-trigger mode, when synchronized data delivered to next step
4) GET4, TRB3, Spadic, MBS support
5) I/O for event data


How to use:
1) checkout repository with command
  svn co https://subversion.gsi.de/go4/app/stream stream
2) If required, configure ROOT and Go4
3) Compile project
   make all
4) Use generated streamlogin script to set variables
5) Run any example from applications subdirectory with go4
    go4analysis -file your_file_name.lmd
    