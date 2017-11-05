#ifndef CACHE_H
#define CACHE_H

#include "types.h"

#define MAX_WAYS 16

typedef struct Cache_Line Cache_Line;
typedef struct Cache_Set Cache_Set;
typedef struct Cache Cache;


struct Cache_Line
{
  Flag valid;
  Flag dirty;
  Addr tag;
  uint32_t core_id;
  uint64_t last_access_time; // for LRU
  // Note: No data as we are only estimating hit/miss
};


struct Cache_Set
{
  Cache_Line line[MAX_WAYS];
};


struct Cache
{
  uint64_t num_sets;
  uint64_t num_ways;
  uint64_t repl_policy;

  Cache_Set *sets;
  Cache_Line last_evicted_line; // for checking writebacks

  //stats
  uint64_t stat_read_access;
  uint64_t stat_write_access;
  uint64_t stat_read_miss;
  uint64_t stat_write_miss;
  uint64_t stat_dirty_evicts; // how many dirty lines were evicted?
};

Cache *cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
Flag cache_access(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_write_back_victim(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_print_stats(Cache *c, char *header);
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id);


uint32_t getSetId(Cache *c, uint64_t lineaddr);
uint64_t getTag(Cache *c, uint64_t lineaddr);
uint32_t getNumSetBits(uint64_t numOfSets);
bool isVictimDirty(Cache *c);
void flushVictim(Cache *c);

#endif // CACHE_H
