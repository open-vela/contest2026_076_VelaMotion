---
name: openvela-board-porting
description: 把一块新的 MCU/MPU 开发板适配到 openvela（基于 NuttX）。当任务涉及「新硬件平台适配」「板级 bring-up」「写 board 目录 / Kconfig / defconfig / linkfile 映射」「让 openvela 在一块尚未支持的板子上启动」「板级外设驱动 glue（UART 控制台 / SPI / I2C / SDMMC / Ethernet / LCD）」时使用本技能。不适用于纯应用层开发或已在 openvela 支持列表中的板卡。
---

# openvela 板级适配

将一块尚未被 openvela 支持的开发板带到「能启动、串口控制台可用、基础外设可验证」
的状态。核心原则：**先找到一块已适配的同芯片/同架构板作为可信基线，再只改板级差异**，
绝不从零发明工程约定。

## 何时使用

- 需要为一块新板卡创建 `boards/<arch>/<chip>/<board>/` 目录
- 需要写板级 `Kconfig` / `defconfig` / `board.h` / bring-up 源码
- 需要在专属仓里用 manifest `<linkfile>` 把板级代码映射进编译树
- 板子能编译但起不来（卡在 boot、无控制台、外设不 ACK）

## 步骤

### 第 1 步：确定芯片层是否已就绪（决定工作量）

先查 openvela 是否已支持该 **芯片**（不是板）：

- `vendor_<厂商>` 仓库的 `boards/<arch>/<chip>/` 下是否已有其它板？
- `nuttx/arch/<arch>/src/<chip>/` 是否存在？

| 情况 | 工作量 | 说明 |
| --- | --- | --- |
| 芯片已支持，板是新的 | 小（板级） | 本次适配的典型情况。芯片层 BSP 复用，只写板级 |
| 芯片未支持 | 大（芯片级） | 需要 `arch/<arch>/src/<chip>/` + 时钟/中断/启动，属芯片移植，不是板级 |

**不要**把「芯片已支持」当成「没有工作量」：板级引脚、供电时序、外设组合
才是这块板的真实差异。

### 第 2 步：克隆一块已适配的参考板作为基线

```bash
# 找出同芯片已适配的板
ls vendor_<厂商>/boards/<arch>/<chip>/
```

逐文件抄读参考板的：`Kconfig`、`configs/*/defconfig`、`src/`（boot / bringup /
appinit / 各外设 board glue）、`include/board.h`。**这是唯一可信的约定来源**，
比任何文档都准。

### 第 3 步：确认板级配置的加载机制（最容易踩的坑）

`ARCH_BOARD_CUSTOM` 型板卡不是靠目录名被发现的，而是靠 `defconfig` 里指向板目录：

```
CONFIG_ARCH_BOARD_CUSTOM=y
CONFIG_ARCH_BOARD_CUSTOM_DIR="../vendor/<厂商>/boards/<arch>/<chip>/<板名>"
CONFIG_ARCH_BOARD_CUSTOM_DIR_RELPATH=y
CONFIG_ARCH_BOARD_CUSTOM_NAME="<板名>"
```

因此 **板目录必须与芯片支持放在一起**（`vendor/<厂商>/boards/<arch>/<chip>/`），
而不是随便找个 `vendor/` 下的位置 —— 否则 `Kconfig` 与板级源码都加载不到。

板级 `Kconfig` 用 `if ARCH_BOARD_CUSTOM` 包裹。

### 第 4 步：manifest `<linkfile>` 映射

参赛场景下板级代码写在专属仓里，用 manifest 软链进编译树：

```xml
<project path="contest2026_<编号>_<队名>" name="contest2026_<编号>_<队名>">
  <linkfile src="board/<板名>" dest="vendor/<厂商>/boards/<arch>/<chip>/<板名>"/>
</project>
```

`src` 是专属仓内目录，`dest` 必须与第 3 步的
`CONFIG_ARCH_BOARD_CUSTOM_DIR` 一致。

### 第 5 步：写板级源码

按参考板的文件切分，不要自创结构：

| 文件 | 职责 |
| --- | --- |
| `<chip>_boot.c`（或等价名） | `<chip>_board_initialize()` —— 符号名由芯片层固定，**不可改名**；`board_late_initialize()` 转发到 bringup |
| `..._appinit.c` | `board_app_initialize()`，`CONFIG_BOARDCTL` 时经 `BOARDIOC_INIT` 调用 |
| `..._bringup.c` | 逐个外设初始化；单个外设失败只记日志，不中断整体 bring-up |
| `..._board_<外设>.c` | 每个外设一个文件，与参考板同名同职责 |
| `include/board.h` | 引脚映射，多板型用条件编译区分 |

**引脚来源必须是原理图或原厂固件，不能猜。** 在文件头注明来源与复核要求。

### 第 6 步：核对 API，不要生成不存在的接口

对每个外设，先在 `nuttx/` 里确认驱动与注册函数真实存在：

```
grep -rn "<外设关键字>" nuttx/drivers/ nuttx/include/nuttx/
```

常见误区：
- 以为某颗芯片（如 W5500）没有驱动而准备自己写 —— 先 `ls nuttx/drivers/net/`
- 使用记忆中「应该有」的函数名，实际签名不同 —— 必须打开头文件核对

### 第 7 步：编译

```bash
cd <openvela 工作区根>
./build.sh vendor/<厂商>/boards/<arch>/<chip>/<板名>/configs/<配置名> -j$(nproc)
```

注意分支依赖：芯片层依赖往往只在特定分支（如 `dev-ai-contest-2026`）齐备，
`trunk` / `dev` 可能**编译不过**，这不是你的代码问题。

**首次编译前务必先做这四件事**（否则会出现「看起来能跑其实什么都没编」和
一类链接错误，实测踩过，详见 `references/esp32s3-porting-checklist.md` 第八节）：

1. **显式把目标工具链加进 PATH**。`build.sh` 不 source `envsetup.sh`，
   而 `envsetup.sh` 里的通用 glob 也匹配不到 `xtensa-esp32s3-elf` 这种名字。
   不加的后果是 `make` 空转不报错 —— 用
   `find nuttx -name "*.o" -newermt "-3 minutes" | wc -l` 验证是否为 0。
2. **板目录放好 `scripts/Make.defs`**，否则 `configure.sh` 直接中止。
3. **克隆完成后手工打 esp-hal-3rdparty 的 patch**（上游文档称会自动应用，是错的）；
   每次切换板型都要重打。
4. **确认 `CONFIG_TLS_TASK_NELEM > 0`**（开了 Wi-Fi 就必须有），
   以及板级提供了 `esp32s3_spi<N>_status()`。

### 第 8 步：烧录与验证

按芯片的启动方案烧录，然后**逐外设验证**，不要只看「能启动」：

```
nsh> uname -a          # 板名出现在版本串里
nsh> ls /dev           # 期望的设备节点是否都出现
nsh> i2c dev 0x03 <addr>   # I2C 器件是否 ACK
```

把验证命令逐条写进板级 README，让评审可复现。

## 自检清单

- [ ] 参考板基线已选定，且能说清每处差异的理由
- [ ] `ARCH_BOARD_CUSTOM_DIR` 与实际映射位置一致
- [ ] `<chip>_board_initialize()` 符号名与芯片层要求一致
- [ ] 每个引脚都有来源（原理图 / 原厂固件），并在文件头标注复核要求
- [ ] 每个驱动的注册函数都在 `nuttx/` 里核对过签名
- [ ] `defconfig` 用 `make menuconfig` + `make savedefconfig` 回写，与实际构建一致
- [ ] 单个外设失败不会导致整板起不来
- [ ] 目标工具链已显式加入 PATH，且 `find nuttx -name "*.o" -newermt "-3 minutes" | wc -l` 不为 0
- [ ] 板目录含 `scripts/Make.defs`，`configure.sh` 能走完
- [ ] esp-hal-3rdparty patch 已应用（切板型后已重打）
- [ ] 开了 Wi-Fi 时 `CONFIG_TLS_TASK_NELEM > 0`
- [ ] 板级提供 `esp32s3_spi<N>_status()`（否则链接失败）
- [ ] 板级 README 含编译 / 烧录 / 逐外设验证 / 已知限制

## 参考

- `references/esp32s3-porting-checklist.md` —— ESP32-S3 板级适配的专项清单与实测结论
- openvela 官方：[芯片移植指南](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/chip_porting/porting_guide.md)
- 可信范例：`vendor_espressif/boards/esp32s3/esp32s3-eye`
