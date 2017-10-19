#include "pipeline.h"

#include <map>

int32_t getOldestSchedulableRestIndex(Pipeline *p, bool inOrderMachine)
{
  int32_t oldestSchedulableRestEntry = -1;
  std::map<uint64_t, int32_t> oldestMap;
  for (uint32_t i = 0; i < NUM_REST_ENTRIES; ++i)
  {
    REST_Entry& restEntry = p->pipe_REST->REST_Entries[i];
    Inst_Info& info = restEntry.inst;
    if (oldestMap.find(info.inst_num) == oldestMap.end())
    {
      bool inputsReady = info.src1_ready && info.src2_ready;
      bool validAndNotScheduled = restEntry.valid && (!restEntry.scheduled);
      if (inOrderMachine)
      {
        if (validAndNotScheduled)
        {
          oldestMap[info.inst_num] = i;
        }
      }
      else
      {
        if (validAndNotScheduled && inputsReady)
        {
          oldestMap[info.inst_num] = i;
        }
      }
    }
    else
    {
      utils::throw_exception("duplicate enteries in REST");
    }
  }

  if (inOrderMachine)
  {
    int32_t restIndex = oldestMap.empty() ? -1 : oldestMap.begin()->second;
    if (restIndex == -1)
    {
      oldestSchedulableRestEntry = -1;
    }
    else
    {
      Inst_Info &info = p->pipe_REST->REST_Entries[restIndex].inst;
      oldestSchedulableRestEntry = (info.src1_ready && info.src2_ready) ? restIndex : -1;
    }
  }
  else
  {
    oldestSchedulableRestEntry = oldestMap.empty() ? -1 : oldestMap.begin()->second;
  }
  return oldestSchedulableRestEntry;
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
        // TODO: SC stage should be stalled in the exeq stage, but if sc latch is invalid and stalled
        // we can still copy an instruction and save a cycle ?
        continue;
      }

      int32_t restIndex = getOldestSchedulableRestIndex(p, true);
      if (restIndex != -1)
      {
        REST_Entry& restEntry = p->pipe_REST->REST_Entries[restIndex];
        restEntry.scheduled = true;

        p->SC_latch[way].valid = true;
        p->SC_latch[way].stall = false;
        p->SC_latch[way].inst = restEntry.inst;
      }
      else
      {
        //utils::throw_exception("unexpected checkpoint");
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

      int32_t restIndex = getOldestSchedulableRestIndex(p, false);
      if (restIndex == -1)
      {
        continue;
      }

      REST_Entry &rest_entry = p->pipe_REST->REST_Entries[restIndex];
      if (rest_entry.valid && (!rest_entry.scheduled))
      {
        rest_entry.scheduled = true;

        int32_t scLatchSlot = way;
        p->SC_latch[scLatchSlot].valid = true;
        p->SC_latch[scLatchSlot].stall = false;
        p->SC_latch[scLatchSlot].inst = rest_entry.inst;
      }
      else
      {
        utils::throw_exception("unexpected checkpoint");
      }
    }
  }
}
