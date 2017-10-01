#include "pipeline.h"
#include "trace.h"
#include "helper.h"

#include <cstring>

void stall_decode_stage(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *decode_latch = &p->pipe_latch[ID_LATCH][way];
  decode_latch->stall = true;
}

void stall_fetch_stage(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *fetch_latch = &p->pipe_latch[FE_LATCH][way];
  fetch_latch->stall = true;
}

void insert_bubble_fetch_stage(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *fetch_latch = &p->pipe_latch[FE_LATCH][way];
  memset(fetch_latch, 0x0, sizeof(Pipeline_Latch));
  fetch_latch->valid = false;
}

void insert_bubble_decode_stage(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *decode_latch = &p->pipe_latch[ID_LATCH][way];
  memset(decode_latch, 0x0, sizeof(Pipeline_Latch));
  decode_latch->valid = false;
}

void resume_decode_stage_stall(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *decode = &p->pipe_latch[ID_LATCH][way];
  decode->stall = false;
}

void resume_fetch_stage_stall(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *fetch = &p->pipe_latch[FE_LATCH][way];
  fetch->stall = false;
}

bool is_decode_stalled(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *decode = &p->pipe_latch[ID_LATCH][way];
  return decode->stall;
}

bool is_fetch_stalled(Pipeline *p, uint32_t way)
{
  Pipeline_Latch *fetch = &p->pipe_latch[FE_LATCH][way];
  return fetch->stall;
}

bool isForwardingEnabled()
{
  return ENABLE_EXE_FWD && ENABLE_MEM_FWD;
}

bool can_stall_n_way(uint64_t decode_stage_op_id)
{
  if ((decode_stage_op_id >= youngest_blocking_op_id) && (youngest_blocking_op_id != 0))
  {
    return true;
  }
  return false;
}

bool isResumeMatchInExStage(Pipeline *p)
{
  return true;
  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (resume_op_id == p->pipe_latch[ID_LATCH][way].op_id)
    {
      if (resume_op_id)
      {
        if (p->pipe_latch[ID_LATCH][way].tr_entry.mem_read)
        {
          // resume in mem stage
        }
        else
        {
          return true;
        }
      }
    }
  }
  return false;
}

bool isResumeMatchInMemStage(Pipeline *p)
{
  return true;
  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (resume_op_id == p->pipe_latch[EX_LATCH][way].op_id)
    {
      if (resume_op_id)
      {
        if (p->pipe_latch[EX_LATCH][way].tr_entry.mem_read)
        {
          return true;
        }
      }
    }
  }
  return false;
}

bool isResumeMatchInWBStage(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (resume_op_id == p->pipe_latch[MEM_LATCH][way].op_id)
    {
      if (resume_op_id)
      {
        return true;
      }
    }
  }
  return false;
}

bool can_resume_n_ways(Pipeline* p)
{
  if (isResumeMatchInWBStage(p))
  {
    return true;
  }
  else if (isForwardingEnabled())
  {
    if (isResumeMatchInExStage(p))
    {
      return true;
    }
    else if (isResumeMatchInMemStage(p))
    {
      return true;
    }
  }
  return false;
}

bool can_stall_mem_stage_dependencies(Trace_Rec *mem_stage_ins, Trace_Rec *decode_stage_ins)
{
  bool mem_dest_id_src1_alu = mem_stage_ins->dest_needed && decode_stage_ins->src1_needed && (mem_stage_ins->dest == decode_stage_ins->src1_reg);
  bool mem_dest_id_src2_alu = mem_stage_ins->dest_needed && decode_stage_ins->src2_needed && (mem_stage_ins->dest == decode_stage_ins->src2_reg);
  bool mem_cc_write_id_cc_read_alu = mem_stage_ins->cc_write && decode_stage_ins->cc_read;

  bool mem_dependency_alu = mem_dest_id_src1_alu || mem_dest_id_src2_alu || mem_cc_write_id_cc_read_alu;

  bool is_mem_dependency = isForwardingEnabled() ? false : mem_dependency_alu; // no stalling in case MEM fwd is enabled
  return is_mem_dependency;
}

bool can_stall_exec_stage_dependencies(Trace_Rec *execute_stage_ins, Trace_Rec *decode_stage_ins)
{
  bool ex_dest_id_src1_alu = execute_stage_ins->dest_needed && decode_stage_ins->src1_needed && (execute_stage_ins->dest == decode_stage_ins->src1_reg);
  bool ex_dest_id_src2_alu = execute_stage_ins->dest_needed && decode_stage_ins->src2_needed && (execute_stage_ins->dest == decode_stage_ins->src2_reg);
  bool ex_cc_write_id_cc_read_alu = execute_stage_ins->cc_write && decode_stage_ins->cc_read;

  bool exec_dependency_alu = ex_dest_id_src1_alu || ex_dest_id_src2_alu || ex_cc_write_id_cc_read_alu;

  bool is_exec_dependency = false;
  if (isForwardingEnabled())
  {
    bool exec_dependency_mem_access = exec_dependency_alu && execute_stage_ins->mem_read;
    is_exec_dependency = exec_dependency_mem_access;
  }
  else
  {
    is_exec_dependency = exec_dependency_alu;
  }
  return is_exec_dependency;
}

bool can_stall_decode_stage_dependencies(Trace_Rec *decode_stage_n_way_ins, Trace_Rec *decode_stage_current_ins)
{
  bool idn_dest_idc_src1_alu = decode_stage_n_way_ins->dest_needed && decode_stage_current_ins->src1_needed && (decode_stage_n_way_ins->dest == decode_stage_current_ins->src1_reg);
  bool idn_dest_idc_src2_alu = decode_stage_n_way_ins->dest_needed && decode_stage_current_ins->src2_needed && (decode_stage_n_way_ins->dest == decode_stage_current_ins->src2_reg);
  bool idn_cc_write_idc_cc_read_alu = decode_stage_n_way_ins->cc_write && decode_stage_current_ins->cc_read;

  bool decode_dependency_alu = idn_dest_idc_src1_alu || idn_dest_idc_src2_alu || idn_cc_write_idc_cc_read_alu;
  return decode_dependency_alu;
}

bool skipExecStageChecksOnDestinationMatch(Trace_Rec *oldest_exec_stage_ins, Trace_Rec* decode_stage_ins)
{
  bool ex_dest_id_src1_alu = oldest_exec_stage_ins->dest_needed && decode_stage_ins->src1_needed && (oldest_exec_stage_ins->dest == decode_stage_ins->src1_reg);
  bool ex_dest_id_src2_alu = oldest_exec_stage_ins->dest_needed && decode_stage_ins->src2_needed && (oldest_exec_stage_ins->dest == decode_stage_ins->src2_reg);
  bool ex_cc_write_id_cc_read_alu = oldest_exec_stage_ins->cc_write && decode_stage_ins->cc_read;

  bool exec_dependency_alu = ex_dest_id_src1_alu || ex_dest_id_src2_alu || ex_cc_write_id_cc_read_alu;

  if (isForwardingEnabled())
  {
    if (exec_dependency_alu && (oldest_exec_stage_ins->mem_read == 0))
    {
      return true;
    }
  }
  return false;
}