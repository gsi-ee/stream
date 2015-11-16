#ifndef BASE_DEFINES_H
#define BASE_DEFINES_H

namespace base {

   /** This enumeration list all known data types
    *  It is just for convenience to put them all together */

   enum EStreamDataTypes {
      proc_RocEvent     =  1,   //!< complete event from one roc board (iSubcrate = rocid)
      proc_ErrEvent     =  2,   //!< one or several events with corrupted data inside (iSubcrate = rocid)
      proc_MergedEvent  =  3,   //!< sorted and synchronized data from several rocs (iSubcrate = upper rocid bits)
      proc_RawData      =  4,   //!< unsorted uncorrelated data from one ROC, no SYNC markers required (mainly for FEET case)
      proc_Triglog      =  5,   //!< subevent produced by MBS directly with sync number and trigger module scalers
      proc_TRD_MADC     =  6,   //!< subevent produced by MBS directly with CERN-Nov10 data
      proc_TRD_Spadic   =  7,   //!< collection of data from susibo board (spadic 0.3)
      proc_CERN_Oct11   =  8,   //!< id for CERN beamtime in October 11
      proc_SlaveMbs     =  9,   //!< subevent produce by slave MBS system, which emulates number of triglog module
      proc_EPICS        = 10,   //!< subevent produced by dabc EPICS plugin (ezca)
      proc_COSY_Nov11   = 11,   //!< id for COSY beamtime in November 11
      proc_SpadicV10Raw = 12,   //!< raw data from single spadic V1.0 chip
      proc_SpadicV10Event = 13, //!< dabc-packed data for spadic V1.0 from SP605
      proc_CERN_Oct12   = 14,   //!< id for CERN beamtime in October 12
      proc_FASP         = 15,   //!< id for the FASP data
      proc_TRBEvent     = 31    //!< container for TRB frontend data
   };

   enum AnalysisKind {
      kind_RawOnly,     //!< make first scan only, no output can be produced
      kind_Raw,         //!< make first scan only, no output event produced, one could switch to other kinds
      kind_Triggered,   //!< triggered mode, after each input event single output event is produced and all data flushed
      kind_Stream       //!< normal analysis
   };

   typedef void* H1handle;
   typedef void* H2handle;
   typedef void* C1handle;
}

#endif
