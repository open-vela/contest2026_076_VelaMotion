/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_board_spi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-side SPI glue for the SomatoSync custom ESP32-S3 boards.
 *
 * The esp32s3 chip layer's SPI ops table references esp32s3_spi<N>_status()
 * and esp32s3_spi<N>_cmddata(), which are deliberately NOT implemented in the
 * chip layer: every ESP32-S3 board supplies its own, because the answer
 * depends on how the board wires its chip selects. This is the one board file
 * that the upstream reference board esp32s3-eye ships and that has no
 * counterpart in the vendor tree documentation, so it is easy to miss.
 *
 * The SomatoSync boards have no SPI display, so no cmddata handling is needed
 * and CONFIG_SPI_CMDDATA stays off; status simply reports "no error".
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <nuttx/spi/spi.h>

#include "somatosync_esp32s3.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32s3_spi2_status
 *
 * Description:
 *   Return the status of the selected SPI2 device. The W5500 and the NFC
 *   frontend have no board-visible status line, so this always reports OK.
 ****************************************************************************/

#ifdef CONFIG_ESP32S3_SPI2

uint8_t esp32s3_spi2_status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0;
}

#endif

/****************************************************************************
 * Name: esp32s3_spi3_status
 *
 * Description:
 *   Return the status of the selected SPI3 device (PN5180 NFC on the gateway,
 *   ICM-42688 IMU-B on the node). No board-visible status line.
 ****************************************************************************/

#ifdef CONFIG_ESP32S3_SPI3

uint8_t esp32s3_spi3_status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0;
}

#endif
