/****************************************************************************
 * contest2026_076_VelaMotion/board/somatosync_esp32s3/src/somatosync_board_eth.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * WIZnet W5500 hardwired-TCP/IP Ethernet controller on SPI.
 *
 * The W5500 protocol driver already exists in openvela's NuttX
 * (drivers/net/w5500.c, include/nuttx/net/w5500.h), so this file only has to
 * supply the board glue: SPI bus, reset line, interrupt line.
 *
 *   Gateway: W5500 on SPI2 — MOSI 11 / MISO 12 / SCLK 13 / CS 14,
 *            INTn 10, RSTn 9. Carries the synchronised body-sensor frames to
 *            the ROS 2 host over Ethernet.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/spi/spi.h>
#include <nuttx/net/w5500.h>

#include <arch/board/board.h>

#include "esp32s3_gpio.h"
#include "esp32s3_spi.h"

#include "somatosync_esp32s3.h"

#ifdef CONFIG_SOMATOSYNC_W5500

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Interrupt request number of the W5500 INTn pin.
 *
 * NOTE(verify): ESP32S3_PIN2IRQ() is the pin-to-IRQ mapping macro of the
 * esp32s3 chip layer. If the name differs in your checkout, this is the one
 * line to fix — see arch/xtensa/src/esp32s3/hardware/ and esp32s3_irq.h.
 */

#define SOMATOSYNC_W5500_IRQ  ESP32S3_PIN2IRQ(BOARD_W5500_INT_PIN)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  somatosync_w5500_attach(FAR const struct w5500_lower_s *lower,
                                    xcpt_t handler, FAR void *arg);
static void somatosync_w5500_enable(FAR const struct w5500_lower_s *lower,
                                    bool enable);
static void somatosync_w5500_reset(FAR const struct w5500_lower_s *lower,
                                   bool reset);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct w5500_lower_s g_w5500_lower =
{
  .frequency = CONFIG_SOMATOSYNC_W5500_SPI_FREQUENCY,
  .spidevid  = SPIDEV_ETHERNET(0),
  .mode      = SPIDEV_MODE0,
  .attach    = somatosync_w5500_attach,
  .enable    = somatosync_w5500_enable,
  .reset     = somatosync_w5500_reset,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: somatosync_w5500_attach
 *
 * Description:
 *   Attach the W5500 interrupt handler to the INTn pin.
 *
 ****************************************************************************/

static int somatosync_w5500_attach(FAR const struct w5500_lower_s *lower,
                                   xcpt_t handler, FAR void *arg)
{
  /* The pin itself is configured once in somatosync_w5500_initialize();
   * here we only bind the handler to the pin's IRQ.
   */

  return irq_attach(SOMATOSYNC_W5500_IRQ, handler, arg);
}

/****************************************************************************
 * Name: somatosync_w5500_enable
 *
 * Description:
 *   Enable or disable the W5500 interrupt.
 *
 ****************************************************************************/

static void somatosync_w5500_enable(FAR const struct w5500_lower_s *lower,
                                    bool enable)
{
  if (enable)
    {
      esp32s3_gpioirqenable(SOMATOSYNC_W5500_IRQ, FALLING);
    }
  else
    {
      esp32s3_gpioirqdisable(SOMATOSYNC_W5500_IRQ);
    }
}

/****************************************************************************
 * Name: somatosync_w5500_reset
 *
 * Description:
 *   Drive the W5500 RSTn pin. The controller is held in reset while
 *   'reset' is true (RSTn is active low).
 *
 ****************************************************************************/

static void somatosync_w5500_reset(FAR const struct w5500_lower_s *lower,
                                   bool reset)
{
  esp32s3_gpiowrite(BOARD_W5500_RST_PIN, !reset);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: somatosync_w5500_initialize
 *
 * Description:
 *   Configure the INTn / RSTn pins, allocate the SPI bus and register the
 *   W5500 network driver.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

int somatosync_w5500_initialize(void)
{
  FAR struct spi_dev_s *spi;
  int ret;

  /* RSTn as output, INTn as pulled-up input */

  esp32s3_configgpio(BOARD_W5500_RST_PIN, OUTPUT);
  esp32s3_configgpio(BOARD_W5500_INT_PIN, INPUT_PULLUP);

  /* Hardware reset: assert, hold, release */

  somatosync_w5500_reset(&g_w5500_lower, true);
  up_mdelay(10);
  somatosync_w5500_reset(&g_w5500_lower, false);
  up_mdelay(50);

  spi = esp32s3_spibus_initialize(CONFIG_SOMATOSYNC_W5500_SPI);
  if (spi == NULL)
    {
      nerr("ERROR: Failed to initialize SPI%d for the W5500\n",
           CONFIG_SOMATOSYNC_W5500_SPI);
      return -ENODEV;
    }

  ret = w5500_initialize(spi, &g_w5500_lower, 0);
  if (ret < 0)
    {
      nerr("ERROR: w5500_initialize() failed: %d\n", ret);
      return ret;
    }

  return OK;
}

#endif /* CONFIG_SOMATOSYNC_W5500 */
