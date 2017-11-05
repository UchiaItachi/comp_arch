#include "cache.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <algorithm>

extern uint64_t cycle; // You can use this as timestamp for LRU

// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
Cache *cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy)
{

  Cache *c = (Cache *) calloc(1, sizeof(Cache));
  c->num_ways = assoc;
  c->repl_policy = repl_policy;

  if (c->num_ways > MAX_WAYS)
  {
    printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
    exit(-1);
  }

  // determine num sets, and init the cache
  c->num_sets = size / (linesize * assoc);
  c->sets = (Cache_Set *) calloc(c->num_sets, sizeof(Cache_Set));

  return c;
}

// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
void cache_print_stats(Cache *c, char *header)
{
  double read_mr = 0;
  double write_mr = 0;

  if (c->stat_read_access)
  {
    read_mr = (double) (c->stat_read_miss) / (double) (c->stat_read_access);
  }

  if (c->stat_write_access)
  {
    write_mr = (double) (c->stat_write_miss) / (double) (c->stat_write_access);
  }

  printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
  printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
  printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
  printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
  printf("\n%s_READ_MISSPERC  \t\t : %10.3f", header, 100 * read_mr);
  printf("\n%s_WRITE_MISSPERC \t\t : %10.3f", header, 100 * write_mr);
  printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

  printf("\n");
}

// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise 
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
Flag cache_access(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id)
{
  Flag outcome = MISS;
  const uint32_t set_num = getSetId(c, lineaddr);
  const uint64_t tag = getTag(c, lineaddr);
  int32_t hitWay = -1;

  Cache_Set& set = *(&c->sets[set_num]);
  for (uint32_t way = 0; way < c->num_ways; ++way)
  {
    if (set.line[way].valid && (set.line[way].tag == tag) && (set.line[way].core_id == core_id))
    {
      hitWay = way;
      outcome = HIT;
      break;
    }
  }

  if (is_write)
  {
    c->stat_write_access += 1;
  }
  else
  {
    c->stat_read_access += 1;
  }


  if (MISS == outcome)
  {
    if (is_write)
    {
      c->stat_write_miss += 1;
    }
    else
    {
      c->stat_read_miss += 1;
    }
    if (hitWay != -1)
    {
      throw std::exception();
    }
  }

  if (HIT == outcome)
  {
    if (hitWay == -1)
    {
      throw std::exception();
    }

    set.line[hitWay].core_id = core_id;
    set.line[hitWay].last_access_time = cycle;
    set.line[hitWay].valid = TRUE;
    set.line[hitWay].tag = tag;
    if (is_write)
    {
      set.line[hitWay].dirty = true;
    }
  }

  return outcome;
}


// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
void cache_install(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id)
{
  // Your Code Goes Here
  // Find victim using cache_find_victim
  // Initialize the evicted entry
  // Initialize the victime entry
  const uint32_t set_num = getSetId(c, lineaddr);
  const uint64_t tag = getTag(c, lineaddr);


  uint32_t victimWay = cache_find_victim(c, set_num, core_id);
  Cache_Line& victimLine = c->sets[set_num].line[victimWay];

  if (TRUE == victimLine.dirty)
  {
    c->stat_dirty_evicts += 1;
  }

  c->last_evicted_line = victimLine;
  c->last_evicted_line.tag = (victimLine.tag << getNumSetBits(c->num_sets)) | set_num;

  victimLine.valid = TRUE;
  victimLine.dirty = is_write ? TRUE : FALSE;
  victimLine.tag = tag;
  victimLine.core_id = core_id;
  victimLine.last_access_time = cycle;
}


// You may find it useful to split victim selection from install
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id)
{
  uint32_t victim = 0;
  Cache_Set& set = *(&c->sets[set_index]);

  switch (c->repl_policy)
  {
    case 0:
    {
      uint64_t leastCycleTime = cycle;
      int32_t lruWay = -1;
      for (uint32_t way = 0; way < c->num_ways; ++way)
      {
        if (set.line[way].last_access_time < leastCycleTime)
        {
          leastCycleTime = set.line[way].last_access_time;
          lruWay = way;
        }
      }

      if (-1 == lruWay)
      {
        throw std::exception();
      }

      victim = lruWay;
    }
    break;

    case 1:
    {
      victim = 0;
    }
    break;

    default:
      throw std::exception();
  }
  return victim;
}

void cache_write_back_victim(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id)
{
  cache_install(c, lineaddr, is_write, core_id);
}

uint32_t getSetId(Cache *c, uint64_t lineaddr)
{
  const uint32_t num_of_set_bits = getNumSetBits(c->num_sets);
  const uint64_t set_mask = (0x1 << num_of_set_bits) - 1;
  const uint32_t set_no = lineaddr & set_mask;

  return set_no;
}

uint64_t getTag(Cache *c, uint64_t lineaddr)
{
  const uint32_t numOfSetBits = getNumSetBits(c->num_sets);
  const uint64_t tag = lineaddr >> numOfSetBits;
  return tag;
}

uint32_t getNumSetBits(uint64_t numOfSets)
{
  uint32_t numOfSetBits = std::ceil(std::log2(numOfSets));
  return numOfSetBits;
}

bool isVictimDirty(Cache *c)
{
  bool isDirty = (TRUE == c->last_evicted_line.dirty);
  bool isValid = (TRUE == c->last_evicted_line.valid);

  return isDirty && isValid;
}

void flushVictim(Cache *c)
{
  c->last_evicted_line.valid = FALSE;
  c->last_evicted_line.dirty = FALSE;
}