#ifndef _BPRED_H_
#define _BPRED_H_

#include "TwoBitBranchPredictor.h"
#include "GlobalHistoryRegister.h"

#include <cstdint>
#include <map>

typedef enum BPRED_TYPE_ENUM 
{
  BPRED_PERFECT=0,
  BPRED_ALWAYS_TAKEN=1,
  BPRED_GSHARE=2, 
  NUM_BPRED_TYPE=3
} BPRED_TYPE;

struct PatternHistoryTable
{
  typedef uint32_t AddressIndex_t;
  typedef std::map<AddressIndex_t, TwoBitBranchPredictor>  PatterHistoryTable_t;
 
  void updatePatternEntry(uint32_t addressPatternHash, bool takenDirection)
  {
    const uint32_t addressPatternIndex = addressPatternHash & ADDRESS_INDEX_MASK;
    TwoBitBranchPredictor& _2bc = pht_[addressPatternIndex];

    if (takenDirection)
    {
      _2bc.spinTowardsTaken();
    }
    else
    {
      _2bc.spinTowardsNotTaken();
    }
  }

  PatterHistoryTable_t pht_;
  const uint32_t HISTORY_LENGTH = 12;
  const uint32_t ADDRESS_INDEX_MASK = ((0x1 << HISTORY_LENGTH) - 1);
};

class BPRED
{
  BPRED_TYPE policy;

public:
  uint64_t stat_num_branches;
  uint64_t stat_num_mispred;
  
  BPRED(uint32_t policy);
  bool GetPrediction(uint32_t PC);  
  void UpdatePredictor(uint32_t PC, bool resolveDir, bool predDir);  

private:
  bool isTowardsTakenDirection(bool resolveDir, bool predDir) const;


  PatternHistoryTable pht_;
};

#endif

