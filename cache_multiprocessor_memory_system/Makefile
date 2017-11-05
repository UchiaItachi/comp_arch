RM        := /bin/rm -rf
SIM       := ./sim
CC        := g++
CFLAGS    := -O2 -lm -W -Wall -Wno-unused-parameter
DFLAGS    := -pg -g
PFLAGS    := -pg



all: 
	${CC} ${CFLAGS} core.cpp dram.cpp cache.cpp  sim.cpp memsys.cpp   -o ${SIM}


clean: 
	$(RM) ${SIM} *.o 
