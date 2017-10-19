

#include "pipeline.h"
#include "utils.h"

void pipe_cycle_rename(Pipeline *p)
{
  // TODO: If src1/src2 is remapped set src1tag, src2tag
  // TODO: Find space in ROB and set drtag as such if successful
  // TODO: Find space in REST and transfer this inst (valid=1, sched=0)
  // TODO: If src1/src2 is not remapped marked as src ready
  // TODO: If src1/src2 remapped and the ROB (tag) is ready then mark src ready
  // FIXME: If there is stall, we should not do rename and ROB alloc twice
  for (uint32_t way = 0; way < PIPE_WIDTH; ++way)
  {
    if (!p->ID_latch[way].valid)
    {
      continue;
    }
    bool isRobEntryAvailable = false;
    bool isRestEntryAvailable = false;
    isRobEntryAvailable = ROB_check_space(p->pipe_ROB);
    isRestEntryAvailable = REST_check_space(p->pipe_REST);

    if ((!isRestEntryAvailable) || (!isRobEntryAvailable))
    {
      p->ID_latch[way].stall = true;
      //stall_all_decode_stages(p);
      stall_all_fetch_stages(p);
      continue;
    }
    else
    {
      p->ID_latch[way].stall = false;
    }

    Inst_Info &inst = p->ID_latch[way].inst;
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
    else
    {
      inst.src1_tag = -1;
      inst.src1_ready = true;
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
    else
    {
      inst.src2_tag = -1;
      inst.src2_ready = true;
    }

    inst.dr_tag = ROB_insert(p->pipe_ROB, inst);
    REST_insert(p->pipe_REST, inst);
    if (inst.dest_reg != -1)
    {
      RAT_set_remap(p->pipe_RAT, inst.dest_reg, inst.dr_tag);
    }
    p->ID_latch[way].valid = false;
  }
}
