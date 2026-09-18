/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_esp32s3.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-private declarations for the SomatoSync custom ESP32-S3 boards.
 *
 * Kept in src/ (not include/) so that the quoted includes in this directory's
 * sources resolve relative to the including file — the same layout the
 * upstream reference board uses for esp32s3-eye.h.
 ****************************************************************************/

#ifndef __BOARD_SOMATOSYNC_ESP32S3_INTERNAL_H
#define __BOARD_SOMATOSYNC_ESP32S3_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: somatosync_bringup
 *
 * Description:
 *   Bring up all on-board peripherals. Called from board_late_initialize()
 *   (CONFIG_BOARD_LATE_INITIALIZE=y) or from board_app_initialize() via
 *   BOARDIOC_INIT (CONFIG_BOARDCTL=y).
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 ****************************************************************************/

int somatosync_bringup(void);

#ifdef CONFIG_SOMATOSYNC_W5500
/****************************************************************************
 * Name: somatosync_w5500_initialize
 *
 * Description:
 *   Board glue for the WIZnet W5500 Ethernet controller: configure RSTn and
 *   INTn, allocate the SPI bus and register the network driver.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 ****************************************************************************/

int somatosync_w5500_initialize(void);
#endif

#endif /* __BOARD_SOMATOSYNC_ESP32S3_INTERNAL_H */
