#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "memsys.h"

#include <exception>

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE SIM_MODE;
extern uint64_t CACHE_LINESIZE;
extern uint64_t REPL_POLICY;

extern uint64_t DCACHE_SIZE;
extern uint64_t DCACHE_ASSOC;
extern uint64_t ICACHE_SIZE;
extern uint64_t ICACHE_ASSOC;
extern uint64_t L2CACHE_SIZE;
extern uint64_t L2CACHE_ASSOC;


uint64_t memsys_L1_access(Memsys *sys, Addr lineaddr, Flag is_write, uint32_t core_id, bool isICache)
{
  uint64_t delay = 0;

  Cache *c = isICache ? sys->icache : sys->dcache;

  delay += DCACHE_HIT_LATENCY;

  Flag l1_access_outcome = cache_access(c, lineaddr, is_write, core_id);
  if (MISS == l1_access_outcome)
  {
    Flag is_writeback = FALSE;

    delay += memsys_L2_access(sys, lineaddr, is_writeback, core_id);
    cache_install(c, lineaddr, is_write, core_id);
    // ------------------------------ Write Back ----------------------------------------
    if (isVictimDirty(c))// && (!isICache))
    {
      is_writeback = TRUE; // ignore the delay
      memsys_L2_access(sys, c->last_evicted_line.tag, is_writeback, c->last_evicted_line.core_id);
      flushVictim(c);
    }
  }

  return delay;
}

// This function is called on ICACHE miss, DCACHE miss, DCACHE writeback
// ----- YOU NEED TO WRITE THIS FUNCTION AND UPDATE DELAY ----------

uint64_t memsys_L2_access(Memsys *sys, Addr lineaddr, Flag is_writeback, uint32_t core_id)
{
  uint64_t delay = 0;
  Flag is_write = FALSE;

  Cache* c = sys->l2cache;

  //To get the delay of L2 MISS, you must use the dram_access() function
  //To perform writebacks to memory, you must use the dram_access() function
  //This will help us track your memory reads and memory writes


  if (is_writeback)
  {
    // ------------------------------ Write Back ----------------------------------------
    is_write = TRUE;
    delay += L2CACHE_HIT_LATENCY;
    Flag l2_wb_outcome = cache_access(c, lineaddr, is_write, core_id);
    if (MISS == l2_wb_outcome)
    {
      delay += dram_access(sys->dram, lineaddr, false);
      cache_install(c, lineaddr, is_write, core_id);  // TODO: is_write or is_read ??
      if (isVictimDirty(c))
      {
        delay += dram_access(sys->dram, c->last_evicted_line.tag, is_write);
        flushVictim(c);
      }
    }
  }
  else
  {
    delay += L2CACHE_HIT_LATENCY;
    Flag l2_rw_outcome = cache_access(c, lineaddr, is_write, core_id); // TODO: miss in L1, read or write in L2 ?

    if (MISS == l2_rw_outcome)
    {
      delay += dram_access(sys->dram, lineaddr, false);  // TODO: again DRAM write or read ?
      cache_install(c, lineaddr, false, core_id);        // TODO: miss in L1, read or write in L2 ?
      if (isVictimDirty(c))
      {
        // ------------------------------ Write Back ----------------------------------------
        dram_access(sys->dram, c->last_evicted_line.tag, TRUE);
        flushVictim(c);
      }
    }
  }

  if (is_writeback)
  {
    delay = 0;
  }
  return delay;
}


uint64_t memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id)
{
  uint64_t delay = 0;

  const uint32_t is_write = (type == ACCESS_TYPE_STORE) ? TRUE : FALSE;

  if (type == ACCESS_TYPE_IFETCH)
  {
    delay += memsys_L1_access(sys, lineaddr, false, core_id, true);
  }


  if (type == ACCESS_TYPE_LOAD)
  {
    delay += memsys_L1_access(sys, lineaddr, false, core_id, false);
  }


  if (type == ACCESS_TYPE_STORE)
  {
    delay += memsys_L1_access(sys, lineaddr, true, core_id, false);
  }

  return delay;
}

Memsys *memsys_new(void)
{
  Memsys *sys = (Memsys *) calloc(1, sizeof(Memsys));

  sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);

  if (SIM_MODE != SIM_MODE_A)
  {
    sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
    sys->dram = dram_new();
  }

  return sys;

}

// This function takes an ifetch/ldst access and returns the delay
uint64_t memsys_access(Memsys *sys, Addr addr, Access_Type type, uint32_t core_id)
{
  uint32_t delay = 0;


  // all cache transactions happen at line granularity, so get lineaddr
  Addr lineaddr = addr / CACHE_LINESIZE;


  if (SIM_MODE == SIM_MODE_A)
  {
    delay = memsys_access_modeA(sys, lineaddr, type, core_id);
  }
  else
  {
    delay = memsys_access_modeBC(sys, lineaddr, type, core_id);
  }


  //update the stats
  if (type == ACCESS_TYPE_IFETCH)
  {
    sys->stat_ifetch_access++;
    sys->stat_ifetch_delay += delay;
  }

  if (type == ACCESS_TYPE_LOAD)
  {
    sys->stat_load_access++;
    sys->stat_load_delay += delay;
  }

  if (type == ACCESS_TYPE_STORE)
  {
    sys->stat_store_access++;
    sys->stat_store_delay += delay;
  }


  return delay;
}


void memsys_print_stats(Memsys *sys)
{
  char header[256];
  sprintf(header, "MEMSYS");

  double ifetch_delay_avg = 0;
  double load_delay_avg = 0;
  double store_delay_avg = 0;

  if (sys->stat_ifetch_access)
  {
    ifetch_delay_avg = (double) (sys->stat_ifetch_delay) / (double) (sys->stat_ifetch_access);
  }

  if (sys->stat_load_access)
  {
    load_delay_avg = (double) (sys->stat_load_delay) / (double) (sys->stat_load_access);
  }

  if (sys->stat_store_access)
  {
    store_delay_avg = (double) (sys->stat_store_delay) / (double) (sys->stat_store_access);
  }


  printf("\n");
  printf("\n%s_IFETCH_ACCESS  \t\t : %10llu", header, sys->stat_ifetch_access);
  printf("\n%s_LOAD_ACCESS    \t\t : %10llu", header, sys->stat_load_access);
  printf("\n%s_STORE_ACCESS   \t\t : %10llu", header, sys->stat_store_access);
  printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f", header, ifetch_delay_avg);
  printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f", header, load_delay_avg);
  printf("\n%s_STORE_AVGDELAY \t\t : %10.3f", header, store_delay_avg);
  printf("\n");

  cache_print_stats(sys->dcache, "DCACHE");

  if (SIM_MODE != SIM_MODE_A)
  {
    cache_print_stats(sys->icache, "ICACHE");
    cache_print_stats(sys->l2cache, "L2CACHE");
    dram_print_stats(sys->dram);
  }

}


uint64_t memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id)
{
  Flag needs_dcache_access = FALSE;
  Flag is_write = FALSE;

  if (type == ACCESS_TYPE_IFETCH)
  {
    // no icache in this mode
  }

  if (type == ACCESS_TYPE_LOAD)
  {
    needs_dcache_access = TRUE;
    is_write = FALSE;
  }

  if (type == ACCESS_TYPE_STORE)
  {
    needs_dcache_access = TRUE;
    is_write = TRUE;
  }

  if (needs_dcache_access)
  {
    Flag outcome = cache_access(sys->dcache, lineaddr, is_write, core_id);
    if (outcome == MISS)
    {
      cache_install(sys->dcache, lineaddr, is_write, core_id);
    }
  }

  // timing is not simulated in Part A
  return 0;
}