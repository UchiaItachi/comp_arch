#include "bpred.h"

#define TAKEN   true
#define NOTTAKEN false

BPRED::BPRED(uint32_t policy) 
  : stat_num_branches(0)
  , stat_num_mispred(0)
{

  
}

bool BPRED::GetPrediction(uint32_t PC)
{
  return TAKEN;  
}

void BPRED::UpdatePredictor(uint32_t PC, bool resolveDir, bool predDir) 
{
  const bool spinTowardsTaken = isTowardsTakenDirection(resolveDir, predDir);
}

bool BPRED::isTowardsTakenDirection(bool resolveDir, bool predDir) const
{
  const bool TOWARDS_TAKEN = true;
  const bool TOWARDS_NOT_TAKEN = false;
 
  bool spintTowardsTaken = TOWARDS_NOT_TAKEN;

  if ((true == resolveDir) && (true == predDir))
  {
    spintTowardsTaken = TOWARDS_TAKEN;  // actual: taken, pred: taken
  }
  else if ((false == resolveDir) && (true == predDir))
  {
    spintTowardsTaken = TOWARDS_NOT_TAKEN; // actual: not_taken, pred: taken
  }
  else if ((true == resolveDir) && (false == predDir))
  {
    spintTowardsTaken = TOWARDS_TAKEN;  // 
  }
  else if ((false == resolveDir) && (false == predDir))
  {
    spintTowardsTaken = TOWARDS_NOT_TAKEN;
  }
  else
  {
    throw std::exception();
  }
  
  return spintTowardsTaken;
}