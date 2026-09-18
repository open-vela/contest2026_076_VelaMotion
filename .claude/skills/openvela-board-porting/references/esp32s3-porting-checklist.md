# ESP32-S3 板级适配专项清单

> 来源：`vendor_espressif/boards/esp32s3/esp32s3-eye`（openvela 官方已适配的
> ESP32-S3 参考板）逐文件核对结论。ESP32-S3 芯片层位于
> `nuttx/arch/xtensa/src/esp32s3/`。

## 一、可信基线

```
vendor/espressif/boards/esp32s3/esp32s3-eye/
├── Kconfig                 # if ARCH_BOARD_CUSTOM 包裹
├── include/board.h
├── src/
│   ├── Make.defs           # CSRCS 按 CONFIG_* 条件累加
│   ├── esp32s3_boot.c      # esp32s3_board_initialize()
│   ├── esp32s3_appinit.c   # board_app_initialize()
│   ├── esp32s3_bringup.c   # esp32s3_bringup()
│   ├── esp32s3_board_lcd.c # board_lcd_initialize/getdev/uninitialize
│   ├── esp32s3_board_spi.c
│   ├── esp32s3_board_camera.c
│   ├── esp32s3_gpio.c / esp32s3_userleds.c / esp32s3_buttons.c / esp32s3_reset.c
│   └── esp32s3-eye.h
├── configs/openvela/defconfig
└── scripts/                # 链接脚本 + esp-hal-3rdparty 补丁
```

## 二、defconfig 必备骨架

```
CONFIG_ARCH="xtensa"
CONFIG_ARCH_XTENSA=y
CONFIG_ARCH_CHIP="esp32s3"
CONFIG_ARCH_CHIP_ESP32S3=y
CONFIG_ARCH_CHIP_ESP32S3WROOM1N4=y
CONFIG_ARCH_BOARD_COMMON=y
CONFIG_ARCH_BOARD_CUSTOM=y
CONFIG_ARCH_BOARD_CUSTOM_DIR="../vendor/espressif/boards/esp32s3/<板名>"
CONFIG_ARCH_BOARD_CUSTOM_DIR_RELPATH=y
CONFIG_ARCH_BOARD_CUSTOM_NAME="<板名>"
CONFIG_SMP=y
CONFIG_SMP_NCPUS=2
CONFIG_ESP32S3_USBSERIAL=y          # 控制台
CONFIG_ESP32S3_SPIRAM=y
CONFIG_ESP32S3_SPIRAM_MODE_OCT=y
CONFIG_RAM_START=0x20000000
CONFIG_RAM_SIZE=114688
CONFIG_MM_REGIONS=2
CONFIG_BOARD_LOOPSPERMSEC=16717
CONFIG_ARCH_INTERRUPTSTACK=2048
CONFIG_INTELHEX_BINARY=y
```

## 三、芯片层 API（已核实签名）

```c
/* arch/xtensa/src/esp32s3/esp32s3_gpio.h */
int  esp32s3_configgpio(uint32_t pin, gpio_pinattr_t attr);
void esp32s3_gpiowrite(int pin, bool value);
bool esp32s3_gpioread(int pin);
void esp32s3_gpioirqenable(int irq, gpio_intrtype_t intrtype);
void esp32s3_gpioirqdisable(int irq);
```

属性宏：`INPUT` / `OUTPUT` / `PULLUP` / `PULLDOWN` / `OPEN_DRAIN`；
中断类型：`RISING` / `FALLING` / `CHANGE` / `ONLOW` / `ONHIGH`。
合法引脚：`pin <= 21 || (26 <= pin < 49)`。

总线初始化（参考板用法）：

```c
FAR struct i2c_master_s *esp32s3_i2cbus_initialize(int port);
FAR struct spi_dev_s   *esp32s3_spibus_initialize(int port);
```

## 四、SSD1306 OLED（图形能力落地）

`nuttx/include/nuttx/lcd/ssd1306.h`：

```c
/* CONFIG_LCD_SSD1306_I2C 时签名如下（SPI 时首参为 struct spi_dev_s *） */
FAR struct lcd_dev_s *ssd1306_initialize(FAR struct i2c_master_s *dev,
                                         FAR const struct ssd1306_priv_s *board_priv,
                                         unsigned int devno);
```

`ssd1306_priv_s` 只有一个成员：`bool (*set_vcc)(bool on)`，无电源门控时传 `NULL`。

所需 Kconfig：

```
CONFIG_LCD=y
CONFIG_LCD_DEV=y
CONFIG_LCD_FRAMEBUFFER=y
CONFIG_LCD_MAXCONTRAST=255
CONFIG_LCD_MAXPOWER=1
CONFIG_LCD_UG2864HSWEG01=y      # 128x64 SSD1306 模块（select LCD_SSD1306）
CONFIG_LCD_SSD1306_I2C=y        # 接口 choice 默认是 SPI，必须显式选 I2C
CONFIG_SSD1306_I2CADDR=60       # 7 位 0x3c
CONFIG_SSD1306_I2CFREQ=400000
```

注册进设备树：`board_lcd_initialize()` → `lcddev_register(0)` → `/dev/lcd0`。
`CONFIG_LCD_FRAMEBUFFER=y` 时同时得到标准 framebuffer，LVGL / fb demo 免改动。

## 五、W5500 以太网

驱动在主线 NuttX 里已存在，**不需要自己写**：

```
nuttx/drivers/net/w5500.c
nuttx/include/nuttx/net/w5500.h
```

```c
int w5500_initialize(FAR struct spi_dev_s *spi_dev,
                     FAR const struct w5500_lower_s *lower,
                     unsigned int devno);
```

板级只需实现 `struct w5500_lower_s` 的四个字段 + 三个回调：

```c
struct w5500_lower_s {
  uint32_t frequency;   /* SPI_SETFREQUENCY() */
  uint16_t spidevid;    /* SPIDEV_ETHERNET(n) */
  enum spi_mode_e mode; /* SPI_SETMODE() */
  int  (*attach)(FAR const struct w5500_lower_s *lower, xcpt_t handler, FAR void *arg);
  void (*enable)(FAR const struct w5500_lower_s *lower, bool enable);
  void (*reset)(FAR const struct w5500_lower_s *lower, bool reset);
};
```

需要 `CONFIG_NET_W5500=y`。

## 六、SDMMC

`board_sdmmc_initialize()` 由 esp32s3 芯片层提供（参考板直接调用，
板目录内没有该文件）。引脚由 defconfig 给出：

```
CONFIG_ESP32S3_SDMMC=y
CONFIG_ESP32S3_SDMMC_CMD=<CMD 引脚>
CONFIG_SDIO_WIDTH_D1_ONLY=y
CONFIG_MMCSD=y
CONFIG_FS_FAT=y
CONFIG_FAT_LFN=y
```

## 七、易错点

| 易错点 | 后果 | 正确做法 |
| --- | --- | --- |
| 板目录放在 `vendor/openvela/boards/` 等非芯片仓库位置 | Kconfig 与板级源码都加载不到 | 放在 `vendor/<厂商>/boards/<arch>/<chip>/` |
| 把 `<chip>_board_initialize()` 改名成板名 | 链接失败 | 符号名由芯片层固定 |
| `LCD_SSD1306` 接口 choice 不显式选 I2C | 按 SPI 编译，I2C 板无法驱动 | `CONFIG_LCD_SSD1306_I2C=y` |
| 手动改 `.config` 后不回写 defconfig | 提交的 defconfig 与实际构建不一致 | `make menuconfig` → `make savedefconfig` |
| 用 `trunk` / `dev` 分支编译 | 芯片层依赖缺失，链路报错 | 用 `dev-ai-contest-2026` |
| `make distclean` 后绕过 `build.sh` 直接编译 | `spinlock_init` 重复定义链接失败 | 始终用 `build.sh` |
| 单个外设初始化失败就 `return ret` | 整板起不来，无法定位 | 只记日志，继续 bring-up |
