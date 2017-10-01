#pragma once

#include <cstdint>
#include <exception>

struct TwoBitBranchPredictor
{
  enum class PredictorState : uint8_t { not_taken, weakly_not_taken, weakly_taken, taken };

  TwoBitBranchPredictor()
    : state_(PredictorState::weakly_taken)
  {
  }

  bool getPrediction()
  {
    switch (state_)
    {
    case PredictorState::taken:
    case PredictorState::weakly_taken:
      return true;
    case PredictorState::not_taken:
    case PredictorState::weakly_not_taken:
      return false;
    default:
      throw std::exception();
      return false;
    }
  }

  void spinTowardsTaken()
  {
    switch (state_)
    {
    case PredictorState::not_taken:
      state_ = PredictorState::weakly_not_taken;
      break;

    case PredictorState::weakly_not_taken:
      state_ = PredictorState::weakly_taken;
      break;

    case PredictorState::weakly_taken:
      state_ = PredictorState::taken;
      break;

    case PredictorState::taken:
      state_ = PredictorState::taken;
      break;
    default:
      throw std::exception();
    }
  }

  void spinTowardsNotTaken()
  {
    switch (state_)
    {
    case PredictorState::not_taken:
      state_ = PredictorState::not_taken;
      break;

    case PredictorState::weakly_not_taken:
      state_ = PredictorState::not_taken;
      break;

    case PredictorState::weakly_taken:
      state_ = PredictorState::weakly_not_taken;
      break;

    case PredictorState::taken:
      state_ = PredictorState::weakly_taken;
      break;
    default:
      throw std::exception();
    }
  }

  PredictorState state_;
};