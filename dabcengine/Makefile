## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

DABCSTREAMDIRS        = .
DABCSTREAMDIRI        = .

# now defined in the Makefile.config
DABCSTREAM_LIBNAME = $(LIB_PREFIX)DabcStream
DABCSTREAM_LIB     = ../lib/$(DABCSTREAM_LIBNAME).$(DllSuf)

DABCSTREAM_H     = $(wildcard $(DABCSTREAMDIRI)/*.$(HedSuf))
DABCSTREAM_S     = $(wildcard $(DABCSTREAMDIRS)/*.$(SrcSuf))
DABCSTREAM_O     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCSTREAM_S))
DABCSTREAM_D     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCSTREAM_S))

DABCSTREAM_LIBS  = -lDabcMbs -lDabcHadaq -lrt -Wl,-rpath,$(STREAMSYS)/lib -L$(STREAMSYS)/lib -lStream

# used in the main Makefile

ALLDEPENDENC      += $(DABCSTREAM_D)

libs:: $(DABCSTREAM_LIB)

clean::

##### local rules #####

$(DABCSTREAM_LIB):   $(DABCSTREAM_O)
	@$(MakeLib) $(DABCSTREAM_LIBNAME) "$(DABCSTREAM_O)" ../lib/ "$(DABCSTREAM_LIBS)"

########### extra rules #############
$(DABCSTREAM_O) $(DABCSTREAM_D): INCLUDES += ../include 

include $(DABCSYS)/config/Makefile.rules
