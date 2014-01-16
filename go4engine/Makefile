ifndef GO4SYS
GO4SYS = $(shell go4-config --go4sys 2>/dev/null)
ifeq ($(GO4SYS),)
$(error Go4 not found  - call '. go4login' before compilation !!!)
endif
endif

ifndef STREAMSYS
$(error STREAMSYS variable notconfigured)
endif


-include $(GO4SYS)/Makefile.config

INCLUDES += -I$(STREAMSYS)/include  
#DEFINITIONS += $(ROC_GO4DEFINITIONS) 

## normally should be like this for every module, but can be specific

NEWGO4_LINKDEF       = LinkDef.$(HedSuf)


NEWGO4_NOTLIBF     = 

## must be similar for every module

NEWGO4_DICT        = $(DICT_PREFIX)NewGo4Monitor
NEWGO4_DH          = $(NEWGO4_DICT).$(HedSuf)
NEWGO4_DS          = $(NEWGO4_DICT).$(SrcSuf)
NEWGO4_DO          = $(NEWGO4_DICT).$(ObjSuf)

NEWGO4_H           = TStreamEvent.h \
                     TUserSource.h \
                     TFirstStepProcessor.h

NEWGO4_S           = TUserSource.cxx \
                     TFirstStepProcessor.cxx

NEWGO4_O           = $(NEWGO4_S:.$(SrcSuf)=.$(ObjSuf))

NEWGO4_DEP         =  $(NEWGO4_O:.$(ObjSuf)=.$(DepSuf))
NEWGO4_DDEP        =  $(NEWGO4_DO:.$(ObjSuf)=.$(DepSuf))

NEWGO4_LIBNAME     = $(GO4_USERLIBNAME)

NEWGO4_LIB         = $(STREAMSYS)/lib/$(NEWGO4_LIBNAME).$(DllSuf)

NEWGO4_LIB_DEP    = $(STREAMSYS)/lib/libStreamDict.so \
                    $(STREAMSYS)/lib/libStream.so \
                    $(ANAL_LIB_DEP) 


# used in the main Makefile

EXAMPDEPENDENCS    += $(NEWGO4_DEP) $(NEWGO4_DDEP)


##### local rules #####

all:: $(NEWGO4_LIB) 

dict:
	echo "Dictionary done"

$(NEWGO4_LIB):  $(NEWGO4_O) $(NEWGO4_DO)
	@$(MakeLibrary) $(NEWGO4_LIBNAME) "$(NEWGO4_O) $(NEWGO4_DO)" $(STREAMSYS)/lib $(NEWGO4_LINKDEF) "$(NEWGO4_LIB_DEP)" 

$(NEWGO4_LIB): LDFLAGS+=-Wl,-rpath,$(STREAMSYS)/lib -L$(STREAMSYS)/lib -lStream -lStreamDict 

$(NEWGO4_DS): $(NEWGO4_H) $(NEWGO4_LINKDEF)
	@$(ROOTCINTGO4) $(NEWGO4_H) $(NEWGO4_LINKDEF)


clean: 
	@rm -f $(NEWGO4_O) $(NEWGO4_DO) 
	@rm -f $(NEWGO4_LIB) 
	@rm -f $(NEWGO4_DEP) $(NEWGO4_DDEP) $(NEWGO4_DS) $(NEWGO4_DH)
	@echo "Clean everything in new go4 monitor"

-include $(GO4SYS)/Makefile.rules
