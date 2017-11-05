#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include "dram.h"

#define ROWBUF_SIZE         1024
#define DRAM_BANKS          16

//---- Latency for Part B ------

#define DRAM_LATENCY_FIXED  100

//---- Latencies for Part C ------

#define DRAM_T_ACT         45
#define DRAM_T_CAS         45
#define DRAM_T_PRE         45
#define DRAM_T_BUS         10


extern MODE SIM_MODE;
extern uint64_t CACHE_LINESIZE;


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

DRAM *dram_new()
{
  DRAM *dram = (DRAM *) calloc(1, sizeof(DRAM));
  assert(DRAM_BANKS <= MAX_DRAM_BANKS);
  return dram;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void dram_print_stats(DRAM *dram)
{
  double rddelay_avg = 0;
  double wrdelay_avg = 0;
  char header[256];
  sprintf(header, "DRAM");

  if (dram->stat_read_access)
  {
    rddelay_avg = (double) (dram->stat_read_delay) / (double) (dram->stat_read_access);
  }

  if (dram->stat_write_access)
  {
    wrdelay_avg = (double) (dram->stat_write_delay) / (double) (dram->stat_write_access);
  }

  printf("\n%s_READ_ACCESS\t\t : %10llu", header, dram->stat_read_access);
  printf("\n%s_WRITE_ACCESS\t\t : %10llu", header, dram->stat_write_access);
  printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
  printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);


}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

uint64_t dram_access(DRAM *dram, Addr lineaddr, Flag is_dram_write)
{
  uint64_t delay = DRAM_LATENCY_FIXED;

  if (SIM_MODE != SIM_MODE_B)
  {
    delay = dram_access_sim_rowbuf(dram, lineaddr, is_dram_write);
  }

  // Update stats
  if (is_dram_write)
  {
    dram->stat_write_access++;
    dram->stat_write_delay += delay;
  }
  else
  {
    dram->stat_read_access++;
    dram->stat_read_delay += delay;
  }

  return delay;
}

///////////////////////////////////////////////////////////////////
// ------------ DO NOT MODIFY THE CODE ABOVE THIS LINE -----------
// Modify the function below only if you are attempting Part C 
///////////////////////////////////////////////////////////////////

uint64_t dram_access_sim_rowbuf(DRAM *dram, Addr lineaddr, Flag is_dram_write)
{
  uint64_t delay = 0;

  // Assume a mapping with consecutive lines in the same row
  // Assume a mapping with consecutive rowbufs in consecutive rows
  // You need to write this fuction to track open rows
  // You will need to compute delay based on row hit/miss/empty

  const uint32_t numLinesInRowBuf = ROWBUF_SIZE / CACHE_LINESIZE;
  const uint32_t numOfLineBits = std::log2(numLinesInRowBuf);
  const uint32_t numOfBankBits = std::log2(DRAM_BANKS);

  const uint32_t lineNum = lineaddr & ((0x1 << numOfLineBits) - 1);
  const uint32_t bankNum = (lineaddr >> numOfLineBits) & ((0x1 << numOfBankBits) - 1);

  const uint64_t rowNum = (lineaddr >> (numOfBankBits + numOfLineBits));

  Rowbuf_Entry& bankRowBuf = dram->perbank_row_buf[bankNum];
  if (FALSE == bankRowBuf.valid)
  {
    bankRowBuf.valid = TRUE;
    bankRowBuf.rowid = rowNum;
    delay += DRAM_T_ACT + DRAM_T_CAS + DRAM_T_PRE;
  }
  else
  {
    if (rowNum == bankRowBuf.rowid)
    {
      delay += DRAM_T_CAS;
    }
    else
    {
      delay += DRAM_T_PRE + DRAM_T_CAS + DRAM_T_ACT;
      bankRowBuf.rowid = rowNum;
    }
  }

  delay += DRAM_T_BUS;

  return delay;
}


