/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_appinit.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BOARDIOC_INIT entry point for the SomatoSync custom ESP32-S3 boards.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/board.h>

#include "somatosync_esp32s3.h"

#ifdef CONFIG_BOARDCTL

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform application specific initialization. Called indirectly through
 *   the boardctl() BOARDIOC_INIT command.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
#ifdef CONFIG_BOARD_LATE_INITIALIZE
  /* Board initialization already performed by board_late_initialize() */

  return OK;
#else
  /* Perform board-specific initialization */

  return somatosync_bringup();
#endif
}

#endif /* CONFIG_BOARDCTL */
