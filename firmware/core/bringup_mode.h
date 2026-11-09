#ifndef MKR_BRINGUP_MODE_H
#define MKR_BRINGUP_MODE_H
/* Bench modes are opt-in and mutually exclusive. USB-only forces all outputs
 * off; functional mode uses the real power/marker callbacks. RTC/IWDG and
 * Stop are restored by separate opt-in validation switches. */
#if defined(MKR_USB_ONLY_BRINGUP) && MKR_USB_ONLY_BRINGUP && \
    defined(MKR_FUNCTIONAL_BRINGUP) && MKR_FUNCTIONAL_BRINGUP
#error "Select only one bench bring-up mode"
#endif
#if (defined(MKR_USB_ONLY_BRINGUP) && MKR_USB_ONLY_BRINGUP) || \
    (defined(MKR_FUNCTIONAL_BRINGUP) && MKR_FUNCTIONAL_BRINGUP)
#define MKR_BRINGUP_DIAGNOSTICS 1
#else
#define MKR_BRINGUP_DIAGNOSTICS 0
#endif
#ifndef MKR_BENCH_IWDG
#define MKR_BENCH_IWDG 0
#endif
#ifndef MKR_BENCH_STOP
#define MKR_BENCH_STOP 0
#endif
#if MKR_BENCH_STOP && !MKR_BENCH_IWDG
#error "Bench Stop validation requires bench RTC/IWDG"
#endif
#define MKR_BRINGUP_AWAKE (MKR_BRINGUP_DIAGNOSTICS && !MKR_BENCH_STOP)
#if MKR_BENCH_IWDG && (!defined(MKR_FUNCTIONAL_BRINGUP) || !MKR_FUNCTIONAL_BRINGUP)
#error "Bench watchdog validation requires functional bring-up"
#endif
#define MKR_RUN_IWDG (!MKR_BRINGUP_AWAKE || MKR_BENCH_IWDG)
#endif
