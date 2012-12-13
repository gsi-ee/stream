ifndef GO4SYS
$(error Go4 not found  - call '. go4login' before compilation !!!)
endif

ifndef STREAMSYS
$(error STREAMSYS packet not found - call '. roclogin' before compilation !!!)
endif

-include $(GO4SYS)/Makefile.config


INCLUDES += -I$(STREAMSYS)/include
#DEFINITIONS += $(ROC_GO4DEFINITIONS) 

## normally should be like this for every module, but can be specific

GET4TEST_LINKDEF       = LinkDef.$(HedSuf)


GET4TEST_NOTLIBF     = 

## must be similar for every module

GET4TEST_DICT        = $(DICT_PREFIX)Get4Test
GET4TEST_DH          = $(GET4TEST_DICT).$(HedSuf)
GET4TEST_DS          = $(GET4TEST_DICT).$(SrcSuf)
GET4TEST_DO          = $(GET4TEST_DICT).$(ObjSuf)

GET4TEST_H           = TGet4TestProc.h 

GET4TEST_S           = TGet4TestProc.cxx

GET4TEST_O           = $(GET4TEST_S:.$(SrcSuf)=.$(ObjSuf))

GET4TEST_DEP         =  $(GET4TEST_O:.$(ObjSuf)=.$(DepSuf))
GET4TEST_DDEP        =  $(GET4TEST_DO:.$(ObjSuf)=.$(DepSuf))

GET4TEST_LIBNAME     = $(LIB_PREFIX)Get4Test

GET4TEST_LIB         = $(GET4TEST_LIBNAME).$(DllSuf)

#GET4TEST_LIB_DEP    = $(ROC_LIBDIR)/libNewMonitorDict.so \
                      $(ROC_LIBDIR)/libNewMonitor.so \
                      $(ROC_LIBDIR)/libRocDict.so \
                      $(ROCDICT_DEPLIBS) \
                      $(ANAL_LIB_DEP) 


# used in the main Makefile

EXAMPDEPENDENCS    += $(GET4TEST_DEP) $(GET4TEST_DDEP)


##### local rules #####

all:: $(GET4TEST_LIB) 


$(GET4TEST_LIB):  $(GET4TEST_O) $(GET4TEST_DO)
	@$(MakeLibrary) $(GET4TEST_LIBNAME) "$(GET4TEST_O) $(GET4TEST_DO)" $(CURDIR) $(GET4TEST_LINKDEF) 

#$(GET4TEST_LIB): LDFLAGS+=-Wl,-rpath,$(ROC_LIBDIR) -L$(ROC_LIBDIR) -lRoc -lRocDict -lNewMonitor -lNewMonitorDict 

$(GET4TEST_DS): $(GET4TEST_H)  $(GET4TEST_LINKDEF)
	$(ROOTCINTGO4) $(GET4TEST_H) $(GET4TEST_LINKDEF)


clean: 
	@rm -f $(GET4TEST_O) $(GET4TEST_DO) 
	@rm -f $(GET4TEST_LIB) $(GET4TEST_LIBNAME).* 
	@rm -f $(GET4TEST_DEP) $(GET4TEST_DDEP) $(GET4TEST_DS) $(GET4TEST_DH)
	@echo "Clean everything in get4test"

-include $(GO4SYS)/Makefile.rules
