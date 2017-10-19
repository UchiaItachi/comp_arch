#include <stdio.h>
#include <assert.h>

#include "pipeline.h"
#include "rob.h"


// Init function initializes the ROB
ROB *ROB_init(void)
{
  int ii;
  ROB *t = new ROB();
  for (int i = 0; i < NUM_ROB_ENTRIES; ++i)
  {
    t->freeRobIds_.insert(i);
    t->ROB_Entries[i].valid = false;
    t->ROB_Entries[i].ready = false;
  }
  t->oldestInstruction.clear();
  return t;
}

// Print State
void ROB_print_state(ROB *t)
{
  int ii = 0;
  printf("Printing ROB \n");
  printf("Entry  Inst   Valid   ready\n");
  for (ii = 0; ii < NUM_ROB_ENTRIES; ii++)
  {
    printf("%5d ::  %d\t", ii, (int) t->ROB_Entries[ii].inst.inst_num);
    printf(" %5d\t", t->ROB_Entries[ii].valid);
    printf(" %5d\n", t->ROB_Entries[ii].ready);
  }
  printf("\n");
}


// If there is space in ROB return true, else false
bool ROB_check_space(ROB *t)
{
  return t->oldestInstruction.size() < NUM_ROB_ENTRIES;
}


// insert entry at tail, increment tail (do check_space first)
int ROB_insert(ROB *t, Inst_Info inst)
{
  if (!ROB_check_space(t))
  {
    utils::throw_exception("No space in ROB");
  }

  int robId = -1;
  if (t->oldestInstruction.find(inst.inst_num) == t->oldestInstruction.end())
  {
    robId = *t->freeRobIds_.begin();
    ROB_Entry& entry = t->ROB_Entries[robId];
    if (entry.valid)
    {
      utils::throw_exception("Expected invalid ROB entry during insert (check free pool logic)");
    }
    entry.inst = inst;
    entry.valid = true;
    entry.ready = false;
    entry.inst.dr_tag = robId;
    t->oldestInstruction[inst.inst_num] = robId;
    t->freeRobIds_.erase(t->freeRobIds_.begin());
  }
  else
  {
    utils::throw_exception("Duplicate rob id entries");
  }

  return robId;
}


// Once an instruction finishes execution, mark rob entry as done
void ROB_mark_ready(ROB *t, Inst_Info inst)
{
  if (t->oldestInstruction.find(inst.inst_num) == t->oldestInstruction.end())
  {
    utils::throw_exception("Instruction not present in ROB, to mark ready");
  }

  int robId = t->oldestInstruction.at(inst.inst_num);
  if (t->ROB_Entries[robId].valid)
  {
    t->ROB_Entries[robId].ready = true;
  }
  else
  {
    utils::throw_exception("Rob id invalid, to mark ready");
  }
}

// Find whether the prf-rob entry is ready
bool ROB_check_ready(ROB *t, int tag)
{
  bool ready = t->ROB_Entries[tag].ready;
  bool valid = t->ROB_Entries[tag].valid;
  if (!valid)
  {
    utils::throw_exception("ROB entry should be valid if there is a rob id (tag)");
  }
  return ready;
}

// Check if the oldest ROB entry is ready for commit
bool ROB_check_head(ROB *t)
{
  if (t->oldestInstruction.empty())
  {
    return false;
  }

  int robId = t->oldestInstruction.begin()->second;

  // return ROB_check_ready(t, robId);
  if (!t->ROB_Entries[robId].valid)
  {
    utils::throw_exception("Oldest ROB entry is invalid");
  }
  bool ready = t->ROB_Entries[robId].ready;
  return ready;
}

// Remove oldest entry from ROB (after ROB_check_head)
Inst_Info ROB_remove_head(ROB *t)
{
  int robId = t->oldestInstruction.begin()->second;
  Inst_Info info = t->ROB_Entries[robId].inst;

  t->ROB_Entries[robId].ready = false;
  t->ROB_Entries[robId].valid = false;

  t->oldestInstruction.erase(t->oldestInstruction.begin());
  t->freeRobIds_.insert(robId);

  return info;
}
