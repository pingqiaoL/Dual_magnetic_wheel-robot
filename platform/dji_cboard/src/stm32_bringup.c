/****************************************************************************
 * platform/dji_cboard/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <nuttx/fs/fs.h>

#include "dji_cboard.h"

int stm32_bringup(void)
{
  int ret;

#ifdef CONFIG_CAN
  ret = stm32_can_setup();
  if (ret < 0 && ret != -EEXIST)
    {
      return ret;
    }
#endif

#ifdef CONFIG_FS_PROCFS
  ret = nx_mount(NULL, CONFIG_NSH_PROC_MOUNTPOINT, "procfs", 0, NULL);

  if (ret < 0 && ret != -EEXIST)
    {
      return ret;
    }
#endif

  return 0;
}
