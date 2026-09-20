/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * VelaMotion / SomatoSync — openvela board-level definitions for the
 * custom ESP32-S3 boards (gateway + wearable sensor node).
 *
 * Pin assignments below are transcribed from the shipping ESP-IDF firmware:
 *   - Gateway : apps/gateway_app  (W5500 / PN5180 / SSD1306 / microSD)
 *   - Node    : apps/node_app     (dual ICM-42688 / SSD1306 / DRV2605 / ADC)
 * They MUST be re-verified against the schematic before flashing:
 *   docs/reference/SCH_主机_2026-05-22.pdf  and  docs/reference/PCB_PCB1_2026-05-22.pdf
 *
 ****************************************************************************/

#ifndef __BOARD_SOMATOSYNC_ESP32S3_H
#define __BOARD_SOMATOSYNC_ESP32S3_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Board variants ---------------------------------------------------------- */

#define SOMATOSYNC_BOARD_GATEWAY 1
#define SOMATOSYNC_BOARD_NODE    2

/* Console: USB-CDC (ttyACM0), 115200 8N1, identical on both boards. */

/*==========================================================================
 * Variant A — SomatoSync gateway (synchronisation master / edge gateway)
 *
 * W5500 Ethernet (SPI2), PN5180 NFC frontend (SPI3), SSD1306 128x64 OLED
 * (I2C0 @400 kHz), microSD in 1-bit SDMMC mode.
 *========================================================================*/

#if defined(CONFIG_SOMATOSYNC_BOARD_GATEWAY)

/* I2C0 — SSD1306 OLED, 7-bit address 0x3c
 *
 * Pin order verified against the production ESP-IDF gateway firmware
 * (apps/gateway_app/components/oled_ui/oled_ui.c):
 *     OLED_SDA_PIN = 1, OLED_SCL_PIN = 2
 * An earlier revision of this file had SCL/SDA transposed, which made the
 * panel fail to ACK on real hardware (board_lcd_initialize() -> -EIO).
 */

#define BOARD_I2C0_SCL_PIN        2
#define BOARD_I2C0_SDA_PIN        1

/* SPI2 — WIZnet W5500 hardwired TCP/IP controller */

#define BOARD_W5500_SPI_MOSI_PIN  11
#define BOARD_W5500_SPI_MISO_PIN  12
#define BOARD_W5500_SPI_SCLK_PIN  13
#define BOARD_W5500_SPI_CS_PIN    14
#define BOARD_W5500_INT_PIN       10
#define BOARD_W5500_RST_PIN       9

/* SPI3 — NXP PN5180 NFC frontend (NFC forum fast-config provisioning) */

#define BOARD_PN5180_SPI_SCK_PIN  42
#define BOARD_PN5180_SPI_MISO_PIN 41
#define BOARD_PN5180_SPI_MOSI_PIN 40
#define BOARD_PN5180_SPI_CS_PIN   39
#define BOARD_PN5180_BUSY_PIN     38
#define BOARD_PN5180_RST_PIN      47

/* microSD — 1-bit SDMMC (D3 held high so the card never enters SPI mode) */

#define BOARD_SDMMC_CLK_PIN       7
#define BOARD_SDMMC_CMD_PIN       6
#define BOARD_SDMMC_D0_PIN        5
#define BOARD_SDMMC_D3_PIN        4

/*==========================================================================
 * Variant B — SomatoSync wearable sensor node (TDMA slave, on-body IMU)
 *
 * Two independent ICM-42688 SPI buses (SPI2/SPI3) with per-IMU LDO gating
 * and an external 32.768 kHz clock, SSD1306 OLED + DRV2605 haptic driver on
 * I2C0, Li-Po fuel-gauge divider on ADC1_CH1.
 *========================================================================*/

#elif defined(CONFIG_SOMATOSYNC_BOARD_NODE)

/* I2C0 — SSD1306 OLED + DRV2605 haptic driver */

#define BOARD_I2C0_SCL_PIN        37
#define BOARD_I2C0_SDA_PIN        38
#define BOARD_OLED_PWR_EN_PIN     35
#define BOARD_OLED_RESET_PIN      36

/* Battery sense — divider ground switch prevents permanent drain */

#define BOARD_BAT_ADC_GATE_PIN    1
#define BOARD_BAT_ADC_PIN         2   /* ADC1_CH1 */

/* Per-IMU LDO enables and shared external clock */

#define BOARD_IMU_LDO_EN_A_PIN    4
#define BOARD_IMU_LDO_EN_B_PIN    5
#define BOARD_IMU_CLKIN_PIN       11  /* 32.768 kHz injected into both IMUs */

/* SPI2 — ICM-42688 IMU-A */

#define BOARD_IMU_A_CS_PIN        6
#define BOARD_IMU_A_SCLK_PIN      7
#define BOARD_IMU_A_MOSI_PIN      8
#define BOARD_IMU_A_MISO_PIN      9
#define BOARD_IMU_A_INT_PIN       10

/* SPI3 — ICM-42688 IMU-B */

#define BOARD_IMU_B_CS_PIN        16
#define BOARD_IMU_B_SCLK_PIN      15
#define BOARD_IMU_B_MOSI_PIN      14
#define BOARD_IMU_B_MISO_PIN      13
#define BOARD_IMU_B_INT_PIN       12

/* NFC wake-up mailbox */

#define BOARD_NFC_GPO_PIN         18

/* Charger status (hardware V2 option, off by default) */

#define BOARD_HW_CHG_PIN          21

#else
#  error "No SomatoSync board variant selected: set CONFIG_SOMATOSYNC_BOARD_GATEWAY or CONFIG_SOMATOSYNC_BOARD_NODE"
#endif

#endif /* __BOARD_SOMATOSYNC_ESP32S3_H */
