#include "board_clock.h"
#include "board_crit.h"
#include "board_gpio.h"
#include "board_iwdg.h"
#include "board_stop.h"
#include "board_systick.h"
#include "board_time.h"
#include "bringup.h"
#include "bringup_mode.h"
#include "clock_plan.h"
#include "iwdg_plan.h"
#include "runtime.h"
#include "stm32f411xe.h"
#include "usb_descriptors.h"
#include "usb_dcd.h"
#include "usb_only_bringup.h"
#include <stdint.h>
#if MKR_BRINGUP_DIAGNOSTICS
volatile uint32_t g_bringup_stage;
volatile uint32_t g_bringup_last_usb_event;
volatile uint32_t g_bringup_usb_event_count;
volatile uint32_t g_bringup_reset_cause;
volatile uint32_t g_bringup_uptime_ms;
#define DIAG_STAGE(value) (g_bringup_stage = (value))
#else
#define DIAG_STAGE(value) ((void)0)
#endif
/*
 * Bench validation is separate from external-equipment integration.
 * Functional bring-up enables real isolation power and marker outputs.
 * MKR_POWER_ISO_SETTLE_MS is still the provisional 100 ms software guard.
 */
__attribute__((used, section(".mkr_manifest")))
const struct {
    uint32_t magic;
    uint32_t sysclk_hz;
    uint32_t usb_hz;
    uint32_t pllcfgr;
    uint32_t hse_timeout_ms;
    uint32_t iwdg_lsi_counts;
    uint32_t rtc_kick_lsi_counts;
    uint16_t vid;
    uint16_t pid;
    uint8_t bm_attributes;
    uint8_t b_max_power;
    uint8_t hse_bypass;
    uint8_t default_pulse_ms;
    uint8_t ids_placeholder;
    uint8_t reserved[3];
} g_mkr_manifest = {
    0x53323752u,
    MKR_SYSCLK_HZ,
    MKR_USB_HZ,
    MKR_PLLCFGR_VALUE,
    MKR_HSE_TIMEOUT_MS,
    MKR_IWDG_LSI_COUNTS,
    MKR_RTC_KICK_LSI_COUNTS,
    MKR_USB_VID,
    MKR_USB_PID,
    MKR_USB_BM_ATTRIBUTES,
    MKR_USB_B_MAX_POWER,
    0u,
    MKR_DEVICE_DEFAULT_PULSE_MS,
    (uint8_t)MKR_USB_IDS_ARE_PLACEHOLDER,
    {0u, 0u, 0u}};
volatile uintptr_t g_manifest_addr;
static mkr_runtime_t g_app;
static void on_marker(uint8_t code, bool ready, void *ctx)
{
    (void)ctx;
#ifdef MKR_USB_ONLY_BRINGUP
    (void)code;
    (void)ready;
    board_marker_write(0u, false);
#else
    board_marker_write(code, ready);
#endif
}
static void on_iso(bool enabled, void *ctx)
{
    (void)ctx;
#ifdef MKR_USB_ONLY_BRINGUP
    (void)enabled;
    board_iso_power_set(false);
#else
    board_iso_power_set(enabled);
#endif
}
static void on_tx(const uint8_t *data, uint16_t len, void *ctx)
{
    (void)ctx;
    (void)mkr_usb_dcd_write(data, len);
}
static void runtime_init(void)
{
    const mkr_runtime_io_t io = {
        on_marker, on_iso, on_tx, board_crit_lock, board_crit_unlock, 0};
    mkr_runtime_init(&g_app, &io);
    DIAG_STAGE(2u);
}
static bool start_clock(void)
{
    DIAG_STAGE(3u);
    const bool ok = board_clock_start();
    if (ok) { DIAG_STAGE(4u); }
    return ok;
}
static void latch_fault(void)
{
    mkr_runtime_on_usb_event(&g_app, MKR_POWER_EVENT_CLOCK_FAULT);
}
static void on_usb_event(mkr_power_event_t event, void *ctx)
{
    (void)ctx;
#if MKR_BRINGUP_DIAGNOSTICS
    g_bringup_last_usb_event = (uint32_t)event;
    ++g_bringup_usb_event_count;
#endif
#ifdef MKR_USB_ONLY_BRINGUP
    event = mkr_usb_only_filter_event(event);
#endif
    mkr_runtime_on_usb_event(&g_app, event);
}
static void on_usb_rx(const uint8_t *data, uint16_t len, void *ctx)
{
    (void)ctx;
    mkr_runtime_on_rx(&g_app, data, len);
}
static bool start_usb(void)
{
    const mkr_usb_dcd_cbs_t cbs = {on_usb_event, on_usb_rx, 0};
    DIAG_STAGE(5u);
    const bool ok = mkr_usb_dcd_start(&cbs);
    if (ok) { DIAG_STAGE(6u); }
    return ok;
}
static void on_tick(void *ctx)
{
#if MKR_BRINGUP_DIAGNOSTICS
    ++g_bringup_uptime_ms;
#endif
    mkr_runtime_on_tick_1ms(ctx);
}
static void on_marker_deadline(void *ctx)
{
    mkr_runtime_on_marker_deadline((mkr_runtime_t *)ctx);
}
static void start_timebase(void)
{
    board_time_start(on_marker_deadline, &g_app);
    board_systick_start(on_tick, &g_app);
    DIAG_STAGE(7u);
}
static void halt(void)
{
    board_gpio_force_safe_off();
    for (;;) {
#if MKR_BRINGUP_DIAGNOSTICS
        __asm volatile("nop");
#else
        __asm volatile("wfi");
#endif
    }
}
void mkr_target_nmi_clock_fault(void)
{
    board_gpio_force_safe_off();
    mkr_runtime_latch_clock_fault(&g_app);
    mkr_usb_dcd_disconnect();
#if MKR_BRINGUP_DIAGNOSTICS
    for (;;) { __asm volatile("nop"); }
#endif
}
int main(void)
{
    const mkr_bringup_ops_t ops = {
        board_gpio_init_safe, runtime_init, start_clock, latch_fault, start_usb, start_timebase, halt};
    /* Safe outputs before any halt path, including an IWDG reset.
     * PH0/PH1 go analog before HSEON; clock start does this again. */
    board_gpio_init_safe();
#if MKR_BRINGUP_DIAGNOSTICS
    *((volatile uint32_t *)0xE0042004u) |= 7u;
    *((volatile uint32_t *)0xE000ED10u) &= ~6u;
    g_bringup_reset_cause = RCC->CSR;
    RCC->CSR |= (1u << 24); /* Clear reset flags after recording them. */
#if MKR_BENCH_IWDG
    /* Allow debug register reads without manufacturing a watchdog reset.
     * The watchdog continues to run whenever the core is not halted. */
    *((volatile uint32_t *)0xE0042008u) |= (1u << 12);
#endif
#endif
    DIAG_STAGE(1u);
    board_hse_pins_prepare();
    g_manifest_addr = (uintptr_t)&g_mkr_manifest;
    if (!mkr_wakeup_vectors_installed() || g_mkr_manifest.bm_attributes != 0x80u ||
        g_mkr_manifest.b_max_power != 250u || g_mkr_manifest.hse_bypass != 0u ||
        g_mkr_manifest.sysclk_hz != 84000000u || g_mkr_manifest.usb_hz != MKR_USB_HZ ||
        g_mkr_manifest.pllcfgr != MKR_PLLCFGR_VALUE || g_mkr_manifest.vid != MKR_USB_VID ||
        g_mkr_manifest.pid != MKR_USB_PID ||
        g_mkr_manifest.ids_placeholder != (uint8_t)MKR_USB_IDS_ARE_PLACEHOLDER ||
        g_mkr_manifest.iwdg_lsi_counts != MKR_IWDG_LSI_COUNTS ||
        g_mkr_manifest.rtc_kick_lsi_counts != MKR_RTC_KICK_LSI_COUNTS) {
        halt();
    }
    mkr_bringup(&ops);
    /* bring-up does not return on clock or USB failure. Arm after that. */
#if MKR_RUN_IWDG
    DIAG_STAGE(9u);
    if (!board_iwdg_start()) {
        latch_fault();
        halt();
    }
#endif
    DIAG_STAGE(8u);
    for (;;) {
        board_iwdg_kick();
#if MKR_BRINGUP_AWAKE
        /* Bench modes stay awake; only USB-only mode forces outputs off. */
        __asm volatile("nop");
#else
        if (mkr_usb_dcd_stop_requested()) {
            board_stop_enter();
            /* Resume/reset clears the request from the USB IRQ, which runs
             * before this line. Restore is a no-op when the wakeup IRQ already
             * brought the PLL back. ISO power stays with the power sequence. */
            if (!mkr_usb_dcd_stop_requested() && !board_clock_restore_after_stop()) {
                latch_fault();
                halt();
            }
        } else {
            __asm volatile("wfi");
        }
#endif
    }
}
