
all: busy_beaver

MAKEFILES = Makefile
CXXFLAGS += -std=c++11 -Wall -O3 -march=native
LDFLAGS += -lgmpxx -lgmp
LINKER ?= $(CXX)

OBJS = \
	main.o \
	turing_machine.o \
	rule_table.o \
	micro_machine.o \
	macro_machine.o \
	proof_machine.o \
	tests.o

*.o: $(MAKEFILES)

get-deps:
	sudo apt-get update
	sudo apt-get install -y libgmp-dev
.PHONY: get-deps

busy_beaver: $(OBJS)
	$(LINKER) -o $@ $^ $(LDFLAGS)

test: busy_beaver
	timeout 10 ./busy_beaver --test

test_long: busy_beaver
	timeout 180 ./busy_beaver --test_long

clean:
	rm -f *.o busy_beaver
	rm -f $(DEPDIR)/*.d
	rm -f $(DEPDIR)/*.Td
	rmdir $(DEPDIR)
.PHONY: clean

include autodep.mk
