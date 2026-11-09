#ifndef MKR_USB_DCD_H
#define MKR_USB_DCD_H
/*
 * USB OTG FS device, CDC ACM, slave mode. Not enumerated on hardware.
 * PA11/PA12 are set to AF10 here. PA13/PA14 (SWD) and PA15 are not touched.
 * PA9 is not connected on this board, so VBUS sensing is forced off (F411
 * GCCFG.NOVBUSSENS). Remote wakeup is not implemented.
 *
 * SET_CONFIGURATION is committed only after the Status IN completes
 * (USB 2.0: Configured state follows the status stage). Until that
 * completion, data endpoints stay inactive and USB_CONFIGURED is not
 * emitted, so the ISO-power countdown does not start. A new SETUP, a
 * bus reset, or an EP0 timeout cancels the pending commit.
 */
#include <stdbool.h>
#include <stdint.h>
#include "power_sequence.h"
typedef void (*mkr_usb_event_fn)(mkr_power_event_t event, void *ctx);
typedef void (*mkr_usb_rx_fn)(const uint8_t *data, uint16_t len, void *ctx);
typedef struct {
    mkr_usb_event_fn on_event;
    mkr_usb_rx_fn on_rx;
    void *ctx;
} mkr_usb_dcd_cbs_t;
/* Call after the 84 MHz PLL is running and before the 1 ms SysTick. */
bool mkr_usb_dcd_start(const mkr_usb_dcd_cbs_t *cbs);
/* Queue a CDC IN packet. Returns 0 on acceptance, -1 if the queue is full or
 * the device is not configured. Safe to call from the USB IRQ. */
int mkr_usb_dcd_write(const uint8_t *data, uint16_t len);
/* Soft-disconnect. No-op until the USB AHB clock has been enabled. */
void mkr_usb_dcd_disconnect(void);
/* Set after markers/READY and PA15 are turned off for USB suspend.
 * Cleared on resume or bus reset. Not a measured-current flag. */
bool mkr_usb_dcd_stop_requested(void);
/* EXTI line 18 woke the core. Ungate the PHY. If Stop was requested and
 * this is not a bus reset, emit resume. Does not touch ISO power. */
void mkr_usb_dcd_on_exti_wakeup(void);
void OTG_FS_IRQHandler(void);
#ifdef MKR_USB_HOST_TEST
/* RAM-backed driver checks. Not in the review image. */
bool mkr_usb_dcd_test_configured(void);
bool mkr_usb_dcd_test_inflight_wait(uint8_t ep);
bool mkr_usb_dcd_test_in_busy(uint8_t ep);
uint16_t mkr_usb_dcd_test_tx_count(void);
bool mkr_usb_dcd_test_bulk_zlp(void);
uint8_t mkr_usb_dcd_test_cfg_commit(void);
#endif
#endif
