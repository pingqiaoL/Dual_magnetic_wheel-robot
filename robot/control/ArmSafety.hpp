/**
 * @file ArmSafety.hpp
 * @brief 实现只执行一次的遥控安全开关上电互锁。
 */

#pragma once

#include "msg/ManualControlSwitches.hpp"

/**
 * @class ArmSafety
 * @brief 上电后要求先观察到安全开关关闭，随后按开关电平控制使能。
 *
 * 上电检查完成后不会因遥控信号暂时丢失而重新执行。信号无效时仍会
 * 立即失能，恢复后根据SA的当前位置重新决定是否使能。
 */
class ArmSafety
{
public:
  /** 安全互锁的三个阶段。 */
  enum class State
  {
    WaitForOff,
    ReadyForOn,
    Armed
  };

  /** 创建时始终等待一次明确的关闭状态。 */
  ArmSafety() : _state(State::WaitForOff) {}

  /** 重新进入上电互锁状态。 */
  void reset()
  {
    _state = State::WaitForOff;
  }

  /** 使用最新开关位置推进一次性上电互锁，并返回当前是否允许使能。 */
  bool update(bool valid, uint8_t position)
  {
    if (!valid)
      {
        if (_state == State::Armed)
          {
            _state = State::ReadyForOn;
          }
        return false;
      }

    if (_state == State::WaitForOff)
      {
        if (position == ManualControlSwitches::POSITION_OFF)
          {
            _state = State::ReadyForOn;
          }
        return false;
      }

    if (_state == State::ReadyForOn)
      {
        if (position == ManualControlSwitches::POSITION_ON)
          {
            _state = State::Armed;
            return true;
          }

        // 上电检查已经完成，之后按照SA当前电平正常解锁或关闭。
        return false;
      }

    if (position == ManualControlSwitches::POSITION_ON)
      {
        return true;
      }

    _state = State::ReadyForOn;
    return false;
  }

  /** 返回当前互锁阶段。 */
  State state() const { return _state; }

  /** 指示系统是否正在等待操作者把开关拨回关闭位置。 */
  bool waitingForOff() const { return _state == State::WaitForOff; }

  /** 返回供 NSH 状态输出使用的阶段名称。 */
  const char *stateName() const
  {
    switch (_state)
      {
        case State::ReadyForOn:
          return "ready_for_on";
        case State::Armed:
          return "armed";
        default:
          return "waiting_for_off";
      }
  }

private:
  State _state;
};
