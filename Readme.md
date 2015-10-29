# Stream analysis framework

This is new analysis framework, which should be able to work with many
parallel data streams.

## Main ideas behind

1. Event loop decoupled from the source - number of produced output entities 
    does not depend from input
2. One could have arbitrary number of input sources 
3. At the same time one could work with arbitrary number of input buffers and
    arbitrary number of output events, no any N->1 or 1->1 or 1->N limitations.
4. Each data source is a stream of messages, stamped with the time or with trigger number
5. Streams can be synchronized either by time, by special sync markers, or by trigger number
6. One always scan each stream at least once for raw analysis, statistics, indexing
7. By first scan different kinds of reference markers should be collected
8. Base on such markers central unit (manager) decides how different streams 
    should be synchronized
9. Manager also collects and decides about region-of-interests - 
    where to collect data for next steps of the analysis
10. Selection of data for each region is performed by second scan of the data,
    data is discarded immediately afterwards 
11. Produced sub-data (SubEvent) combined together and delivered as independent 
    entities to the next analysis steps
12. Data buffers are stored in queues until they are scanned, 
    number of preserved buffers is independent for each stream
13. Triggered source (like MBS or SPADIC 0.3 or EPICS) will be treated
    like data assigned directly with reference marker - trigger.
    Time for such marker will be derived from other branches like ROC


## Some implementation details and ideas

1. No any external dependencies - no any Go4, DABC or ROOT
2. Simple go4 run engine allows to run code in the go4 environment and
   use histograms and conditions for visualization, pure ROOT wrapper also exists
3. There is DABC run engine, which allows to run code without ROOT at all,
   producing ROOT histograms at the end. If possible, analysis can run in
   parallel in many threads.
4. All classes provided with ROOT dictionaries and therefore can be used in ROOT scripts
5. Configuration of particular analysis setup is done via ROOT-like scripts, 
   same setup script can be used for all kinds of run engines (go4/dabc/root)
6. base::Buffer class provides simplified functionality of dabc::Buffer class
   Same raw buffer can have many references and automatically destroyed when
   last reference is removed. Allows to use STL collections for base::Buffer class
7. Code will be designed to be able run separate streams in different threads 
   with only minimal synchronization
8. Way how current beamtime/test setups software organized should be preserved.
   Means new framework will only replace first step with standard raw analysis,
   all consequent steps like detector mapping, efficiency calculations, tracking
   should look very-very similar. 


## Current state
1. First working implementation for: 
   ROC/nXYTER, ROC/GET4, TRB3/TDC, TRB3/ADC, MBS/GET4
2. Working correction for nXYTER last-epoch problem
3. Capable to correctly work with AUX-based trigger
4. Failure detection in TRB3 readout
5. Correct synchronization of streams with different number
   or with missing SYNC messages
6. Also no-SYNC mode works - in optic case sync messages can be 
   skipped completely from analysis     
7. Preliminary code for GET4 testing is implemented.
   It allows to recognize different problems from measured pulser data  
8. Files from cern-oct12 (CERN/PS) and cern-gem12 (CERN/SPS) 
   beamtimes can be processed (at least primary data selection).
   For GEM detector-mapping (second step) is implemented.
9. Solid time model. Universal time unit is seconds.
   Each subsystem able to provide continuous time scale without
   any overflows, which are typical problem of simple binary counters.
10. ROOT TTree as storage for event data


## To be done in near future
1. Precise time calibration, was missing completely in onlinemonitor.
   For a moment simple linear interpolation between two syncs are used.
   Now, when all local times are continuous, it is possible to implement
   calibration coefficients with smoothing instead of simple interpolations.  
2. More complex rules for Region-of-Interests (RoI) definitions.
   One could use correlation between several channels for that.
3. Regular time intervals - model of time slices. In such case
   all data (with some duplication) should be delivered to next step
 

## How to use package

1. Checkout repository with command

    [shell] svn co https://subversion.gsi.de/go4/app/stream stream
  
2. If required, configure ROOT and Go4 shell variables.
   Typically one should call go4login initialization script
   
    [shell] . go4login 
   
   Project can be compiled without ROOT and Go4, there is run engine
   provided with DABC framework
    
3. Compile project:

    [shell] cd stream
    [shell] make all
   
4. Use generated streamlogin script to set variables:

    [shell] . streamlogin

5. Run any example from applications sub-directory with go4.
   If necessary, compile additional code there:

    [shell] cd applications/get4test
    [shell] make all
    [shell] go4analysis -file your_file_name.lmd

6. Results histogram can be seen from autosave file

    [shell] go4 Go4AnalysisASF.root    
   

For any questions or suggestions contact:
S.Linev (at) gsi.de

