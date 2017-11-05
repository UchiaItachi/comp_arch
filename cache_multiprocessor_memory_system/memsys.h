#ifndef MEMSYS_H
#define MEMSYS_H

#include "types.h"
#include "cache.h"
#include "dram.h"

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

typedef struct Memsys Memsys;

struct Memsys
{
  Cache *dcache;  // For Part A
  Cache *icache;  // For Part A,B
  Cache *l2cache; // For Part A,B
  DRAM *dram;    // For Part A,B

  // stats
  uint64_t stat_ifetch_access;
  uint64_t stat_load_access;
  uint64_t stat_store_access;
  uint64_t stat_ifetch_delay;
  uint64_t stat_load_delay;
  uint64_t stat_store_delay;
};



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

Memsys *memsys_new();
void memsys_print_stats(Memsys *sys);

uint64_t memsys_access(Memsys *sys, Addr addr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id);


// For mode B and mode C you must use this function to access L2 
uint64_t memsys_L2_access(Memsys *sys, Addr lineaddr, Flag is_writeback, uint32_t core_id);

///////////////////////////////////////////////////////////////////

#endif // MEMSYS_H
