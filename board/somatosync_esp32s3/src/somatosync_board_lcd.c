/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_board_lcd.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SSD1306 128x64 mono OLED on I2C0.
 *
 * This is the graphics-capability landing point of the entry: the panel is
 * registered as a NuttX LCD device (CONFIG_LCD_DEV) and, with
 * CONFIG_LCD_FRAMEBUFFER, is also usable through the standard framebuffer
 * interface — so LVGL and fb-based demos run on this board unmodified.
 *
 * APIs used here are the ones the upstream esp32s3-eye board uses:
 *   esp32s3_i2cbus_initialize()  - I2C0 master, pins from defconfig
 *   ssd1306_initialize()         - include/nuttx/lcd/ssd1306.h
 *   lcddev_register()            - exposes /dev/lcd0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/ssd1306.h>

#include <arch/board/board.h>

#include "esp32s3_gpio.h"
#include "esp32s3_i2c.h"

#include "somatosync_esp32s3.h"

#ifdef CONFIG_SOMATOSYNC_OLED

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct i2c_master_s *g_i2c;
static FAR struct lcd_dev_s *g_lcd;

/* The wearable node gates the panel power rail through a MOSFET, so it can
 * actually switch the display off between TDMA frames. The gateway has no
 * such gate and leaves set_vcc unset (NULL).
 */

#if defined(CONFIG_SOMATOSYNC_BOARD_NODE)

static bool somatosync_oled_set_vcc(bool on)
{
  esp32s3_gpiowrite(BOARD_OLED_PWR_EN_PIN, on);
  return true;
}

static const struct ssd1306_priv_s g_oled_priv =
{
  .set_vcc = somatosync_oled_set_vcc
};

#  define SOMATOSYNC_OLED_PRIV (&g_oled_priv)

#else

#  define SOMATOSYNC_OLED_PRIV NULL

#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_lcd_initialize
 *
 * Description:
 *   Initialize the OLED video hardware and switch the panel on.
 *
 *   The panel is fully initialised with its display memory cleared, then
 *   powered at CONFIG_LCD_MAXPOWER so that /dev/lcd0 is immediately usable
 *   once NSH comes up.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_lcd_initialize(void)
{
  int ret;

#if defined(CONFIG_SOMATOSYNC_BOARD_NODE)
  /* Node only: bring the panel power rail up and pulse the reset line */

  esp32s3_configgpio(BOARD_OLED_PWR_EN_PIN, OUTPUT);
  esp32s3_configgpio(BOARD_OLED_RESET_PIN, OUTPUT);

  somatosync_oled_set_vcc(true);
  up_mdelay(10);

  esp32s3_gpiowrite(BOARD_OLED_RESET_PIN, false);
  up_mdelay(10);
  esp32s3_gpiowrite(BOARD_OLED_RESET_PIN, true);
  up_mdelay(10);
#endif

  g_i2c = esp32s3_i2cbus_initialize(0);
  if (g_i2c == NULL)
    {
      lcderr("ERROR: Failed to get I2C0 bus for the SSD1306\n");
      return -ENODEV;
    }

  g_lcd = ssd1306_initialize(g_i2c, SOMATOSYNC_OLED_PRIV, 0);
  if (g_lcd == NULL)
    {
      lcderr("ERROR: ssd1306_initialize() failed\n");
      return -ENODEV;
    }

  /* Panel on, full contrast (SSD1306_MAXCONTRAST) */

  ret = g_lcd->setpower(g_lcd, CONFIG_LCD_MAXPOWER);
  if (ret < 0)
    {
      lcderr("ERROR: Failed to power on the SSD1306: %d\n", ret);
      return ret;
    }

  g_lcd->setcontrast(g_lcd, CONFIG_LCD_MAXCONTRAST);

  return OK;
}

/****************************************************************************
 * Name: board_lcd_getdev
 *
 * Description:
 *   Return a reference to the LCD object for the specified LCD.
 *
 ****************************************************************************/

FAR struct lcd_dev_s *board_lcd_getdev(int devno)
{
  if (g_lcd == NULL)
    {
      lcderr("ERROR: OLED %d not bound\n", devno);
      return NULL;
    }

  return g_lcd;
}

/****************************************************************************
 * Name: board_lcd_uninitialize
 *
 * Description:
 *   Uninitialize the LCD support.
 *
 ****************************************************************************/

void board_lcd_uninitialize(void)
{
  if (g_lcd != NULL)
    {
      g_lcd->setpower(g_lcd, 0);
    }

#if defined(CONFIG_SOMATOSYNC_BOARD_NODE)
  somatosync_oled_set_vcc(false);
#endif
}

#endif /* CONFIG_SOMATOSYNC_OLED */
