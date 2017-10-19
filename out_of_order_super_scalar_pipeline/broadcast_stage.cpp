//
// Created by itachi on 10/16/17.
//

#include "pipeline.h"

void pipe_cycle_broadcast(Pipeline *p)
{
  // TODO: Go through all instructions out of EXE latch
  // TODO: Broadcast it to REST (using wakeup function)
  // TODO: Remove entry from REST (using inst_num)
  // TODO: Update the ROB, mark ready, and update Inst Info in ROB
  for (uint32_t way = 0; way < MAX_EXEQ_ENTRIES; ++way)
  {
    if (!p->EX_latch[way].valid)
    {
      continue;
    }
    Inst_Info &info = p->EX_latch[way].inst;

    REST_wakeup(p->pipe_REST, info.dr_tag);
    REST_remove(p->pipe_REST, info);

    ROB_mark_ready(p->pipe_ROB, info);

    p->EX_latch[way].valid = false;
    p->EX_latch[way].stall = false;
  }
}