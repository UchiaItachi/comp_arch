#include "DependencyAnalysis.h"
#include "helper.h"

#include <cstdlib>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>

void dependencyAnalysis(Pipeline *p)
{
  std::vector<uint64_t> decodeOpIds;
  std::vector<uint64_t> execOpIds;
  std::vector<uint64_t> memOpIds;

  std::map<uint64_t, Pipeline_Latch*> decMap;
  std::map<uint64_t, Pipeline_Latch*> execMap;
  std::map<uint64_t, Pipeline_Latch*> memMap;

  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    Pipeline_Latch *declatch = &p->pipe_latch[FE_LATCH][way];
    decodeOpIds.push_back(declatch->op_id);
    decMap[declatch->op_id] = declatch;

    Pipeline_Latch *execlatch = &p->pipe_latch[ID_LATCH][way];
    execOpIds.push_back(execlatch->op_id);
    execMap[execlatch->op_id] = execlatch;

    Pipeline_Latch *memlatch = &p->pipe_latch[EX_LATCH][way];
    memOpIds.push_back(memlatch->op_id);
    memMap[memlatch->op_id] = memlatch;
  }
  std::sort(decodeOpIds.begin(), decodeOpIds.end());
  std::sort(execOpIds.begin(), execOpIds.end());
  std::sort(memOpIds.begin(), memOpIds.end());

  for (size_t curr_dec_i = 0; curr_dec_i < decodeOpIds.size(); ++curr_dec_i)
  {
    Pipeline_Latch *curr_dec_l = decMap[decodeOpIds[curr_dec_i]];
    Trace_Rec* curr_dec_ins = &curr_dec_l->tr_entry;

    if (!curr_dec_l->valid)
    {
      continue;
    }
    // decode checks
    for (size_t top_dec_i = 0; top_dec_i < curr_dec_i; ++top_dec_i)
    {
      Pipeline_Latch *top_dec_l = decMap[decodeOpIds[top_dec_i]];
      Trace_Rec* top_dec_ins = &top_dec_l->tr_entry;
      if (!top_dec_l->valid)
      {
        continue;
      }
      if (can_stall_decode_stage_dependencies(top_dec_ins, curr_dec_ins))
      {
        updateBlockingAndResumeOpId(curr_dec_l->op_id, top_dec_l->op_id);
        return;
      }
    }

    // execute checks
    for (size_t top_exec_i = execOpIds.size(); top_exec_i > 0; --top_exec_i)
    {
      Pipeline_Latch *top_exec_l = execMap[execOpIds[top_exec_i - 1]];
      Trace_Rec* top_exec_ins = &top_exec_l->tr_entry;
      if (!top_exec_l->valid)
      {
        continue;
      }

      if (skipExecStageChecksOnDestinationMatch(top_exec_ins, curr_dec_ins))
      {
        break;
      }

      if (can_stall_exec_stage_dependencies(top_exec_ins, curr_dec_ins))
      {
        updateBlockingAndResumeOpId(curr_dec_l->op_id, top_exec_l->op_id);
        return;
      }
    }

    if (isForwardingEnabled())
    {
      continue;
    }
    // memory checks
    for (size_t top_mem_i = memOpIds.size(); top_mem_i > 0; --top_mem_i)
    {
      Pipeline_Latch *top_mem_l = memMap[memOpIds[top_mem_i - 1]];
      Trace_Rec* top_mem_ins = &top_mem_l->tr_entry;
      if (!top_mem_l->valid)
      {
        continue;
      }
      if (can_stall_mem_stage_dependencies(top_mem_ins, curr_dec_ins))
      {
        updateBlockingAndResumeOpId(curr_dec_l->op_id, top_mem_l->op_id);
        return;
      }
    }
  }
}


void updateBlockingAndResumeOpId(uint64_t blockingOpid, uint64_t resumeOpId)
{
  youngest_blocking_op_id = blockingOpid;
  resume_op_id = resumeOpId;
}

void stallDependentStages(Pipeline *p)
{
  for (int32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    const uint64_t decodeStageNWayOpId = p->pipe_latch[FE_LATCH][way].op_id;
    if (can_stall_n_way(decodeStageNWayOpId))
    {
      stall_fetch_stage(p, way);
      stall_decode_stage(p, way);
    }
  }
}

void resumeStalledStages(Pipeline *p)
{
  if (can_resume_n_ways(p))
  {
    resume_op_id = 0;
    youngest_blocking_op_id = 0;
    for (int32_t way = 0; way < PIPE_WIDTH; ++way)
    {
      resume_decode_stage_stall(p, way);
      resume_fetch_stage_stall(p, way);
    }
  }
}

void resumeControlDependencies(Pipeline *p)
{
  bool canResume = false;
  for (int32_t i = 0; i < PIPE_WIDTH; ++i)
  {
    if ((p->pipe_latch[MEM_LATCH][i].op_id == resume_op_id_mispred_branch) 
      || (resume_op_id_mispred_branch == 0))
    {
      canResume = true;
      break;
    }
  }

  if (canResume)
  {
    resume_op_id_mispred_branch = 0;
    isStallDueToMisprediction = false;
    for (int32_t i = 0; i < PIPE_WIDTH; ++i)
    {
      resume_fetch_stage_stall(p, i);
    }
  }
}

void stallControlDependencies(Pipeline *p)
{
  if ((resume_op_id_mispred_branch != 0) && isStallDueToMisprediction)
  {
    for (int32_t i = 0; i < PIPE_WIDTH; ++i)
    {
      stall_fetch_stage(p, i);
    }
  }
}

void analyzeDependencies(Pipeline *p)
{
  if (BPRED_POLICY)
  {
    resumeControlDependencies(p);
  }
  resumeStalledStages(p);
  dependencyAnalysis(p);
  stallDependentStages(p);
  
  if (BPRED_POLICY)
  {
    stallControlDependencies(p);
  }
}
