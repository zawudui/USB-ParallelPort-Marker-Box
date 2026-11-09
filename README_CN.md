# USB-DB25 EEG 事件标记盒

**面向 EEG / ERP 与神经科学实验的 USB-to-DB25 开源事件标记硬件。**

由 **zawudui** 开发并通过个人账号发布。此个人源码快照基于此前整理版，保留第三方来源及许可证。本仓库整理了 r7.2 硬件设计、STM32F411 固件、MATLAB 上位机、制造文件、外壳 CAD 和测试说明。

[English README](README.md) · [个人仓库](https://github.com/zawudui/USB-ParallelPort-Marker-Box)

> **当前状态：工程样机 / 开源候选版本。** 本设备是科研实验辅助硬件，不是医疗器械，也没有取得医疗器械或整机安全认证。连接具体 EEG/DAQ 设备前，必须核对接收设备的 DB25 引脚、电气阈值、极性、最大输入电压和脉宽要求。

## 设备功能

- USB-C 连接电脑，固件实现 USB Full-Speed CDC。
- DB25 母座提供 8 位高有效数字事件输出。
- STM32F411 控制器和二进制命令协议。
- 输出侧使用隔离电源及光耦隔离架构。
- 提供 MATLAB 客户端、示例程序和协议说明。
- 提供 KiCad 原理图/PCB、Gerber、钻孔、BOM 和 3D 外壳文件。

## DB25 引脚定义

| DB25 引脚 | 信号 | 说明 |
|---|---|---|
| 2–9 | D0–D7 | 8 位高有效 Marker 输出，D0 为最低位 |
| 18–25 | GND_ISO | 隔离侧信号参考地 |
| 1、10–17 | NC | 不连接 |

DB25 接口并不代表所有采集设备都采用相同触发定义。请以接收设备手册为准，并在接入正式实验系统前使用假负载完成验证。

## 2026-11-09 样机测试摘要

目前有一块 r7.2 工程样机完成了台架测试：

- 100 次命令/ACK 往返全部成功。
- 主机写入至完整 ACK 的往返时间：0.1796–0.3522 ms。
- 单次 D1 输出脉冲测得约 2.002 ms。
- 一次 MCU 输出与 DB25 输出边沿比较约 31 ns；未校准探头和通道偏斜。

这些是有限条件下的实测观察，不是单向端到端延迟、统计抖动、长期漏码率或整机认证指标。完整限制见 [测试范围说明](docs/validation/README.md)。

## 文件导航

- [硬件设计 r7.2](hardware/r7.2/)
- [KiCad 工程](hardware/r7.2/kicad/)
- [Gerber、钻孔、BOM 和装配图](hardware/r7.2/manufacturing/)
- [固件及构建方法](firmware/)
- [MATLAB 客户端和示例](tools/matlab/)
- [r7.2 无螺丝 PA12 卡扣外壳](hardware/enclosure/r7.2/snap-fit-pa12/)
- [DB25 引脚定义](docs/hardware/db25-pinout.md)
- [电气接口与验证限制](docs/hardware/electrical-specifications.md)
- [固件通信协议](docs/firmware/protocol.md)

## 固件构建

主机端测试：

    cmake -S firmware -B build
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

STM32F411 目标固件：

    cmake -S firmware/target/stm32f411 -B build-target \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release
    cmake --build build-target --parallel

注意：当前固件 USB VID/PID 0x0483:0x5740 是 ST CDC 示例占位值，不是分配给本项目的产品标识。正式商业分发前必须解决 VID/PID 授权与分配问题。

## 开源许可证

- 硬件设计文件：CERN-OHL-S-2.0，受第三方材料排除条款约束。
- 固件和上位机软件：MIT。
- 原创技术文档：CC BY 4.0。
- 第三方资料仍适用其各自许可证，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 安全与验证边界

当前证据不证明整机已经通过 ESD、EMC、耐压、隔离、USB 电流/浪涌或长期可靠性测试。元器件数据手册中的单器件额定值不能替代整机验证。

当前选用的外壳是 **r7.2 C 型 PA12 双件免螺丝卡扣方案**，包含底盖、上盖、装配 STEP、两件 STL 打印包和原始 CAD 生成脚本；早期 FIRST-FIT 螺丝固定外壳仅作为历史参考。CAD 几何校验已记录，但卡扣寿命、实际装配手感、USB-C 接头、DB25 插头外壳与锁紧螺钉间隙均仍需实物复核；尚不属于已完成量产验证的结构。

**请先在假负载和受控台架条件下测试，不要未经核对就连接昂贵的 EEG/DAQ 采集设备。**

本次提供源码候选版，不附正式量产固件或私有 Git 历史。上传前运行 `python tools/release_audit.py`，并阅读 [发布检查](docs/release/RELEASE_GATE.md) 和 [个人仓库上传步骤](docs/release/PERSONAL_REPO_MIGRATION.md)。实体试装、整机资格验证及正式 USB 标识仍分别记录为未完成。
