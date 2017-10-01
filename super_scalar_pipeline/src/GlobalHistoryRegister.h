#pragma once

#include <cstdint>
#include <bitset>

struct GlobalHistoryRegister
{
  GlobalHistoryRegister()
    : register_(0)
  {
  }

  void insertTaken()
  {
    register_ <<= 1;
    register_ |= 0x1;
  }

  void insertNotTaken()
  {
    register_ <<= 1;
    register_ |= 0x0;
  }

  uint32_t value() const
  {
    return static_cast<uint32_t>(register_.to_ulong());
  }
  static const uint32_t REGISTER_LENGTH = 12;  
  
  std::bitset<REGISTER_LENGTH> register_;
};
