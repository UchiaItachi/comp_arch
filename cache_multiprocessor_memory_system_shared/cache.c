#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cache.h"


extern uns64 SWP_CORE0_WAYS; // Input Way partitions for Core 0       
extern uns64 cycle; // You can use this as timestamp for LRU

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy)
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

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

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



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise 
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////

uns32 getNumSetBits(uns64 numOfSets)
{
  uns32 numOfSetBits = log2(numOfSets);
  return numOfSetBits;
}

uns32 getSetId(Cache *c, uns64 lineaddr)
{
  const uns32 num_of_set_bits = getNumSetBits(c->num_sets);
  const uns64 set_mask = (0x1 << num_of_set_bits) - 1;
  const uns32 set_no = lineaddr & set_mask;

  return set_no;
}

uns64 getTag(Cache *c, uns64 lineaddr)
{
  const uns32 numOfSetBits = getNumSetBits(c->num_sets);
  const uns64 tag = lineaddr >> numOfSetBits;
  return tag;
}


// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
Flag cache_access(Cache *c, Addr lineaddr, uns32 is_write, uns32 core_id)
{
  Flag outcome = MISS;
  const uns32 set_num = getSetId(c, lineaddr);
  const uns64 tag = getTag(c, lineaddr);
  int32 hitWay = -1;

  Cache_Set* set = &c->sets[set_num];
  uns32 way = 0;
  for (way = 0; way < c->num_ways; ++way)
  {
    if (set->line[way].valid && (set->line[way].tag == tag))
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
  }

  if (HIT == outcome)
  {
    set->line[hitWay].core_id = core_id;
    set->line[hitWay].last_access_time = cycle;
    set->line[hitWay].valid = TRUE;
    set->line[hitWay].tag = tag;
    if (is_write)
    {
      set->line[hitWay].dirty = TRUE;
    }
  }

  return outcome;
}


// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
void cache_install(Cache *c, Addr lineaddr, uns32 is_write, uns32 core_id)
{
  // Your Code Goes Here
  // Find victim using cache_find_victim
  // Initialize the evicted entry
  // Initialize the victime entry
  const uns32 set_num = getSetId(c, lineaddr);
  const uns64 tag = getTag(c, lineaddr);


  uns32 victimWay = cache_find_victim(c, set_num, core_id);
  Cache_Line* victimLine = &c->sets[set_num].line[victimWay];

  if (TRUE == victimLine->dirty)
  {
    c->stat_dirty_evicts += 1;
  }

  c->last_evicted_line = *victimLine;
  c->last_evicted_line.tag = (victimLine->tag << getNumSetBits(c->num_sets)) | set_num;

  victimLine->valid = TRUE;
  victimLine->dirty = is_write ? TRUE : FALSE;
  victimLine->tag = tag;
  victimLine->core_id = core_id;
  victimLine->last_access_time = cycle;
}


// You may find it useful to split victim selection from install
uns32 cache_find_victim(Cache *c, uns32 set_index, uns32 core_id)
{
  uns32 victim = 0;
  Cache_Set* set = &c->sets[set_index];

  switch (c->repl_policy)
  {
    case 0:
    {
      uns64 leastCycleTime = cycle;
      int32 lruWay = -1;
      uns32 way = 0;
      for (way = 0; way < c->num_ways; ++way)
      {
        if (set->line[way].last_access_time < leastCycleTime)
        {
          leastCycleTime = set->line[way].last_access_time;
          lruWay = way;
        }
      }
      victim = lruWay;
    }
      break;

    case 1:
    {
      victim = rand() % c->num_ways;
    }
      break;

    default:
      break;
  }
  return victim;
}