WARNINGS= \
-Wall -Wno-unused-function -Wconversion -Wpointer-arith -Wparentheses \
-Wcast-qual -Wcast-align -Wno-sign-compare -Wno-unknown-pragmas -Wno-conversion \
 -Werror

RELEASE=.
BIN=$(RELEASE)/bin
OBJ=.obj
LIB=$(RELEASE)/lib
DOC=$(RELEASE)/doc

DEFINES=-DCHECK_LOGIC

# Target list
DIRS=$(BIN) $(LIB) $(OBJ)

NODENAME := $(shell uname -n)

SYS := $(shell g++ -dumpmachine)

ifneq (, $(findstring darwin, $(SYS)))
$(info Mac build)
		CXXFLAGS+=-fPIC
		TOOLS_BIN=/usr/bin
		ECHO=/bin/echo
		MD=/bin/mkdir
		GCC=g++
		DOXYGEN=$(TOOLS_BIN)/doxygen
		STRIP=$(TOOLS_BIN)/strip
		AR=$(TOOLS_BIN)/ar
		RM=rm
		MAKE=$(TOOLS_BIN)/make
		FIND=$(TOOLS_BIN)/find
		XARGS=$(TOOLS_BIN)/xargs
		DIRNAME=$(TOOLS_BIN)/dirname
		DIFF=$(TOOLS_BIN)/diff
		GREP=$(TOOLS_BIN)/grep
		AWK=$(TOOLS_BIN)/awk
		WC=$(TOOLS_BIN)/wc
		CAT=cat
		DYN_EXT=dynlib
		COPY=cp -f
else
		CXXFLAGS+=-fPIC
		TOOLS_BIN=/usr/bin
		ECHO=/bin/echo -e
		MD=/bin/mkdir
		GCC=g++
		DOXYGEN=$(TOOLS_BIN)/doxygen
		STRIP=$(TOOLS_BIN)/strip
		AR=$(TOOLS_BIN)/ar
		RM=rm
		MAKE=$(TOOLS_BIN)/make
		FIND=$(TOOLS_BIN)/find
		XARGS=$(TOOLS_BIN)/xargs
		DIRNAME=$(TOOLS_BIN)/dirname
		DIFF=$(TOOLS_BIN)/diff
		GREP=$(TOOLS_BIN)/grep
		AWK=$(TOOLS_BIN)/awk
		WC=$(TOOLS_BIN)/wc
		CAT=cat
		DYN_EXT=so
		CXXFLAGS+-fmax-errors=5
		COPY=cp -f
endif

TARGET=$(LIB)/libklfm18.$(DYN_EXT)
TEST_APP=$(BIN)/klfm_test

INCLUDES+=-Iinclude/ -I$(LIBERTY_INCLUDE) -I.

CXXFLAGS+=$(INCLUDES) $(DEFINES) -std=c++17 $(WARNINGS) 

debug: $(DIRS) $(TARGET) $(TEST_APP)
release : $(DIRS) $(DOC) $(TARGET)  $(TEST_APP)

release: CXXFLAGS += -Ofast
debug: CXXFLAGS += -DDEBUG -g -O0 -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC

release: STRIP_CMD=@$(ECHO) Stripping $@; $(STRIP) $@
debug: STRIP_CMD=

strip_all: $(TARGET)
	@$(ECHO) Stripping $(TARGET)
	@$(STRIP) $(TARGET)
	@$(STRIP) $(LEF_TEST_APP)

OBJS+=\
        $(OBJ)/bucket.o \
        $(OBJ)/cell.o \
        $(OBJ)/celllist.o \
        $(OBJ)/hypergraph.o \
        $(OBJ)/net.o \
        $(OBJ)/partition.o \
        $(OBJ)/pin.o \
        $(OBJ)/solution.o \
        $(OBJ)/iteration.o \
		$(OBJ)/klfm18.o

-include $(OBJ)/*.depend

$(OBJ)/%.o:src/%.cpp
	@$(ECHO) Compiling $<
	@$(GCC) $(CXXFLAGS) -c -o $@ $<
	@x=$(OBJ)/$*.depend; if [ ! -f  $$x ]; then $(ECHO) Generating $$x;  printf $(OBJ)/ > $$x; $(GCC) -MM $(CXXFLAGS) -c $<  >> $$x; fi

$(TARGET) : $(OBJS)
	@$(ECHO) Linking $@
	@$(GCC) -shared -o $@ $(OBJS)
	@$(STRIP_CMD)
	chmod -x $@
	$(DONE)

TEST_OBJS+=\
	   $(OBJ)/test.o \
	   $(OBJ)/testbuilder.o

$(TEST_APP): $(TEST_OBJS) $(TARGET)
	@$(ECHO) Linking $@
	@$(GCC) -o $@ $(TEST_OBJS) -lstdc++ $(TARGET) -lgtest
	@$(STRIP_CMD)
	$(DONE)

MKDIR=if [ ! -d $@ ]; then echo "Creatng folder $@"; $(MD) -p $@; fi

$(OBJ):; @$(MKDIR)

$(BIN):; @$(MKDIR)

$(LIB):; @$(MKDIR)

distclean:
	@$(ECHO) Cleanning distribution
	$(RM) -fr $(RELEASE) $(BIN) $(OBJ) *~ $(DOC) Doxyfile.bak $(LIB)
	@-($(RM) -fr a.out*) > /dev/null 2>&1
	@$(ECHO) Done

clean:
	@$(ECHO) Cleanning work folder
	$(RM) -fr $(BIN) $(LEF_LIB) $(OBJ) *~ *.hpp *.cpp *.output Doxyfile.bak lib obj bin doc
	$(RM) -fr  ui_*.h
	@for i in $(CONFIG_LIB); do echo "Removing "$$i" .."; $(RM) -fr $$i $$i.dSYM; done

doc: $(DOC)

$(DOC): Doxyfile
	@$(ECHO) Generating documentation
	@$(MKDIR)
	@-$(DOXYGEN) Doxyfile

test: $(TEST_APP)
	$(TEST_APP)

install: test
	@$(ECHO) Copying $(TARGET) to $(OPENCAD)/lib/
	@$(COPY) $(TARGET) $(OPENCAD)/lib/
	
help: FORCE
	@$(ECHO) "========================================================================"
	@$(ECHO) "Run 'make' or 'make release' to make optimized '"$(TARGET)"' executable "
	@$(ECHO) "'make debug' to make '"$(TARGET)"' executable with debug information"
	@$(ECHO) "'make test' to run smoke tests from test folder"
	@$(ECHO) "'make golden' to create GOLDEN files for new tests"
	@$(ECHO) "'make help' to print this message"
	@$(ECHO) "'make clean' to clean local build, binaries and obj's"
	@$(ECHO) "'make distclean' to deep clean"
	@$(ECHO) "========================================================================"

FORCE:

# If you want to see intermediary files, uncomment next line; It cause repeated compilation
#.PRECIOUS: shelby_wrap.cpp
