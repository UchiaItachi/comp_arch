#include <stdio.h>
#include <assert.h>

#include "rob.h"


extern int32_t NUM_ROB_ENTRIES;

// Init function initializes the ROB
ROB *ROB_init(void)
{
  int ii;
  ROB *t = (ROB *) calloc(1, sizeof(ROB));
  for (ii = 0; ii < MAX_ROB_ENTRIES; ii++)
  {
    t->ROB_Entries[ii].valid = false;
    t->ROB_Entries[ii].ready = false;
  }
  t->head_ptr = 0;
  t->tail_ptr = 0;
  return t;
}


// Print State
void ROB_print_state(ROB *t)
{
  int ii = 0;
  printf("Printing ROB \n");
  printf("Entry  Inst   Valid   ready\n");
  for (ii = 0; ii < 7; ii++)
  {
    printf("%5d ::  %d\t", ii, (int) t->ROB_Entries[ii].inst.inst_num);
    printf(" %5d\t", t->ROB_Entries[ii].valid);
    printf(" %5d\n", t->ROB_Entries[ii].ready);
  }
  printf("\n");
}


//------- DO NOT CHANGE THE CODE ABOVE THIS LINE -----------
// If there is space in ROB return true, else false
bool ROB_check_space(ROB *t)
{
  if ((t->tail_ptr < NUM_ROB_ENTRIES) || (t->head_ptr > 0))
  {
    return true;
  }
  return false;
}


// insert entry at tail, increment tail (do check_space first)
int ROB_insert(ROB *t, Inst_Info inst)
{
  if (!ROB_check_space(t))
  {
    utils::throw_exception("Trying to insert in ROB when ROB is full");
  }

  t->ROB_Entries[t->tail_ptr].inst = inst;
  t->ROB_Entries[t->tail_ptr].valid = true;
  int robId = t->tail_ptr++;
  return robId;
}


// Once an instruction finishes execution, mark rob entry as done
void ROB_mark_ready(ROB *t, Inst_Info inst)
{
  for (int i = t->head_ptr; i < t->tail_ptr; ++i)
  {
    if (!t->ROB_Entries[i].valid)
    {
      continue;
    }

    Inst_Info &info = t->ROB_Entries[i].inst;
    if (info.inst_num == inst.inst_num)
    {
      t->ROB_Entries[i].ready = true;
    }
  }
}


// Find whether the prf-rob entry is ready
bool ROB_check_ready(ROB *t, int tag)
{
  bool ready = t->ROB_Entries[tag].ready;
  bool valid = t->ROB_Entries[tag].valid;
  return ready && valid;
}

// Check if the oldest ROB entry is ready for commit
bool ROB_check_head(ROB *t)
{
  return ROB_check_ready(t, t->head_ptr);
}


// Remove oldest entry from ROB (after ROB_check_head)
Inst_Info ROB_remove_head(ROB *t)
{
  Inst_Info info = t->ROB_Entries[t->head_ptr].inst;
  t->ROB_Entries[t->head_ptr].valid = false;
  t->ROB_Entries[t->head_ptr].ready = false;


  t->head_ptr += 1;

  return info;
}



