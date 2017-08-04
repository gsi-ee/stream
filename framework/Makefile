#use minimum options set


ROOT_VERSION = $(shell root-config --version 2>/dev/null)


ifneq ($(ROOT_VERSION),)
IS_ROOT=true
ifneq ($(firstword $(subst ., ,$(ROOT_VERSION))),5)
IS_ROOT6 = true
endif
endif

CXXOPT       += -O2 -fPIC -Wall $(INCLUDES:%=-I%) $(DEFINITIONS:%=-D%) 
LD            = g++
CXX           = g++

ifdef debug
CXXOPT      += -O0 -g
endif

INCLUDES      = $(STREAMSYS)/include

NEWLIB = $(STREAMSYS)/lib/libStream.so

NEWLIB_HEADERS = $(wildcard $(STREAMSYS)/include/base/*.h) \
                 $(wildcard $(STREAMSYS)/include/nx/*.h) \
                 $(wildcard $(STREAMSYS)/include/get4/*.h) \
                 $(wildcard $(STREAMSYS)/include/hadaq/*.h) \
                 $(wildcard $(STREAMSYS)/include/mbs/*.h) \
                 $(STREAMSYS)/include/dabc/BinaryFile.h

NEWLIB_SRCS =    $(wildcard base/*.cxx) \
                 $(wildcard nx/*.cxx) \
                 $(wildcard get4/*.cxx) \
                 $(wildcard hadaq/*.cxx) \
                 $(wildcard mbs/*.cxx)

NEWLIB_OBJS = $(patsubst %.cxx, %.o, $(NEWLIB_SRCS))
NEWLIB_DEPS = $(patsubst %.cxx, %.d, $(NEWLIB_SRCS))

NEWLIBROOT_HEADERS = $(wildcard $(STREAMSYS)/include/root/*.h)
NEWLIBROOT_SRCS    = $(wildcard root/*.cxx)
NEWLIBROOT_OBJS    = $(patsubst %.cxx, %.o, $(NEWLIBROOT_SRCS))
NEWLIBROOT_DEPS    = $(patsubst %.cxx, %.d, $(NEWLIBROOT_SRCS))


NEWLIBDICT_LIBNAME = libStreamDict
NEWLIBDICT_LIB = $(STREAMSYS)/lib/$(NEWLIBDICT_LIBNAME).so
NEWLIBDICT_MAP = $(STREAMSYS)/lib/$(NEWLIBDICT_LIBNAME).rootmap

NEWLIBDICT_LINKDEF = LinkDef.h
NEWLIBDICT_NAME = G__Stream
NEWLIBDICT_HDR = $(NEWLIBDICT_NAME).h
NEWLIBDICT_SRC = $(NEWLIBDICT_NAME).cxx
NEWLIBDICT_OBJ = $(NEWLIBDICT_NAME).o

all: lib dict

lib: $(NEWLIB)

clean:
	rm -f $(NEWLIB) $(NEWLIB_OBJS) $(NEWLIB_DEPS) $(NEWLIBROOT_OBJS) $(NEWLIBROOT_DEPS) $(NEWLIBDICT_MAP) $(NEWLIBDICT_NAME).* $(NEWLIBDICT_LIBNAME)_rdict.pcm


ifdef IS_ROOT

$(NEWLIBDICT_MAP): $(NEWLIBDICT_LIB)
	@echo 'Building: $@'
ifdef IS_ROOT6
	@rootcling -v2 -f $(NEWLIBDICT_SRC) -s $(NEWLIBDICT_LIB) -m $(NEWLIBDICT_LIBNAME)_rdict.pcm -excludePath $(STREAMSYS) -rml $(NEWLIBDICT_LIBNAME).so -rmf $(NEWLIBDICT_MAP) -I$(STREAMSYS)/include $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
else
	@rlibmap -o $@ -l $(NEWLIBDICT_LIB) -d $(NEWLIB) -c $(NEWLIBDICT_LINKDEF)
endif

$(NEWLIBDICT_SRC): $(NEWLIBDICT_LINKDEF) $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS)
ifdef IS_ROOT6
	@rootcling -f $@ -s $(NEWLIBDICT_LIBNAME).so -c $(CXXOPT) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
else
	@rootcint -f $@ -c -p -I../include $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
endif

$(NEWLIBDICT_OBJ): $(NEWLIBDICT_SRC) $(NEWLIBDICT_LINKDEF)

$(NEWLIBDICT_OBJ): CXXOPT += $(shell root-config --cflags)
#$(NEWLIBDICT_OBJ): INCLUDES += $(STREAMSYS)/include

$(NEWLIBDICT_LIB): $(NEWLIBDICT_OBJ) $(NEWLIBROOT_OBJS) $(NEWLIBDICT_LINKDEF)
	@echo 'Building: $@'
	@$(LD) -o $@ -shared -O $(NEWLIBDICT_OBJ) $(NEWLIBROOT_OBJS) -L$(STREAMSYS)/lib -lStream -Wl,-rpath,$(STREAMSYS)/lib

dict: $(NEWLIBDICT_LIB) $(NEWLIBDICT_MAP)

$(NEWLIBROOT_OBJS) $(NEWLIBROOT_DEPS) : CXXOPT += $(shell root-config --cflags)

else

dict:
	@echo "ROOT not available, dictionary for $(NEWLIB) cannot be generated"

endif

$(NEWLIB) : $(NEWLIB_OBJS)
	@echo 'Building: $@'
	$(LD) -shared -Wl,--as-needed -O $(NEWLIB_OBJS) -o $@


# rules	
%.d: %.cxx
	@echo "Build dependency for $< ..."
	@rm -f $@ $@.bak; touch $@
	@makedepend -f$@ -Y -o.o -w 30000 -- $(CXXOPT) -- $< > /dev/null 2>&1
	@sed -i 's|$*.o|$*.o $*.d|g' $@
	@rm -f $@.bak

%.o: %.cxx
	$(CXX) -c $< $(CXXOPT) -o $@

ifeq ($(findstring $(MAKECMDGOALS), clean),)
-include $(NEWLIB_DEPS) $(NEWLIBROOT_DEPS)
endif

