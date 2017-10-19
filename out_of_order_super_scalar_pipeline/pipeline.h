#ifndef _PIPELINE_H
#define _PIPELINE_H

#include <inttypes.h>
#include <stdio.h>
#include <assert.h>

#include "trace.h"

#include "rat.h"
#include "rest.h"
#include "rob.h"
#include "exeq.h"
#include "utils.h"

#define MAX_PIPE_WIDTH 8
#define MAX_BROADCASTS 256

extern int32_t PIPE_WIDTH;
extern int32_t SCHED_POLICY;
extern int32_t LOAD_EXE_CYCLES;
extern int32_t NUM_REST_ENTRIES;
extern int32_t NUM_ROB_ENTRIES;

// Pipeline Latches 
typedef struct Pipe_Latch_Struct
{
  bool valid;
  bool stall;
  Inst_Info inst;
} Pipe_Latch;

typedef struct Pipeline
{
  FILE *tr_file;
  Pipe_Latch FE_latch[MAX_PIPE_WIDTH];// fetch Latches
  Pipe_Latch ID_latch[MAX_PIPE_WIDTH];// decode Latches
  Pipe_Latch SC_latch[MAX_PIPE_WIDTH];// schedule Latches
  Pipe_Latch EX_latch[MAX_BROADCASTS];// Exe Latches (note, can be > pipe_width)

  ROB *pipe_ROB;
  RAT *pipe_RAT;
  REST *pipe_REST;
  EXEQ *pipe_EXEQ;  // execution Q for multicycle ops (students need not implement this object)

  uint64_t inst_num_tracker; //sequence number for inst
  uint64_t halt_inst_num;   // last inst in Trace
  bool halt;               // Pipeline is halted Flag

  // Statistics: students need to update these counters
  uint64_t stat_retired_inst;         // Total Commited Instructions
  uint64_t stat_num_cycle;            // Total Cycles
} Pipeline;

Pipeline *pipe_init(FILE *tr_file);        // Allocate Structures

void pipe_cycle(Pipeline *p);              // Runs one Pipeline Cycle
void pipe_cycle_fetch(Pipeline *p);        // Fetch Stage 
void pipe_cycle_decode(Pipeline *p);       // ID Stage 
void pipe_cycle_rename(Pipeline *p);       // Rename Stage 
void pipe_cycle_schedule(Pipeline *p);     // out of reservation station
void pipe_cycle_exe(Pipeline *p);          // execute (multi-cycle?)
void pipe_cycle_broadcast(Pipeline *p);    // broadcast and update ROB
void pipe_cycle_commit(Pipeline *p);       // commit
void pipe_fetch_inst(Pipeline *p, Pipe_Latch *fe_latch);

void stall_all_fetch_stages(Pipeline *p);
void stall_all_decode_stages(Pipeline *p);
void stall_all_schedule_stages(Pipeline *p);
void resume_all_fetch_stages(Pipeline *p);
void resume_all_decode_stages(Pipeline *p);
bool are_all_decode_stages_stalled(Pipeline *p);
void pipe_print_state(Pipeline *p);        // Print Pipeline state

#endif
