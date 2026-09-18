/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * On-board peripheral bring-up for the SomatoSync custom ESP32-S3 boards.
 *
 *   Gateway : SSD1306 OLED (I2C0) + W5500 Ethernet (SPI2) + microSD (SDMMC)
 *             + Wi-Fi / ESP-NOW radio + PN5180 NFC (SPI3)
 *   Node    : SSD1306 OLED + DRV2605 haptics (I2C0) + dual ICM-42688 (SPI2/3)
 *             + battery gauge (ADC1_CH1) + Wi-Fi / ESP-NOW radio
 *
 * Hardware-verification note
 * --------------------------
 * The OLED path is complete and uses only verified NuttX APIs. Three hooks
 * call into the esp32s3 chip / espressif vendor layer and are marked
 * NOTE(verify) below; they are called exactly the way the upstream
 * esp32s3-eye board calls them, but the header names must be confirmed
 * against your checkout before the first build.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <syslog.h>
#include <errno.h>

#include <nuttx/fs/fs.h>
#include <nuttx/board.h>

#ifdef CONFIG_SOMATOSYNC_OLED
#  include <nuttx/lcd/lcd_dev.h>
#endif

#ifdef CONFIG_ESP32S3_SDMMC
#  include "esp32s3_board_sdmmc.h"     /* NOTE(verify): chip-layer header */
#endif

#ifdef CONFIG_ESP32S3_WIFI
#  include "esp32s3_board_wlan.h"      /* NOTE(verify): vendor-layer header */
#endif

#include "somatosync_esp32s3.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: somatosync_bringup
 *
 * Description:
 *   Perform board-specific initialization. Called from board_late_initialize()
 *   when CONFIG_BOARD_LATE_INITIALIZE=y, otherwise from
 *   board_app_initialize() through BOARDIOC_INIT.
 *
 *   A failure in any one peripheral is logged but does not abort bring-up:
 *   NSH still comes up with the interfaces that did initialise.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 ****************************************************************************/

int somatosync_bringup(void)
{
  int ret;

#ifdef CONFIG_FS_PROCFS
  ret = nx_mount(NULL, "/proc", "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n",
             CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_SOMATOSYNC_OLED
  /* SSD1306 128x64 — the graphics capability landing point */

  ret = board_lcd_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize the OLED: %d\n", ret);
    }
  else
    {
      ret = lcddev_register(0);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: lcddev_register() failed: %d\n", ret);
        }
    }
#endif

#ifdef CONFIG_SOMATOSYNC_W5500
  /* WIZnet W5500 — Ethernet uplink to the ROS 2 host */

  ret = somatosync_w5500_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize the W5500: %d\n", ret);
    }
#endif

#ifdef CONFIG_SOMATOSYNC_SDCARD
  /* microSD in 1-bit SDMMC mode — raw motion-capture logging backend.
   *
   * NOTE(verify): board_sdmmc_initialize() is supplied by the esp32s3 chip
   * layer; the upstream esp32s3-eye board calls it in exactly this way and
   * the pin selection comes from CONFIG_ESP32S3_SDMMC_CMD plus the defconfig.
   */

  ret = board_sdmmc_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize SDMMC: %d\n", ret);
    }
#endif

#ifdef CONFIG_SOMATOSYNC_WIFI
  /* ESP-NOW rides on the Wi-Fi radio; board_wlan_init() is the same entry
   * point the upstream esp32s3-eye board uses.
   *
   * NOTE(verify): header/bus wiring comes from the espressif vendor layer.
   */

  ret = board_wlan_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize the wireless subsystem: %d\n",
             ret);
    }
#endif

#ifdef CONFIG_SOMATOSYNC_NFC
  /* PN5180 NFC frontend (SPI3): board glue only in this baseline.
   *
   * The PN5180 protocol driver (NFC forum fast-config provisioning, the
   * "tap-to-provision" path used by the original firmware) is scheduled as
   * follow-up work and is not part of the porting baseline. See
   * README_zh-cn.md §"后续工作".
   */
#endif

  return OK;
}
