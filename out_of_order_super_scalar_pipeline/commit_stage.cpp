#include "pipeline.h"

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
      //std::cout << " Instruction = " << info.inst_num << std::endl;
      p->stat_retired_inst++;
      if (info.inst_num >= p->halt_inst_num)
      {
        p->halt = true;
      }
      if (info.dest_reg != -1)
      {
        int phyReg = RAT_get_remap(p->pipe_RAT, info.dest_reg);
        if ((phyReg != -1) && (info.dr_tag == phyReg))
        {
          RAT_reset_entry(p->pipe_RAT, info.dest_reg);
        }
      }
      else
      {
        //utils::throw_exception("Expectation is that for ins. w/o destination dr_tag has rob id");
      }
    }
    else
    {
      break;
    }
  }

}