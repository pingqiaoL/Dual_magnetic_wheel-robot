/**
 * @file ParamSystem.cpp
 * @brief 把 CBoard 预留的两个 Flash 扇区装配给通用参数管理器。
 */

#include "robot/params/ParamSystem.hpp"

#include "robot/params/FlashParamStorage.hpp"

#include <arch/board/board.h>

/** 创建一次后持续存在的板级 Flash 后端。 */
static FlashParamStorage &boardParamStorage()
{
  static FlashParamStorage storage(BOARD_PARAM_FLASH_SLOT_A,
                                   BOARD_PARAM_FLASH_SLOT_B,
                                   BOARD_PARAM_FLASH_SLOT_SIZE);
  return storage;
}

/** 初始化参数系统并加载最新有效页。 */
bool paramSystemInitialize()
{
  return ParamManager::instance().initialize(boardParamStorage());
}

/** 返回参数管理器，供命令和业务模块共享。 */
ParamManager &params()
{
  return ParamManager::instance();
}
