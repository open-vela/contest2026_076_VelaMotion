/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_boot.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board early boot for the SomatoSync custom ESP32-S3 boards.
 *
 * The chip layer requires every ESP32-S3 board to provide
 * esp32s3_board_initialize(); the symbol name is fixed by
 * nuttx/arch/xtensa/src/esp32s3 and must not be renamed.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

#include <nuttx/board.h>
#include <arch/board/board.h>

#include "somatosync_esp32s3.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_board_initialize
 *
 * Description:
 *   Called early in the boot sequence, after memory has been configured and
 *   mapped but before any devices are initialised. Nothing is required at
 *   this stage for this board: all peripheral bring-up is deferred to
 *   board_late_initialize() so that it can use the full driver framework.
 ****************************************************************************/

void esp32s3_board_initialize(void)
{
}

/****************************************************************************
 * Name: board_late_initialize
 *
 * Description:
 *   Called immediately after up_initialize() and just before the initial
 *   application is started, when CONFIG_BOARD_LATE_INITIALIZE is selected.
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  somatosync_bringup();
}
#endif
