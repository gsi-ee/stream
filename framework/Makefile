#use minimum options set


ROOT_VERSION = $(shell root-config --version 2>/dev/null)

ifneq ($(ROOT_VERSION),)
IS_ROOT=true
ifneq ($(firstword $(subst ., ,$(ROOT_VERSION))),5)
IS_ROOT6=true
endif
endif

CPPVERS      = -std=c++11

CXXPLATFORMFLAGS   = -m64
CXXOPT       += $(CPPVERS) -O2 -fPIC $(CXXPLATFORMFLAGS) -Wall $(INCLUDES:%=-I%) $(DEFINITIONS:%=-D%)

ifeq ($(shell uname -m),aarch64)
CXXPLATFORMFLAGS := 
DEFINITIONS += STREAM_AARCH64
endif

ifeq ($(shell uname),Darwin)
CXX           = clang++
LD            = clang++
LDFLAGSPRE    =
else
CXX           = g++
LD            = g++
LDFLAGSPRE    = -Wl,--as-needed
endif

ifdef clang
LD            = clang++
CXX           = clang++
endif

ifdef debug
CXXOPT      += -g
endif

INCLUDES  = $(STREAMSYS)/include

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
NEWLIBDICT_SRC = $(NEWLIBDICT_NAME).cxx
NEWLIBDICT_OBJ = $(NEWLIBDICT_NAME).o

LIBTGTS = $(NEWLIB)

ifdef IS_ROOT
LIBTGTS += $(NEWLIBDICT_LIB) $(NEWLIBDICT_MAP)
endif

all: $(LIBTGTS)

lib: $(LIBTGTS)

clean:
	@rm -f $(NEWLIB) $(NEWLIB_OBJS) $(NEWLIB_DEPS) $(NEWLIBROOT_OBJS) $(NEWLIBROOT_DEPS) $(NEWLIBDICT_MAP) $(NEWLIBDICT_NAME).* $(NEWLIBDICT_LIBNAME)_rdict.pcm


ifdef IS_ROOT

$(NEWLIBDICT_MAP): $(NEWLIBDICT_LIB)
	@echo 'Building: $@'
ifdef IS_ROOT6
	@rootcling -v2 -f $(NEWLIBDICT_SRC) -s $(NEWLIBDICT_LIB) -m $(NEWLIBDICT_LIBNAME)_rdict.pcm -excludePath $(STREAMSYS) -rml $(NEWLIBDICT_LIBNAME).so -rmf $(NEWLIBDICT_MAP) -I$(STREAMSYS)/include $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
	@touch $(NEWLIBDICT_OBJ)
	@touch $(NEWLIBDICT_LIB)
	@touch $(NEWLIBDICT_MAP)
else
	@rlibmap -o $@ -l $(NEWLIBDICT_LIB) -d $(NEWLIB) -c $(NEWLIBDICT_LINKDEF)
endif

$(NEWLIBDICT_SRC): $(NEWLIBDICT_LINKDEF) $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS)
ifdef IS_ROOT6
	@echo 'Creating root6 dictionary: $@'
	@rootcling -f $@ -s $(NEWLIBDICT_LIBNAME).so -c $(CXXOPT) $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
else
	@echo 'Creating root5 dictionary: $@'
	@rootcint -f $@ -c -p -I../include $(NEWLIB_HEADERS) $(NEWLIBROOT_HEADERS) $(NEWLIBDICT_LINKDEF)
endif

$(NEWLIBDICT_LIB): $(NEWLIBDICT_OBJ) $(NEWLIBROOT_OBJS) $(NEWLIBDICT_LINKDEF)
	@echo 'Building: $@'
	@$(LD) -o $@ -shared -O $(NEWLIBDICT_OBJ) $(NEWLIBROOT_OBJS) -L$(STREAMSYS)/lib -lStream -Wl,-rpath,$(STREAMSYS)/lib $(shell root-config --libs)

$(NEWLIBDICT_SRC) $(NEWLIBROOT_OBJS) $(NEWLIBROOT_DEPS) : CXXOPT += $(shell root-config --cflags)

endif

$(NEWLIB) : $(NEWLIB_OBJS)
	@echo 'Building: $@'
	$(LD) -shared $(LDFLAGSPRE) -O $(NEWLIB_OBJS) -o $@

# rules	
%.d: %.cxx
	@echo "Build dependency for $< ..."
	@$(CXX) $(CXXOPT) $< -o $@ -MM -MT "$*.o $*.d"

$(NEWLIBDICT_OBJ): $(NEWLIBDICT_SRC)
	$(CXX) -c $< $(CXXOPT) $(shell root-config --cflags) -o $@

%.o: %.cxx
	$(CXX) -c $< $(CXXOPT) -o $@

ifeq ($(findstring $(MAKECMDGOALS), clean),)
-include $(NEWLIB_DEPS) 
ifdef IS_ROOT
-include $(NEWLIBROOT_DEPS)
endif
endif

