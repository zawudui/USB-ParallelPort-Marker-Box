#pragma once
/**
 * GPIO map for the USB-to-DB25 Marker board, hardware revision r7.2.
 * Pin map used by this firmware: PB0–PB7 data, PB8 READY, PA11/PA12 USB,
 * PA13/PA14 SWD, PA15 ISO_PWR_EN, PH0/PH1 HSE. No pin conflict is recorded
 * against the r7.2 manufacturing package.
 * Manufacturing source for the power path (not this header): PR #10 branch
 * cursor/r72-pcb-resync-1f60, BOM hardware/r7.2/fab-review/bom/
 * usb_db25_marker-bom-r7.2-by-ref.csv — R98 = 1 kΩ, R99 is not fitted.
 * docs/r7.2/设计规范-r7.2.md is the design narrative and now matches that.
 *
 * MARKER_D0..D7 : PB0..PB7 (same port for atomic BSRR writes)
 * MARKER_READY  : PB8 (internal optical-output gate; not a DB25 pin)
 * USB_DM/DP     : PA11 / PA12 (STM32 USB FS)
 * SWDIO/SWCLK   : PA13 / PA14 (must remain available for programming/debug)
 * ISO_PWR_EN    : PA15 (active high; keep low until USB is configured and
 *                  the provisional power-up sequence has elapsed)
 */
#include <stdint.h>
#define MKR_GPIO_PORT_DATA   /* GPIOB */ 0
#define MKR_PIN_D0           0u  /* PB0 */
#define MKR_PIN_D1           1u
#define MKR_PIN_D2           2u
#define MKR_PIN_D3           3u
#define MKR_PIN_D4           4u
#define MKR_PIN_D5           5u
#define MKR_PIN_D6           6u
#define MKR_PIN_D7           7u
#define MKR_PIN_READY        8u  /* PB8 */
#define MKR_DATA_MASK        0x00FFu
#define MKR_READY_MASK       (1u << MKR_PIN_READY)
/* r7.2: active-high control for Q1 upstream of the boost converter. */
#define MKR_ISO_PWR_EN_PORT  /* GPIOA */ 0
#define MKR_PIN_ISO_PWR_EN   15u  /* PA15 */
