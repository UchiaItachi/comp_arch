#include <stdio.h>
#include <assert.h>

#include "rest.h"

extern int32_t NUM_REST_ENTRIES;

// Init function initializes the Reservation Station
REST *REST_init(void)
{
  int ii;
  REST *t = (REST *)calloc(1, sizeof(REST));
  for (ii = 0; ii < MAX_REST_ENTRIES; ii++)
  {
    t->REST_Entries[ii].valid = false;
  }
  assert(NUM_REST_ENTRIES <= MAX_REST_ENTRIES);
  return t;
}

// Print State
void REST_print_state(REST *t)
{
  int ii = 0;
  printf("Printing REST \n");
  printf("Entry  \t\tInst Num\tS1_tag\tS1_ready\tS2_tag\tS2_ready\t\tVld\t\tScheduled\n");
  for (ii = 0; ii < NUM_REST_ENTRIES; ii++)
  {
    printf("%5d ::  \t\t%d\t", ii, (int)t->REST_Entries[ii].inst.inst_num);
    printf("%5d\t\t", t->REST_Entries[ii].inst.src1_tag);
    printf("%5d\t\t", t->REST_Entries[ii].inst.src1_ready);
    printf("%5d\t\t", t->REST_Entries[ii].inst.src2_tag);
    printf("%5d\t\t", t->REST_Entries[ii].inst.src2_ready);
    printf("%5d\t\t", t->REST_Entries[ii].valid);
    printf("%5d\n", t->REST_Entries[ii].scheduled);
  }
  printf("\n");
}


// If space return true else return false
bool REST_check_space(REST *t)
{
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    if (!t->REST_Entries[i].valid)
    {
      t->REST_Entries[i].scheduled = false;
      return true;
    }
  }
  return false;
}

// Insert an inst in REST, must do check_space first
void REST_insert(REST *t, Inst_Info inst)
{
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    if (!t->REST_Entries[i].valid)
    {
      t->REST_Entries[i].valid = true;
      t->REST_Entries[i].scheduled = false;
      t->REST_Entries[i].inst = inst;
      return;
    }
  }
  utils::throw_exception("couldn't insert an entry into REST, missed calling REST_check_space() ? ");
}

// When instruction finishes execution, remove from REST
void REST_remove(REST *t, Inst_Info inst)
{
  uint32_t numOfEntriesRemoved = 0;
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    if (!t->REST_Entries[i].valid)
    {
      continue;
    }

    Inst_Info &info = t->REST_Entries[i].inst;
    if (info.inst_num == inst.inst_num)
    {
      t->REST_Entries[i].valid = false;
      t->REST_Entries[i].scheduled = false;
      info.src1_ready = false;
      info.src2_ready = false;
      numOfEntriesRemoved += 1;
    }
  }

  if (numOfEntriesRemoved == 0)
  {
    utils::throw_exception("couldn't remove an entry from REST");
  }
  else if (numOfEntriesRemoved > 1)
  {
    utils::throw_exception("duplicate entries found in REST");
  }
}

// For broadcast of freshly ready tags, wakeup waiting inst
void REST_wakeup(REST *t, int tag)
{
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    if (!t->REST_Entries[i].valid)
    {
      continue;
    }

    Inst_Info &info = t->REST_Entries[i].inst;
    if ((info.src1_reg != -1) && (info.src1_tag == tag))
    {
      info.src1_ready = true;
    }

    if ((info.src2_reg != -1) && (info.src2_tag == tag))
    {
      info.src2_ready = true;
    }
  }
}

// When an instruction gets scheduled, mark REST entry as such
void REST_schedule(REST *t, Inst_Info inst)
{
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    Inst_Info &info = t->REST_Entries[i].inst;
    if (t->REST_Entries[i].valid &&
      (info.inst_num == inst.inst_num))
    {
      t->REST_Entries[i].scheduled = true;
    }
  }
}
