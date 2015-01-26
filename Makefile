
ifndef STREAMSYS
STREAMSYS := $(CURDIR)
endif

LOGINFILE = streamlogin
FASTRULES = clean
LIBDIR    = lib 


.PHONY:    all baselib go4lib $(FASTRULES)



ifdef GO4SYS
IS_GO4=true
else
ifneq ($(shell go4-config --go4sys 2>/dev/null),)
IS_GO4=true
endif
endif


ifdef ROOTSYS
IS_ROOT=true
else
ifneq ($(shell root-config --version 2>/dev/null),)
IS_ROOT=true
endif
endif



all: baselib go4lib


baselib: $(LIBDIR) $(LOGINFILE)
	cd framework; $(MAKE) STREAMSYS=..

go4lib: baselib
ifdef IS_GO4
	cd go4engine; $(MAKE) STREAMSYS=..
else
	@echo "Go4 not configured - not able to build go4 engine"
endif


clean:
ifdef IS_GO4
	cd go4engine; $(MAKE) clean STREAMSYS=..
else
   @rm -f go4engine/G__*.* go4engine/*.o go4engine/*.d 
endif
	cd framework; $(MAKE) clean STREAMSYS=..
	@rm -rf $(LIBDIR) $(LOGINFILE)
	@echo "stream project clean done"

$(LIBDIR):
	@(if [ ! -d $(LIBDIR) ] ; then mkdir -p $(LIBDIR); fi)

$(LOGINFILE):
	@rm -f $@
	@echo "# this is generated file, use it to configure enviroment" >> $@
	@echo "# just type '. streamlogin' in shell, not forget space in between" >> $@
	@echo "" >> $@
	@echo "echo Configure stream v0.6.0 at $(STREAMSYS)" >> $@
	@echo "" >> $@
	@echo 'export STREAMSYS=$(STREAMSYS)' >> $@
	@echo 'export LD_LIBRARY_PATH=$$STREAMSYS/lib:$$LD_LIBRARY_PATH' >> $@
