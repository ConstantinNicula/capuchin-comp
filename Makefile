### OS SPECIFICS ###
CLEANUP=rm -f
MKDIR=mkdir -p

#.PHONY: clean
#.PHONY: test

### PATHS AND SOURCES ### 
PATHU = unity/src/
PATHS = src/
PATHT = test/

PATHB = build/
PATHD = build/depends/
PATHO = build/objs/
PATHR = build/results/

BUILD_PATHS = $(PATHB) $(PATHD) $(PATHO) $(PATHR)


# all obj files 
SRC = $(wildcard $(PATHS)*.c)
SRC_OBJ = $(patsubst $(PATHS)%.c, $(PATHO)%.o, $(SRC))

# tell compiler where to look for tests 
SRCT = $(wildcard $(PATHT)*.c)

### TOOLCHAIN SETUP ###
COMPILE=gcc -c
LINK=gcc
DEPEND=gcc -MM -MG -MF
CFLAGS=-I. -I$(PATHU) -I$(PATHS) -DTEST -Wall -g3 -std=c99

RESULTS = $(patsubst $(PATHT)test_%.c, $(PATHR)test_%.txt, $(SRCT))


### RULES ### 
PASSED = `grep -s PASS $(PATHR)*.txt`
FAIL = `grep -s "FAIL\|Abort\|Assertion\|Segmentation" $(PATHR)*.txt`
IGNORE = `grep -s IGNORE $(PATHR)*.txt`

test: $(BUILD_PATHS) $(RESULTS)
	@echo "\033[0;33m-----------------------\nIGNORES:\n-----------------------"
	@echo "$(IGNORE)\033[0m"
	@echo "\033[0;32m-----------------------\nPASSED:\n-----------------------"
	@echo "$(PASSED)\033[0m"
	@echo "\033[0;31m-----------------------\nFAILURES:\n-----------------------"
	@echo "$(FAIL)\033[0m"
	@echo "\nDONE"


$(PATHR)%.txt: $(PATHB)%.out
	-./$< > $@ 2>&1

$(PATHB)test_%.out: $(PATHO)test_%.o $(SRC_OBJ) $(PATHO)unity.o 
	$(LINK) -o $@ $^

$(PATHO)%.o:: $(PATHT)%.c 
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHS)%.c 
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHU)%.c $(PATHU)%.h
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHD)%.d:: $(PATHT)%.c
	$(DEPEND) $@ $<

$(PATHB):
	$(MKDIR) $(PATHB)

$(PATHD):
	$(MKDIR) $(PATHD)

$(PATHO):
	$(MKDIR) $(PATHO)

$(PATHR):
	$(MKDIR) $(PATHR)


repl: capuchin 

capuchin: $(PATHO)/repl.o $(SRC_OBJ)
	$(LINK) -o $@ $^

$(PATHO)/repl.o: $(PATHS)/repl/repl.c 
	$(COMPILE) $(CFLAGS) $< -o $@

clean: 
	$(CLEANUP) $(PATHO)*.o
	$(CLEANUP) $(PATHB)*.out
	$(CLEANUP) $(PATHR)*.txt 

.PRECIOUS: $(PATHB)test_%.out
.PRECIOUS: $(PATHD)%.d 
.PRECIOUS: $(PATHO)%.o 
.PRECIOUS: $(PATHR)%.txt 
