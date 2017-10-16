/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Moinuddin K. Qureshi
 * Date         : 19th February 2014
 * Description  : Out of Order Pipeline for Lab3 ECE 6100
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>
#include <cstring>


extern int32_t PIPE_WIDTH;
extern int32_t SCHED_POLICY;
extern int32_t LOAD_EXE_CYCLES;
extern int32_t NUM_REST_ENTRIES;

/**********************************************************************
 * Support Function: Read 1 Trace Record From File and populate Fetch Inst
 **********************************************************************/

void pipe_fetch_inst(Pipeline *p, Pipe_Latch *fe_latch)
{
  static int halt_fetch = 0;
  uint8_t bytes_read = 0;
  Trace_Rec trace;
  if (halt_fetch != 1)
  {
    bytes_read = fread(&trace, 1, sizeof(Trace_Rec), p->tr_file);
    Inst_Info *fetch_inst = &(fe_latch->inst);
    // check for end of trace
    // Send out a dummy terminate op
    if (bytes_read < sizeof(Trace_Rec))
    {
      p->halt_inst_num = p->inst_num_tracker;
      halt_fetch = 1;
      fe_latch->valid = true;
      fe_latch->inst.dest_reg = -1;
      fe_latch->inst.src1_reg = -1;
      fe_latch->inst.src1_reg = -1;
      fe_latch->inst.inst_num = -1;
      fe_latch->inst.op_type = 4;
      return;
    }

    // got an instruction ... hooray!
    fe_latch->valid = true;
    fe_latch->stall = false;
    p->inst_num_tracker++;
    fetch_inst->inst_num = p->inst_num_tracker;
    fetch_inst->op_type = trace.op_type;

    fetch_inst->dest_reg = trace.dest_needed ? trace.dest : -1;
    fetch_inst->src1_reg = trace.src1_needed ? trace.src1_reg : -1;
    fetch_inst->src2_reg = trace.src2_needed ? trace.src2_reg : -1;

    fetch_inst->dr_tag = -1;
    fetch_inst->src1_tag = -1;
    fetch_inst->src2_tag = -1;
    fetch_inst->src1_ready = false;
    fetch_inst->src2_ready = false;
    fetch_inst->exe_wait_cycles = 0;
  }
  else
  {
    fe_latch->valid = false;
  }
  return;
}


int32_t getOldestSchedulableRestIndex(Pipeline *p)
{
  uint64_t oldestInsAddress = p->pipe_REST->REST_Entries[0].inst.inst_num;
  int32_t oldestSchedulableRestEntry = -1;
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    REST_Entry &rest_entry = p->pipe_REST->REST_Entries[i];
    if (rest_entry.valid && (!rest_entry.scheduled))
    {
      if (rest_entry.inst.inst_num <= oldestInsAddress)
      {
        oldestSchedulableRestEntry = i;
        oldestInsAddress = rest_entry.inst.inst_num;
      }
    }
  }
  return oldestSchedulableRestEntry;
}

int32_t getFreeSCLatchIndex(Pipeline *p)
{
  for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if ((!p->SC_latch[way].valid) && (!p->SC_latch[way].stall))
    {
      return way;
    }
  }
  return -1;
}

/**********************************************************************
 * Pipeline Class Member Functions 
 **********************************************************************/

Pipeline *pipe_init(FILE *tr_file_in)
{
  printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

  // Initialize Pipeline Internals
  Pipeline *p = (Pipeline *) calloc(1, sizeof(Pipeline));

  p->pipe_RAT = RAT_init();
  p->pipe_ROB = ROB_init();
  p->pipe_REST = REST_init();
  p->pipe_EXEQ = EXEQ_init();
  p->tr_file = tr_file_in;
  p->halt_inst_num = ((uint64_t) -1) - 3;
  int ii = 0;
  for (ii = 0; ii < PIPE_WIDTH; ii++)
  {  // Loop over No of Pipes
    p->FE_latch[ii].valid = false;
    p->ID_latch[ii].valid = false;
    p->EX_latch[ii].valid = false;
    p->SC_latch[ii].valid = false;
  }
  return p;
}


/**********************************************************************
 * Print the pipeline state (useful for debugging)
 **********************************************************************/

void pipe_print_state(Pipeline *p)
{
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "cycle count : " << p->stat_num_cycle << " retired_instruction : " << p->stat_retired_inst << std::endl;
  uint8_t latch_type_i = 0;
  uint8_t width_i = 0;
  for (latch_type_i = 0; latch_type_i < 4; latch_type_i++)
  {
    switch (latch_type_i)
    {
      case 0:
        printf(" FE: ");
        break;
      case 1:
        printf(" ID: ");
        break;
      case 2:
        printf(" SCH: ");
        break;
      case 3:
        printf(" EX: ");
        break;
      default:
        printf(" -- ");
    }
  }
  printf("\n");
  for (width_i = 0; width_i < PIPE_WIDTH; width_i++)
  {
    if (p->FE_latch[width_i].valid == true)
    {
      printf("  %d  ", (int) p->FE_latch[width_i].inst.inst_num);
    }
    else
    {
      printf(" --  ");
    }
    if (p->ID_latch[width_i].valid == true)
    {
      printf("  %d  ", (int) p->ID_latch[width_i].inst.inst_num);
    }
    else
    {
      printf(" --  ");
    }
    if (p->SC_latch[width_i].valid == true)
    {
      printf("  %d  ", (int) p->SC_latch[width_i].inst.inst_num);
    }
    else
    {
      printf(" --  ");
    }
    if (p->EX_latch[width_i].valid == true)
    {
      for (int ii = 0; ii < MAX_BROADCASTS; ii++)
      {
        if (p->EX_latch[ii].valid)
          printf("  %d  ", (int) p->EX_latch[ii].inst.inst_num);
      }
    }
    else
    {
      printf(" --  ");
    }
    printf("\n");
  }
  printf("\n");

  RAT_print_state(p->pipe_RAT);
  REST_print_state(p->pipe_REST);
  EXEQ_print_state(p->pipe_EXEQ);
  ROB_print_state(p->pipe_ROB);
}


/**********************************************************************
 * Pipeline Main Function: Every cycle, cycle the stage 
 **********************************************************************/

void pipe_cycle(Pipeline *p)
{
  p->stat_num_cycle++;

  pipe_cycle_commit(p);
  pipe_cycle_broadcast(p);
  pipe_cycle_exe(p);
  pipe_cycle_schedule(p);
  pipe_cycle_rename(p);
  pipe_cycle_decode(p);
  pipe_cycle_fetch(p);

  //pipe_print_state(p);
}

void pipe_cycle_fetch(Pipeline *p)
{
  int ii = 0;
  Pipe_Latch fetch_latch;

  for (ii = 0; ii < PIPE_WIDTH; ii++)
  {
    if ((p->FE_latch[ii].stall) || (p->FE_latch[ii].valid))
    {   // Stall
      continue;

    }
    else
    {  // No Stall and Latch Empty
      pipe_fetch_inst(p, &fetch_latch);
      // copy the op in FE LATCH
      p->FE_latch[ii] = fetch_latch;
    }
  }
}

void pipe_cycle_decode(Pipeline *p)
{
  int ii = 0;

  int jj = 0;

  static uint64_t start_inst_id = 1;

  // Loop Over ID Latch
  for (ii = 0; ii < PIPE_WIDTH; ii++)
  {
    if ((p->ID_latch[ii].stall == 1) || (p->ID_latch[ii].valid))
    { // Stall
      continue;
    }
    else
    {  // No Stall & there is Space in Latch
      for (jj = 0; jj < PIPE_WIDTH; jj++)
      { // Loop Over FE Latch
        if (p->FE_latch[jj].valid)
        {
          if (p->FE_latch[jj].inst.inst_num == start_inst_id)
          { // In Order Inst Found
            p->ID_latch[ii] = p->FE_latch[jj];
            p->ID_latch[ii].valid = true;
            p->FE_latch[jj].valid = false;
            start_inst_id++;
            break;
          }
        }
      }
    }
  }

}

void pipe_cycle_exe(Pipeline *p)
{
  int ii;
  //If all operations are single cycle, simply copy SC latches to EX latches
  if (LOAD_EXE_CYCLES == 1)
  {
    for (ii = 0; ii < PIPE_WIDTH; ii++)
    {
      if (p->SC_latch[ii].valid)
      {
        p->EX_latch[ii] = p->SC_latch[ii];
        p->EX_latch[ii].valid = true;
        p->SC_latch[ii].valid = false;
      }
    }
    return; // FIXED:
  }

  //---------Handling exe for multicycle operations is complex, and uses EXEQ

  // All valid entries from SC get into exeq  

  for (ii = 0; ii < PIPE_WIDTH; ii++)
  {
    if (p->SC_latch[ii].valid)
    {
      EXEQ_insert(p->pipe_EXEQ, p->SC_latch[ii].inst);
      p->SC_latch[ii].valid = false;
    }
  }

  // Cycle the exeq, to reduce wait time for each inst by 1 cycle
  EXEQ_cycle(p->pipe_EXEQ);

  // Transfer all finished entries from EXEQ to EX_latch
  int index = 0;

  while (1)
  {
    if (EXEQ_check_done(p->pipe_EXEQ))
    {
      if (index >= PIPE_WIDTH)
      {
        //utils::throw_exception("number of execution queues cannot be greater than execution latches");
      }
      p->EX_latch[index].valid = true;
      p->EX_latch[index].stall = false;
      p->EX_latch[index].inst = EXEQ_remove(p->pipe_EXEQ);
      index++;
    }
    else
    { // No More Entry in EXEQ
      break;
    }
  }
}

void pipe_cycle_rename(Pipeline *p)
{
  // TODO: If src1/src2 is remapped set src1tag, src2tag
  // TODO: Find space in ROB and set drtag as such if successful
  // TODO: Find space in REST and transfer this inst (valid=1, sched=0)
  // TODO: If src1/src2 is not remapped marked as src ready
  // TODO: If src1/src2 remapped and the ROB (tag) is ready then mark src ready
  // FIXME: If there is stall, we should not do rename and ROB alloc twice
  bool isRobEntryAvailable = false;
  bool isRestEntryAvailable = false;

  for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (!p->ID_latch[way].valid)
    {
      continue;
    }
    Inst_Info &inst = p->ID_latch[way].inst;
    if (inst.dest_reg == -1)
    {
      inst.dr_tag = -1;
      //utils::throw_exception("number of execution queues cannot be greater than execution latches");
    }

    isRobEntryAvailable = ROB_check_space(p->pipe_ROB);
    isRestEntryAvailable = REST_check_space(p->pipe_REST);

    if ((!isRestEntryAvailable) || (!isRobEntryAvailable))
    {
      p->ID_latch[way].stall = true;
      p->FE_latch[way].stall = true;
      continue;
    }

    if (inst.src1_reg != -1)
    {
      bool isSrc1Remapped = RAT_get_remap(p->pipe_RAT, inst.src1_reg) != -1;
      if (isSrc1Remapped)
      {
        inst.src1_tag = RAT_get_remap(p->pipe_RAT, inst.src1_reg);
        inst.src1_ready = ROB_check_ready(p->pipe_ROB, inst.src1_tag);
      }
      else
      {
        inst.src1_tag = -1;
        inst.src1_ready = true;
      }
    }

    if (inst.src2_reg != -1)
    {
      bool isSrc2Remapped = RAT_get_remap(p->pipe_RAT, inst.src2_reg) != -1;
      if (isSrc2Remapped)
      {
        inst.src2_tag = RAT_get_remap(p->pipe_RAT, inst.src2_reg);
        inst.src2_ready = ROB_check_ready(p->pipe_ROB, inst.src2_tag);
      }
      else
      {
        inst.src2_tag = -1;
        inst.src2_ready = true;
      }
    }

    if (inst.dest_reg != -1)
    {
      inst.dr_tag = ROB_insert(p->pipe_ROB, inst);
      RAT_set_remap(p->pipe_RAT, inst.dest_reg, inst.dr_tag);
    }
    else
    {
      inst.dr_tag = -1;
    }
    REST_insert(p->pipe_REST, inst);
    p->ID_latch[way].valid = false;
  }
}

void pipe_cycle_schedule(Pipeline *p)
{
  // TODO: Implement two scheduling policies (SCHED_POLICY: 0 and 1)
  if (SCHED_POLICY == 0)
  {
    // inorder scheduling
    // Find all valid entries, if oldest is stalled then stop
    // Else send it out and mark it as scheduled
    for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
    {
      if (p->SC_latch[way].valid || p->SC_latch[way].stall)
      {
        continue;
      }

      int32_t restIndex = getOldestSchedulableRestIndex(p);
      if (restIndex == -1)
      {
        utils::throw_exception("no oldtest schedulable instruction found in REST");
      }

      REST_Entry &rest_entry = p->pipe_REST->REST_Entries[restIndex];
      if (rest_entry.valid && (!rest_entry.scheduled))
      {
        bool src1Ready = (rest_entry.inst.src1_reg != -1) && rest_entry.inst.src1_ready;
        bool src2Ready = (rest_entry.inst.src2_reg != -1) && rest_entry.inst.src2_ready;
        if (src1Ready && src2Ready)
        {
          rest_entry.scheduled = true;
          p->SC_latch[way].valid = true;
          p->SC_latch[way].stall = false;
          p->SC_latch[way].inst = rest_entry.inst;
        }
        else
        {
          p->SC_latch[way].stall = true;
        }
      }
      else
      {
        utils::throw_exception("unexpected behavior checkpoint");
      }
    }
    return;
  }

  if (SCHED_POLICY == 1)
  {
    // out of order scheduling
    // Find valid/unscheduled/src1ready/src2ready entries in REST
    // Transfer them to SC_latch and mark that REST entry as scheduled
    for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
    {
      if (p->SC_latch[way].valid || p->SC_latch[way].stall)
      {
        continue;
      }

      int32_t restIndex = getOldestSchedulableRestIndex(p);
      if (restIndex == -1)
      {
        continue;
      }
      int32_t scLatchSlot = getFreeSCLatchIndex(p);
      if (scLatchSlot == -1)
      {
        continue;
      }

      REST_Entry &rest_entry = p->pipe_REST->REST_Entries[restIndex];
      if (rest_entry.valid && (!rest_entry.scheduled))
      {
        Inst_Info& info = rest_entry.inst;
        bool src1Ready = (info.src1_reg == -1) ? true : info.src1_ready;
        bool src2Ready = (info.src2_reg == -1) ? true : info.src2_ready;
        if (src1Ready && src2Ready)
        {
          int32_t scLatchSlot = getFreeSCLatchIndex(p);
          rest_entry.scheduled = true;
          p->SC_latch[scLatchSlot].valid = true;
          p->SC_latch[way].stall = false;
          p->SC_latch[scLatchSlot].inst = rest_entry.inst;
        }
      }
      else
      {
        utils::throw_exception("unexpected checkpoint");
      }
    }
  }
}

void pipe_cycle_broadcast(Pipeline *p)
{
  // TODO: Go through all instructions out of EXE latch
  // TODO: Broadcast it to REST (using wakeup function)
  // TODO: Remove entry from REST (using inst_num)
  // TODO: Update the ROB, mark ready, and update Inst Info in ROB
  for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
  {

    if (!p->EX_latch[way].valid)
    {
      continue;
    }
    Inst_Info &info = p->EX_latch[way].inst;
    if (info.dest_reg != -1)
    {
      REST_wakeup(p->pipe_REST, info.dr_tag);
    }
    REST_remove(p->pipe_REST, info);
    ROB_mark_ready(p->pipe_ROB, info);
  }
}

void pipe_cycle_commit(Pipeline *p)
{
  // TODO: check the head of the ROB. If ready commit (update stats)
  // TODO: Deallocate entry from ROB
  // TODO: Update RAT after checking if the mapping is still valid
  for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (ROB_check_head(p->pipe_ROB))
    {
      Inst_Info info = ROB_remove_head(p->pipe_ROB);
      p->stat_retired_inst++;
      if (info.dest_reg != -1)
      {
        int phyReg = RAT_get_remap(p->pipe_RAT, info.dest_reg);
        if ((phyReg != -1) && (info.dr_tag == phyReg))
        {
          RAT_reset_entry(p->pipe_RAT, info.dest_reg);
        }
      }
    }

    if (p->FE_latch[way].valid)
    {
      if (p->FE_latch[way].inst.inst_num >= p->halt_inst_num)
      {
        p->halt = true;
      }
    }
  }
}