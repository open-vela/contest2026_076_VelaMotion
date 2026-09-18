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
| 切换板型后忘记重打 esp-hal 补丁 | 重新克隆会覆盖补丁，编译报 `invalid initializer` | 每次切板型后重打（见 8.3） |
| 只开 `ESP32S3_WIFI` 不开 `TLS_TASK_NELEM` | 链接报 `undefined reference to task_tls_alloc` | 加 `CONFIG_TLS_TASK_NELEM=4`（见 8.5） |
| 单个外设初始化失败就 `return ret` | 整板起不来，无法定位 | 只记日志，继续 bring-up |

---

## 八、首次构建的六个真实坑（实测记录）

以下六条来自一次**真实完成**的 ESP32-S3 板级适配（构建成功并产出
`nuttx.bin` 可烧录镜像）的完整过程。上游参考板的文档只覆盖了其中一部分，
其余是这个工作区首次构建 ESP32-S3 时才会暴露的问题。

### 8.1 xtensa 工具链不在 PATH，而 make 不会报错

`build.sh` **不 source** `build/envsetup.sh`，而工具链的 PATH 正是在那里设置的
（`build/envsetup.sh` 末尾 `export PATH=$VELA_GLOBAL_BUILD_PATHS:$PATH`）。
更麻烦的是其中的通用 glob：

```bash
for TOOLCHAIN_BIN in $T/prebuilts/gcc/${SYSTEM}-${SYS_ARCH}/${ARCH[@]:$i:1}{,-none}-{eabi,elf}/bin
```

它只匹配 `xtensa-eabi` / `xtensa-elf` / `xtensa-none-eabi` / `xtensa-none-elf`
这类名字，**匹配不到 `xtensa-esp32s3-elf`**。

后果极其隐蔽：终端报 `xtensa-esp32s3-elf-gcc: 未找到命令`，但 `make` 不退出，
继续空转 —— 表现为「编译非常慢」，实际一个目标文件都没产出。用这个判断：

```bash
find nuttx -name "*.o" -newermt "-3 minutes" | wc -l   # 为 0 就是根本没在编译
```

正确做法（显式加入，不要依赖 envsetup.sh）：

```bash
export PATH=$PWD/prebuilts/gcc/linux-x86_64/xtensa-esp32s3-elf/bin:$PATH
```

### 8.2 板目录必须有 `scripts/Make.defs`

`nuttx/tools/configure.sh` 按固定顺序查找 `Make.defs`，其中一条是
`${configpath}/../../scripts/Make.defs`，即 `<板目录>/scripts/Make.defs`。
缺了它配置阶段直接中止：

```
File Make.defs could not be found
Error: ############# config ... fail ##############
```

该文件内容是**芯片级通用**的（工具链 defs + 链接脚本选择），与具体板子无关。

### 8.3 esp-hal-3rdparty 的 patch 必须手工打，而且上游文档写错了

构建会自行克隆 `espressif/esp-hal-3rdparty`，但**落在 `main` 分支**上，
而 `main` 的 `components/esp_hw_support/clk_ctrl_os.c` 里
`LOCK_INITIALIZER_UNLOCKED` 是 `0`，编不过：

```
error: invalid initializer
  static lock_type_t periph_spinlock = LOCK_INITIALIZER_UNLOCKED;
```

补丁把它改成 `SP_UNLOCKED`（`components/esp_hw_support/modem_clock.c` 同）。

> ⚠️ `esp32s3-eye` 的 README 声称 `build.sh` 会在 `distclean` 后自动重新应用
> 该补丁 —— **这是错的**。全树任何 `.sh` / `.mk` / `Make.defs` /
> `CMakeLists.txt` / `*.cmake` 里都没有这段逻辑（已逐文件 grep 确认）。
> 补丁只能手工执行，且要**等克隆完成之后**再打，否则会被覆盖。

```bash
cd nuttx/arch/xtensa/src/esp32s3/esp-hal-3rdparty
git apply -p1 <板目录>/scripts/patches/0001-esp-hal-3rdparty-fix-spinlock-init.patch
```

**每切换一次板型（gateway ↔ node）都会重新克隆并冲掉补丁**，切完必须重打。

### 8.4 板级必须提供 `esp32s3_spi<N>_status()`

芯片层的 SPI ops 表引用了 `esp32s3_spi2_status()`，但这个函数**故意不在芯片层实现**
—— 每块 ESP32-S3 板自己提供（答案取决于板子怎么接片选）。链接期报：

```
undefined reference to `esp32s3_spi2_status'
```

在板目录 `src/` 下加一个文件即可：

```c
#ifdef CONFIG_ESP32S3_SPI2
uint8_t esp32s3_spi2_status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0;
}
#endif
```

`esp32s3-eye` 对应的是 `src/esp32s3_board_spi.c`，但这个文件的作用在
vendor 目录的文档里没有任何提示，**极易漏掉**。

### 8.5 Wi-Fi 需要 `CONFIG_TLS_TASK_NELEM`

esp32s3 的 Wi-Fi 适配层调用 `task_tls_alloc()` / `task_tls_get_value()` /
`task_tls_set_value()`，它们在 `nuttx/libs/libc/tls/task_tls_destruct.c` 中受
`CONFIG_TLS_TASK_NELEM > 0` 保护。只开 `ESP32S3_WIFI` 不够：

```
undefined reference to `task_tls_alloc'
```

加 `CONFIG_TLS_TASK_NELEM=4`（与参考板一致）。

### 8.6 nxtmpdir 缓存会被反复清空 / esptool 版本

**`nxtmpdir` 缓存机制**：`chip/esp-hal-3rdparty` 的内容来自
`NXTMPDIR=$(WSDIR)/nxtmpdir` 缓存，而 `CHECK_COMMITSHA` 用
`git branch --contains <sha>` 判断缓存是否可用 —— **判定失败就 `rm -rf` 整个目录**。

如果用 `git init` + `git fetch --depth=1 <sha>` 来准备缓存（比全量克隆快得多：
294 MB 全量克隆在此网络下会反复中断，而定向 fetch 只要 45 秒 / 200 MB），
那么由于**没有任何分支引用**，`branch --contains` 必然失败，缓存每次都被删掉重来。
解决办法是给它建一个分支：

```bash
git -C $CACHE branch -f keep HEAD     # 让 branch --contains 通过
```

另外：本仓库的构建链条需要 `esptool >= 4.8.0`。Ubuntu 22.04 自带的
**pip 22.0.2 解析不了 esptool ≥4.8.0 的 sdist 元数据**，会报
`has inconsistent name: filename has 'esptool', but metadata has 'unknown'`，
进而声称「找不到匹配版本」。先升级 pip 再装：

```bash
python3 -m pip install --user --upgrade pip
python3 -m pip install --user --upgrade esptool
```

### 8.7 一次成功构建的产物（可作验收基线）

```
esptool v5.4.0
Creating ESP32-S3 image...
Successfully created ESP32-S3 image.
Generated: nuttx.bin

nuttx      ~13.8 MB   ELF（GDB 调试用）
nuttx.hex  ~2.3 MB    Intel-HEX
nuttx.bin  ~855 KB    平面镜像，烧录到偏移 0x0
```

`build.sh` 在构建成功后会执行 `make savedefconfig` 并**把结果回写到 config 目录**，
所以提交的 defconfig 会自动与实际构建保持一致 —— 不需要手工同步，
但也**不要**在构建之后再去手改 defconfig（会被下次构建覆盖）。
