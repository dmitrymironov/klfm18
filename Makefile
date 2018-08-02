WARNINGS= \
-Wall -Wno-unused-function -Wconversion -Wpointer-arith -Wparentheses \
-Wcast-qual -Wcast-align -Wno-sign-compare -Wno-unknown-pragmas -Wno-conversion \
 -Werror

RELEASE=${OPENCAD}
BIN=$(RELEASE)/bin
OBJ=.obj
LIB=$(RELEASE)/lib
DOC=$(RELEASE)/doc

# Target list
DIRS=$(BIN) $(LIB) $(OBJ)

NODENAME := $(shell uname -n)

SYS := $(shell g++ -dumpmachine)

ifneq (, $(findstring darwin, $(SYS)))
$(info Mac build)
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
endif

TARGET=$(LIB)/libklfm18.$(DYN_EXT)

INCLUDES+=-Iinclude/ -I$(LIBERTY_INCLUDE) -I.

CXXFLAGS+=$(INCLUDES) $(DEFINES) -std=c++17 $(WARNINGS)

debug: $(DIRS) $(TARGET)
release : $(DIRS) $(DOC) $(TARGET)

release: CXXFLAGS += -Ofast
debug: CXXFLAGS += -DDEBUG -g -O0 -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC

release: STRIP_CMD=@$(ECHO) Stripping $@; $(STRIP) $@
debug: STRIP_CMD=

strip_all: $(TARGET)
	@$(ECHO) Stripping $(TARGET)
	@$(STRIP) $(TARGET)
	@$(STRIP) $(LEF_TEST_APP)

OBJS+=\
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
	$(DONE)


MKDIR=if [ ! -d $@ ]; then echo "Creatng folder $@"; $(MD) -p $@; fi

$(OBJ):; @$(MKDIR)

$(BIN):; @$(MKDIR)

$(LIB):; @$(MKDIR)

distclean:
	@$(ECHO) Cleanning distribution
	$(RM) -fr $(RELEASE) $(BIN) $(OBJ) config_*.* lef_*.* *~ $(DOC) Doxyfile.bak $(LIB)
	@-(cd liberty_parse-2.6; make distclean; $(RM) -fr a.out* attr_enum.h attr_lookup attr_lookup.c group_enum.h group_lookup group_lookup.c syntax_decls.c syntax_decls.h) > /dev/null 2>&1
	@$(ECHO) Done

clean:
	@$(ECHO) Cleanning work folder
	$(RM) -fr $(BIN) $(LEF_LIB) $(OBJ) config_*.* lef_*.* *~ *.hpp *.cpp *.output Doxyfile.bak
	$(RM) -fr  lef_lexer.cpp config_parser.cpp config_lexer.cpp lef_parser.cpp shelby_wrap.cpp ui_*.h
	@for i in $(LEF_LIB) $(ABSTRACT_LIB) $(LIBERTE_LIB) $(VERIFY_LIB) $(SPICE_LIB) $(DEF_LIB) $(VERILOG_LIB) $(GDS_LIB) $(CONFIG_LIB); do echo "Removing "$$i" .."; $(RM) -fr $$i $$i.dSYM; done
	$(RM) -fr init_tcl.*

doc: $(DOC)

$(DOC): Doxyfile
	@$(ECHO) Generating documentation
	@$(MKDIR)
	@-$(DOXYGEN) Doxyfile

golden: FORCE
	@for i in `$(FIND) test -name config -type f | $(XARGS) -n 1 $(DIRNAME)`; do \
		if ! [ -f $$i/GOLDEN ]; then $(ECHO) Creating $$i/GOLDEN ..; $(TARGET) $$i/config > $$i/GOLDEN 2>&1; fi; \
	done; \
	for i in `$(FIND) test/lef -type f`; do \
		if ! [ -f `$(DIRNAME) $$i`/GOLDEN ]; then $(ECHO) Creating `$(DIRNAME) $$i`/GOLDEN ..; $(LEF_TEST_APP) $$i > `$(DIRNAME) $$i`/GOLDEN 2>&1; fi; \
	done; \
	for i in `$(FIND) test -name run.sh -type f | $(XARGS) -n 1 $(DIRNAME)`; do \
		if ! [ -f $$i/GOLDEN ]; then $(ECHO) Creating $$i/GOLDEN ..;	\
		(cd $$i; PATH=${PATH} ./run.sh;) > $$i/GOLDEN 2>&1; \
		fi; \
	done;

#	for i in `$(FIND) acceptance_test -name shelby.config -type f | $(XARGS) -n 1 $(DIRNAME)`; do \
#		if ! [ -f $$i/GOLDEN ]; then $(ECHO) Creating $$i/GOLDEN ..; PATH=${PATH} $(TARGET) $$i/shelby.config > $$i/GOLDEN 2>&1; fi; \
#	done;

test: FORCE
	@$(RM) -f .failed.tc
	@for i in `$(FIND) test -name config -type f | $(XARGS) -n 1 $(DIRNAME)`; do \
	if [ -f $$i/GOLDEN ]; then \
		/bin/echo -n "$$i .. "; \
		PATH=${PATH} $(TARGET) $$i/config > .tmp.golden 2>&1; \
			if $(DIFF) .tmp.golden $$i/GOLDEN > /dev/null; then $(ECHO) ok; else $(ECHO) fail; $(ECHO) "Run following command:\n\nPATH=${PATH} $(TARGET) $$i/config\n\n"; echo "PATH=${PATH} $(TARGET) $$i/config">>.failed.tc; fi; \
$(DIFF) .tmp.golden $$i/GOLDEN; \
			$(RM) .tmp.golden; \
		fi; \
	done; \
	for i in `$(FIND) test/lef -type f | $(GREP) -v GOLDEN`; do \
		/bin/echo -n `$(DIRNAME) $$i`" .. "; \
		$(LEF_TEST_APP) $$i > .tmp.golden 2>&1;  \
		if $(DIFF) .tmp.golden `$(DIRNAME) $$i`/GOLDEN > /dev/null; then $(ECHO) ok; \
		else $(ECHO) fail; $(ECHO)  "Run following command:\n\n$(LEF_TEST_APP) $$i\n\n"; echo "$(LEF_TEST_APP) $$i">>.failed.tc; fi; \
		$(DIFF) .tmp.golden `$(DIRNAME) $$i`/GOLDEN; \
		$(RM) .tmp.golden;  \
	done; \
	for i in `$(FIND) test -name run.sh | $(XARGS) -n 1 $(DIRNAME)`; do \
		/bin/echo -n $$i" .. "; \
		 (cd $$i; PATH=$(PATH) ./run.sh > .tmp.golden 2>&1); \
		if $(DIFF) $$i/.tmp.golden $$i/GOLDEN > /dev/null; then $(ECHO) ok; \
		else $(ECHO) fail; $(ECHO) "Run following command:\n\ncd $$i; PATH=${PATH} ./run.sh;\n\n"; \
		echo "cd $$i; PATH=${PATH} ./run.sh;">>.failed.tc; fi; \
		$(RM) $$i/.tmp.golden; \
	done;
	@if [ -f .failed.tc ]; then $(ECHO) "\nFailed "`$(WC) -l .failed.tc | $(AWK) '{ print $$1 }'`" testcases:"; $(CAT) .failed.tc; \
		else $(ECHO) "\nGood job "`whoami`"! All tests have passed";  fi;
	@$(RM) -f .failed.tc

help: FORCE
	@$(ECHO) "========================================================================"
	@$(ECHO) "Run 'make' or 'make release' to make optimized '"$(TARGET)"' executable "
	@$(ECHO) "'make debug' to make '"$(TARGET)"' executable with debug information"
	@$(ECHO) "'make test' to run smoke tests from test folder"
	@$(ECHO) "'make golden' to create GOLDEN files for new tests"
	@$(ECHO) "'make help' to print this message"
	@$(ECHO) "'make clean' to clean local build, binaries and obj's"
	@$(ECHO) "'make distclean' to deep clean the build staff including liberty"
	@$(ECHO) "========================================================================"

FORCE:

# If you want to see intermediary files, uncomment next line; It cause repeated compilation
#.PRECIOUS: lef_lexer.cpp config_parser.cpp config_lexer.cpp lef_parser.cpp shelby_wrap.cpp
