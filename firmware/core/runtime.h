#ifndef MKR_RUNTIME_H
#define MKR_RUNTIME_H
/*
 * IRQ-facing glue. USB and SysTick call the on_* functions; each one holds
 * lock/unlock around power_sequence, the parser and the marker FSM.
 * NMI clock-fault latch does not take the lock: NMI ignores PRIMASK, and the
 * caller must not return into the interrupted context.
 *
 * Device startup pulse is 2 ms (Mode B), timed from a microsecond deadline
 * (TIM2 on the target). The portable FSM init default stays 10 ms; this
 * runtime overrides it after init. Gap stays 10 ms until SetGap.
 * The 1 ms tick can only end a pulse at or after that deadline. It cannot
 * shorten a 2 ms pulse to ~1 ms. Level-hold does not arm the deadline.
 * Pulse width has not been measured on a scope.
 */
#include <stddef.h>
#include <stdint.h>
#include "power_sequence.h"
#include "protocol_parser.h"
#include "marker_fsm.h"
#define MKR_DEVICE_DEFAULT_PULSE_MS 2u
typedef struct {
    void (*write_marker)(uint8_t code, bool ready, void *ctx);
    void (*set_iso_power)(bool enabled, void *ctx);
    void (*tx)(const uint8_t *data, uint16_t len, void *ctx);
    void (*lock)(void *ctx);
    void (*unlock)(void *ctx);
    void *ctx;
} mkr_runtime_io_t;
typedef struct {
    mkr_power_sequence_t power;
    mkr_fsm_t fsm;
    mkr_parser_t parser;
    bool outputs_live;
    uint8_t code;
    void (*hw_write)(uint8_t code, bool ready, void *ctx);
    void (*set_iso)(bool enabled, void *ctx);
    void (*tx)(const uint8_t *data, uint16_t len, void *ctx);
    void (*lock)(void *ctx);
    void (*unlock)(void *ctx);
    void *ctx;
} mkr_runtime_t;
void mkr_runtime_init(mkr_runtime_t *rt, const mkr_runtime_io_t *io);
void mkr_runtime_on_usb_event(mkr_runtime_t *rt, mkr_power_event_t event);
void mkr_runtime_on_tick_1ms(mkr_runtime_t *rt);
/* TIM2 compare, or a host test, at the pulse/gap deadline. */
void mkr_runtime_on_marker_deadline(mkr_runtime_t *rt);
void mkr_runtime_on_rx(mkr_runtime_t *rt, const uint8_t *data, size_t len);
void mkr_runtime_latch_clock_fault(mkr_runtime_t *rt);
#endif
