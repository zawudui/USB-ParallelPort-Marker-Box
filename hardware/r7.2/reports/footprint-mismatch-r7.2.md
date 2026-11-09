# r7.2 封装库差异归类（KiCad 9.0.9 DRC）

来源：`usb_db25_marker-drc-r7.2.txt`，2026-10-20T06:43:20Z，`--schematic-parity --severity-all`。共 **87** 条 `lib_footprint_mismatch`，0 error，0 unconnected，0 schematic parity。

比对方法：用 KiCad 9.0.9 的 `pcbnew` 读取板上焊盘编号、相对封装锚点的坐标、焊盘尺寸和钻孔，再与 `/usr/share/kicad/footprints` 中同名封装比较。差异只记铜皮相关项。丝印、courtyard 和文本不单独展开。

本次新放上的器件不在这 87 条里：U20、L201、D201、U18、U19、Q3、C20、C201–C207、R90–R98、TP201–TP203。它们用的是当前库封装。不要为了消警告去整库刷新；下面两条例外会改铜皮。

## 焊盘几何与库一致（85）

警告来自丝印、courtyard 或文本，不来自焊盘编号、坐标、尺寸或钻孔。继续使用板上的这份封装。从库更新会改丝印，并可能让 courtyard 撞上相邻器件。

| 库封装 | 数量 | 位号 |
|---|---|---|
| `Resistor_SMD:R_0603_1608Metric` | 52 | R10–R18、R20–R28、R30–R37、R40–R47、R50–R52、R60、R61、R70–R72、R80–R89 |
| `Capacitor_SMD:C_0603_1608Metric` | 23 | C1–C7、C10、C11、C21–C25、C30–C37、C40 |
| `Package_SO:SOIC-14_3.9x8.7mm_P1.27mm` | 4 | U12、U13、U15、U16 |
| `Package_TO_SOT_SMD:SOT-23` | 2 | Q1、Q2 |
| `Package_TO_SOT_SMD:SOT-23-5` | 1 | U14 |
| `Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical` | 1 | J3 |
| `Diode_SMD:D_0402_1005Metric` | 1 | D1 |
| `Package_DFN_QFN:QFN-48-1EP_7x7mm_P0.5mm_EP5.6x5.6mm` | 1 | U1 |

风险：低，只要不执行 “Update Footprint from Library”。这 85 只是 r7.1 留下的本地副本，r7.2 没有改它们的焊盘。

## 焊盘几何有意不同（2）

### J1 `USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal`

只有四只角焊盘和库不同。信号焊盘一致。

| 焊盘 | 板上 | KiCad 9.0.9 库 |
|---|---|---|
| A1、A12、B1、B12 | 0.60 × **1.10** mm | 0.60 × 1.15 mm |

Y 向中心也因此差 0.025 mm。这是 r7.1 为了满足 0.25 mm 孔铜间距，把角焊盘从 1.15 mm 收到 1.10 mm 的结果。不能用库封装覆盖。

风险：若刷新成库焊盘，角焊盘加长，可能重新出现孔铜间距错误。维持板上的 1.10 mm。

### U17 `SOT-323_SC-70`（TPD2E2U06DCKR）

三只焊盘按 TI DCK0003A（附图 4220745/F）的推荐焊盘改过，仍然和 KiCad 库不同。焊盘在封装内旋转 270°，下表是转到封装坐标后的铜皮：长度沿 X，宽度沿 Y。

| 焊盘 | 现板 | TI DCK 推荐 | KiCad 9.0.9 库 |
|---|---|---|---|
| 1、2、3 | 0.95 × 0.40 mm，中心 x = ±1.10 mm | 0.95 × 0.40 mm，左右中心距 2.20 mm | 1.225 × 0.50 mm，中心 x = ±0.9375 mm |

局部阻焊比铜皮大 0.05 mm，落在 TI 写的 NSMD 四周最多 0.07 mm 以内。不要从库刷新。库焊盘更长，会改变 USB D+/D− 旁边的铜皮。

风险：低到中。和库不一致是有意的；风险在误刷新，不在焊盘比推荐图短。J1 的四只角焊盘仍是 1.10 mm，这次没有动。
