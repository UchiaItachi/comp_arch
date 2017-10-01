/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Soham J. Desai
 * Date         : 14th January 2014
 * Description  : Superscalar Pipeline for Lab2 ECE 6100
 **********************************************************************/

#include "pipeline.h"

#include <cstdlib>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>

extern int32_t PIPE_WIDTH;
extern int32_t ENABLE_MEM_FWD;
extern int32_t ENABLE_EXE_FWD;
extern int32_t BPRED_POLICY;

uint64_t resume_op_id = 0;
uint64_t resume_op_id_mispred_branch = 0;
uint64_t retired_op_id = 0;
uint64_t youngest_blocking_op_id = 0;
bool isStallDueToMisprediction = false;
bool startPrintAfterAMisprediction = false;

typedef uint32_t Way_t;
std::map<Way_t, bool> fetchStalledDueToMisspred;

void stall_decode_stage(Pipeline *p, uint32_t way);
void stall_fetch_stage(Pipeline *p, uint32_t way);

void insert_bubble_decode_stage(Pipeline *p, uint32_t way);
void insert_bubble_fetch_stage(Pipeline *p, uint32_t way);

void resume_decode_stage_stall(Pipeline *p, uint32_t way);
void resume_fetch_stage_stall(Pipeline *p, uint32_t way);

bool is_decode_stalled(Pipeline *p, uint32_t way);
bool is_fetch_stalled(Pipeline *p, uint32_t way);

bool isForwardingEnabled();
bool can_stall_n_way(uint64_t decode_stage_op_id);
bool can_resume_n_ways(Pipeline* p);
bool can_stall_mem_stage_dependencies(Trace_Rec *mem_stage_ins, Trace_Rec *decode_stage_ins);
bool can_stall_exec_stage_dependencies(Trace_Rec *execute_stage_ins, Trace_Rec *decode_stage_ins);
bool can_stall_decode_stage_dependencies(Trace_Rec *decode_stage_n_way_ins, Trace_Rec *decode_stage_current_ins);
bool skipExecStageChecksOnDestinationMatch(Trace_Rec *oldest_exec_stage_ins, Trace_Rec* decode_stage_ins);
void updateBlockingAndResumeOpId(uint64_t blockingOpid, uint64_t resumeOpId);
void resumeControlDependencies(Pipeline *p);
void analyzeDependencies(Pipeline *p);
/**********************************************************************
 * Support Function: Read 1 Trace Record From File and populate Fetch Op
 **********************************************************************/

void perform_fetch(Pipeline *p, uint32_t current_way, Pipeline_Latch op)
{
  p->pipe_latch[FE_LATCH][current_way] = op;
}

void perform_decode(Pipeline *p, uint32_t current_way)
{
  p->pipe_latch[ID_LATCH][current_way] = p->pipe_latch[FE_LATCH][current_way];
  p->pipe_latch[FE_LATCH][current_way].valid = false;
}

void perform_execute(Pipeline *p, uint32_t current_way)
{
  p->pipe_latch[EX_LATCH][current_way] = p->pipe_latch[ID_LATCH][current_way];
  p->pipe_latch[ID_LATCH][current_way].valid = false;
}

void perform_mem(Pipeline *p, uint32_t current_way)
{
  p->pipe_latch[MEM_LATCH][current_way] = p->pipe_latch[EX_LATCH][current_way];
  p->pipe_latch[EX_LATCH][current_way].valid = false;
}

void perform_writeback(Pipeline *p, uint32_t current_way)
{
  //p->pipe_latch[MEM_LATCH][current_way];
}

void pipe_get_fetch_op(Pipeline *p, Pipeline_Latch* fetch_op)
{
    uint8_t bytes_read = 0;
  bytes_read = fread(&fetch_op->tr_entry, 1, sizeof(Trace_Rec), p->tr_file);
  // check for end of trace
  if (bytes_read < sizeof(Trace_Rec))
  {
    fetch_op->valid = false;
    fetch_op->op_id = 0;
    fetch_op->is_mispred_cbr = false;
    fetch_op->stall = false;
    p->halt_op_id = p->op_id_tracker;
    return;
  }
  // got an instruction ... hooray!
  fetch_op->valid = true;
  fetch_op->stall = false;
  fetch_op->is_mispred_cbr = false;
  p->op_id_tracker++;
  fetch_op->op_id = p->op_id_tracker;

  //printf("\n Instruction Address = %lu, op_id = %lu, type = %d", fetch_op->tr_entry.inst_addr, fetch_op->op_id, fetch_op->tr_entry.op_type);
  return;
}

/**********************************************************************
 * Pipeline Class Member Functions
 **********************************************************************/

Pipeline* pipe_init(FILE *tr_file_in)
{
  printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

  // Initialize Pipeline Internals
  Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));
  
  p->tr_file = tr_file_in;
  p->halt_op_id = ((uint64_t)-1) - 3;

  // Allocated Branch Predictor
  if (BPRED_POLICY)
  {
    p->b_pred = new BPRED(BPRED_POLICY);
  }

  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    fetchStalledDueToMisspred[way] = false;
  }
  return p;
}

void pipe_print_state(Pipeline *p)
{
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "cycle count : " << p->stat_num_cycle << " retired_instruction : " << p->stat_retired_inst << std::endl;

    uint8_t latch_type_i = 0;   // Iterates over Latch Types
    uint8_t width_i      = 0;   // Iterates over Pipeline Width
    for(latch_type_i = 0; latch_type_i < NUM_LATCH_TYPES; latch_type_i++) 
    {
      switch(latch_type_i) 
      {
        case 0:
          std::cout << std::left << std::setfill(' ') << std::setw(10) << "FE ";
          break;
        case 1:
          std::cout << std::left << std::setfill(' ') << std::setw(10) << "ID ";
            break;
        case 2:
          std::cout << std::left << std::setfill(' ') << std::setw(10) << "EX ";
            break;
        case 3:
          std::cout << std::left << std::setfill(' ') << std::setw(10) << "MEM ";
            break;
        default:
            printf(" ---- ");
      }
    }
    printf("\n");
    for(width_i = 0; width_i < PIPE_WIDTH; width_i++) 
    {
      for(latch_type_i = 0; latch_type_i < NUM_LATCH_TYPES; latch_type_i++) 
      {
          if(p->pipe_latch[latch_type_i][width_i].valid == true) 
          {
            std::cout << std::left << std::setfill(' ') << std::setw(10) << p->pipe_latch[latch_type_i][width_i].op_id;
          } 
          else 
          {
            std::cout << std::left << std::setfill(' ') << std::setw(10) << "----";
          }
      }
      printf("\n");
    }
    printf("\n");
}


/**********************************************************************
 * Pipeline Main Function: Every cycle, cycle the stage 
 **********************************************************************/

void pipe_cycle(Pipeline *p)
{
  p->stat_num_cycle++;

  pipe_cycle_WB(p);
  pipe_cycle_MEM(p);
  pipe_cycle_EX(p);
  pipe_cycle_ID(p);
  pipe_cycle_FE(p);

  analyzeDependencies(p);
  //pipe_print_state(p);
}

void pipe_cycle_WB(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; way++)
  {
    if (p->pipe_latch[MEM_LATCH][way].valid)
    {
      p->stat_retired_inst++;
      if (p->pipe_latch[MEM_LATCH][way].op_id >= p->halt_op_id)
      {
        p->halt = true;
      }
    }
  }
}

void pipe_cycle_MEM(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; way++)
  {
    perform_mem(p, way);
    if (p->pipe_latch[MEM_LATCH][way].valid)
    {
      if (retired_op_id < p->pipe_latch[MEM_LATCH][way].op_id)
      {
        retired_op_id = p->pipe_latch[MEM_LATCH][way].op_id;
      }
    }
  }
}

void pipe_cycle_EX(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; way++)
  {
    perform_execute(p, way);
  }
}

void pipe_cycle_ID(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; way++)
  {
    if (!is_decode_stalled(p, way))
    {
      perform_decode(p, way);
    }
    else
    {
      insert_bubble_decode_stage(p, way);
    }
  }
}

void pipe_cycle_FE(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; way++)
  {
    Pipeline_Latch fetch_op;
   
    if (isStallDueToMisprediction && BPRED_POLICY)
    {
      stall_fetch_stage(p, way);
    }

    if (!is_fetch_stalled(p, way))
    {
      pipe_get_fetch_op(p, &fetch_op);
    }

    if (BPRED_POLICY && (!p->pipe_latch[FE_LATCH][way].stall))
    {
      pipe_check_bpred(p, &fetch_op);
    }

    if (!is_fetch_stalled(p, way))
    {
      perform_fetch(p, way, fetch_op);
    }
    else if(BPRED_POLICY)
    {
      if (isStallDueToMisprediction && (!p->pipe_latch[FE_LATCH][way].valid))
      {
        insert_bubble_fetch_stage(p, way);
      }
    }
  }
}
void pipe_check_bpred(Pipeline *p, Pipeline_Latch *fetch_op)
{
  Trace_Rec *fetch_ins = &fetch_op->tr_entry;
  if (!fetch_op->valid)
  {
    return;
  }

  if (OP_CBR == fetch_op->tr_entry.op_type)
  {
    p->b_pred->stat_num_branches++;
  }
  else
  {
    return;
  }
  const uint32_t pc = static_cast<uint32_t>(fetch_ins->inst_addr);
  const bool takenPredicition = p->b_pred->GetPrediction(pc);
  const bool takenActual = (0 != fetch_ins->br_dir);

  const bool isMisprediction = (takenActual != takenPredicition);

  p->b_pred->UpdatePredictor(pc, takenActual, takenPredicition);

  if (isMisprediction)
  {
    fetch_op->is_mispred_cbr = true;
    isStallDueToMisprediction = true;
    p->b_pred->stat_num_mispred++;
    resume_op_id_mispred_branch = fetch_op->op_id;
    updateBlockingAndResumeOpId(0, fetch_op->op_id);
  }
}
