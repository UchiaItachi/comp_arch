#pragma once

#include "pipeline.h"
#include "trace.h"
#include <map>

extern int32_t PIPE_WIDTH;
extern int32_t ENABLE_MEM_FWD;
extern int32_t ENABLE_EXE_FWD;
extern int32_t BPRED_POLICY;

extern uint64_t resume_op_id;
extern uint64_t resume_op_id_mispred_branch;
extern uint64_t retired_op_id;
extern uint64_t youngest_blocking_op_id;
extern bool isStallDueToMisprediction;

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
