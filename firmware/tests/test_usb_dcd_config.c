#include "board_clock.h"
#include "bringup_mode.h"
#include "board_nvic.h"
#include "power_sequence.h"
#include "stm32f411xe.h"
#include "time_us.h"
#include "usb_dcd.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * Host model of the OTG FS device interrupt path. Passing this test is not
 * USB enumeration on a chip and does not approve flashing or connecting DB25.
 *
 * GINTSTS / DIEPINT / DOEPINT are write-1-to-clear, matching the silicon.
 */
enum {
    H_XFRC = 1u << 0,
    H_TOC = 1u << 3,
    H_RXFLVL = 1u << 4,
    H_USBSUSP = 1u << 11,
    H_TXFE = 1u << 7,
    H_USBRST = 1u << 12,
    H_IEPINT = 1u << 18,
    H_OEPINT = 1u << 19,
    H_USBAEP = 1u << 15,
    H_EPENA = 1u << 31
};
uint8_t mkr_host_usb_mem[0x5000];
GPIO_TypeDef mkr_host_gpioa;
RCC_TypeDef mkr_host_rcc;
EXTI_TypeDef mkr_host_exti;
volatile uint8_t mkr_host_nvic_ipr[128];
volatile uint32_t mkr_host_nvic_iser[8];
static uint32_t g_gintsts;
static uint32_t g_diepint[4];
static uint32_t g_doepint[4];
static uint32_t g_rxq[4];
static unsigned g_rx_n;
static unsigned g_rx_i;
static uint32_t g_fifo[4];
static unsigned g_fifo_n;
static unsigned g_fifo_i;
static uint32_t g_delay_ms;
static uint32_t g_delay_cycles_max;
typedef struct {
    bool power;
    uint8_t code;
    bool ready;
} board_t;
static board_t g_board;
static bool g_check_flush_safe;
static bool g_fail_flush;
static uint32_t g_flush_checks;
static bool allow_register_wait(volatile uint32_t *reg)
{
    if (g_check_flush_safe && reg == &USB_OTG_FS->GRSTCTL) {
        ++g_flush_checks;
        /* If this assertion fails, a blocking FIFO wait happened while
         * Marker/READY or PA15 was still on. */
        assert(!g_board.power && !g_board.ready && g_board.code == 0u);
        return !g_fail_flush;
    }
    return true;
}
uint32_t mkr_host_gintsts_read(void) { return g_gintsts; }
void mkr_host_gintsts_write(uint32_t value) { g_gintsts &= ~value; }
uint32_t mkr_host_diepint_read(uint8_t ep) { return ep < 4u ? g_diepint[ep] : 0u; }
void mkr_host_diepint_write(uint8_t ep, uint32_t value)
{
    if (ep < 4u) {
        g_diepint[ep] &= ~value;
    }
}
uint32_t mkr_host_doepint_read(uint8_t ep) { return ep < 4u ? g_doepint[ep] : 0u; }
void mkr_host_doepint_write(uint8_t ep, uint32_t value)
{
    if (ep < 4u) {
        g_doepint[ep] &= ~value;
    }
}
uint32_t mkr_host_rxstsp(void)
{
#if MKR_BRINGUP_AWAKE
    if ((USB_OTG_PCGCCTL & 1u) != 0u) {
        fputs("FAIL: diagnostic RX access while PHY clock stopped\n", stderr);
        exit(1);
    }
#endif
    uint32_t value = 0u;
    if (g_rx_i < g_rx_n) {
        value = g_rxq[g_rx_i++];
    }
    if (g_rx_i >= g_rx_n) {
        g_gintsts &= ~H_RXFLVL;
    }
    return value;
}
uint32_t mkr_host_dfifo_read(uint32_t ep)
{
    (void)ep;
    if (g_fifo_i < g_fifo_n) {
        return g_fifo[g_fifo_i++];
    }
    return 0u;
}
static unsigned g_tx_fifo_writes;
static uint32_t g_first_tx_word;
void mkr_host_dfifo_write(uint32_t ep, uint32_t value)
{
    if ((USB_INEP(ep)->DIEPCTL & H_EPENA) == 0u) {
        fputs("FAIL: TX FIFO written before endpoint was enabled\n", stderr);
        exit(1);
    }
    if (g_tx_fifo_writes == 0u) { g_first_tx_word = value; }
    ++g_tx_fifo_writes;
}
void board_delay_ms(uint32_t ms) { g_delay_ms += ms; }
void board_delay_cycles(uint32_t cycles)
{
    if (cycles > g_delay_cycles_max) {
        g_delay_cycles_max = cycles;
    }
}
bool board_wait_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles)
{
    (void)timeout_cycles;
    if (!allow_register_wait(reg)) {
        return false;
    }
    *reg |= mask;
    return true;
}
bool board_wait_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles)
{
    (void)timeout_cycles;
    if (!allow_register_wait(reg)) {
        return false;
    }
    *reg &= ~mask;
    return true;
}
static mkr_power_sequence_t g_pwr;
static int g_events[24];
static int g_nevents;
static uint32_t g_rx_calls;
static void on_rx(const uint8_t *data, uint16_t len, void *ctx)
{
    (void)ctx;
    assert(len > 0u);
    ++g_rx_calls;
    /* Simulate a stale packet that would otherwise output a new Marker. */
    g_board.code = data[0];
}
static void on_marker(uint8_t code, bool ready, void *ctx)
{
    board_t *b = ctx;
    b->code = ready ? code : 0u;
    b->ready = ready;
}
static void on_power(bool enabled, void *ctx) { ((board_t *)ctx)->power = enabled; }
static void on_usb(mkr_power_event_t event, void *ctx)
{
    (void)ctx;
    if (g_nevents < (int)(sizeof g_events / sizeof g_events[0])) {
        g_events[g_nevents++] = (int)event;
    }
    mkr_power_sequence_handle_event(&g_pwr, event);
}
static int count_event(mkr_power_event_t event)
{
    int n = 0;
    for (int i = 0; i < g_nevents; i++) {
        if (g_events[i] == (int)event) {
            n++;
        }
    }
    return n;
}
static void setup_bytes(uint8_t t, uint8_t r, uint16_t value, uint16_t index, uint16_t length, uint8_t out[8])
{
    memset(out, 0, 8u);
    out[0] = t;
    out[1] = r;
    out[2] = (uint8_t)value;
    out[3] = (uint8_t)(value >> 8);
    out[4] = (uint8_t)index;
    out[5] = (uint8_t)(index >> 8);
    out[6] = (uint8_t)length;
    out[7] = (uint8_t)(length >> 8);
}
static void load_setup(const uint8_t s[8])
{
    g_fifo[0] = (uint32_t)s[0] | ((uint32_t)s[1] << 8) | ((uint32_t)s[2] << 16) | ((uint32_t)s[3] << 24);
    g_fifo[1] = (uint32_t)s[4] | ((uint32_t)s[5] << 8) | ((uint32_t)s[6] << 16) | ((uint32_t)s[7] << 24);
    g_fifo_n = 2u;
    g_fifo_i = 0u;
    g_rxq[0] = (6u << 17) | (8u << 4);
    g_rxq[1] = (4u << 17);
    g_rx_n = 2u;
    g_rx_i = 0u;
    /* Stale EP0 XFRC must not commit the new SETUP in this same IRQ. */
    g_diepint[0] |= H_XFRC;
    USB_OTG_DEVICE->DAINT |= 1u;
    g_gintsts = H_RXFLVL | H_IEPINT;
}
static void irq(void) { OTG_FS_IRQHandler(); }
static void send_setup(uint8_t t, uint8_t r, uint16_t value)
{
    uint8_t s[8];
    setup_bytes(t, r, value, 0u, 0u, s);
    load_setup(s);
    irq();
}
static void complete_status_in(void)
{
    USB_OTG_DEVICE->DAINT |= 1u;
    g_diepint[0] |= H_XFRC;
    g_gintsts = H_IEPINT;
    irq();
}
static void bus_reset(void)
{
    g_gintsts = H_USBRST;
    irq();
}
static void tick_power_for(uint32_t us)
{
    mkr_time_soft_advance_us(us);
    mkr_power_sequence_tick_1ms(&g_pwr);
}
static void expect_power_at_deadline(void)
{
    const uint32_t deadline = g_pwr.deadline_us;
    assert(g_pwr.state == MKR_POWER_STATE_WAIT_CONFIG_DELAY);
    mkr_time_soft_set_us(deadline - 1u);
    mkr_power_sequence_tick_1ms(&g_pwr);
    assert(!g_board.power);
    mkr_time_soft_set_us(deadline);
    mkr_power_sequence_tick_1ms(&g_pwr);
    assert(g_board.power);
}
int main(void)
{
    const mkr_power_io_t pio = {on_marker, on_power, &g_board};
    const mkr_usb_dcd_cbs_t cbs = {on_usb, on_rx, 0};
    const uint8_t pkt[4] = {1u, 2u, 3u, 4u};
    memset(mkr_host_usb_mem, 0, sizeof mkr_host_usb_mem);
    mkr_power_sequence_init(&g_pwr, &pio);
    assert(mkr_usb_dcd_start(&cbs));
    assert(g_delay_ms == 25u);
    assert(g_delay_cycles_max == 64u);
    assert(!mkr_usb_dcd_test_configured());
    assert(!g_board.power);
    /* Diagnostic mode must survive suspend without stopping its USB clock.
     * Production retains its existing low-power behavior in this change. */
    USB_OTG_DEVICE->DSTS = 1u;
    g_gintsts = H_USBSUSP;
    irq();
#if MKR_BRINGUP_AWAKE
    assert((USB_OTG_PCGCCTL & 1u) == 0u);
    assert(!mkr_usb_dcd_stop_requested());
#else
    assert((USB_OTG_PCGCCTL & 1u) != 0u);
    assert(mkr_usb_dcd_stop_requested());
#endif
    USB_OTG_PCGCCTL = 0u;
    USB_OTG_DEVICE->DSTS = 0u;
    bus_reset();
    /* Real enumeration starts with GET_DESCRIPTOR, not SET_CONFIGURATION.
     * The hardware endpoint must be armed before the FIFO is filled. */
    {
        uint8_t setup[8];
        USB_INEP(0)->DTXFSTS = 64u;
        setup_bytes(0x80u, 0x06u, 0x0100u, 0u, 64u, setup);
        load_setup(setup);
#if MKR_BRINGUP_AWAKE
        USB_OTG_PCGCCTL = 1u; /* IRQ must ungate before any RX access. */
#endif
        irq();
        assert(g_tx_fifo_writes == 5u);
        assert(g_first_tx_word == 0x02000112u);
        assert((USB_INEP(0)->DIEPTSIZ & 0x7ffu) == 18u);
        bus_reset();
    }
    /* DWC2 DAD must be programmed before arming the SET_ADDRESS status ZLP.
     * Cover values crossing each byte/bit boundary and bus-reset clearing. */
    {
        const uint8_t addresses[] = {0u, 1u, 7u, 8u, 33u, 39u, 64u, 127u};
        for (unsigned i = 0u; i < sizeof addresses; ++i) {
            send_setup(0x00u, 0x05u, addresses[i]);
            if (((USB_OTG_DEVICE->DCFG >> 4) & 0x7fu) != addresses[i]) {
                fputs("FAIL: DWC2 address not set before status ZLP\n", stderr);
                exit(1);
            }
            assert(mkr_usb_dcd_test_in_busy(0u));
            assert(USB_INEP(0)->DIEPTSIZ == (1u << 19));
            complete_status_in();
            assert(((USB_OTG_DEVICE->DCFG >> 4) & 0x7fu) == addresses[i]);
            bus_reset();
            assert((USB_OTG_DEVICE->DCFG & (0x7fu << 4)) == 0u);
        }
    }
    /* SET_CONFIGURATION(1) does not open data EPs or start the PA15 countdown. */
    send_setup(0x00u, 0x09u, 1u);
    assert(!mkr_usb_dcd_test_configured());
    assert(mkr_usb_dcd_test_cfg_commit() == 1u);
    assert((USB_INEP(1)->DIEPCTL & H_USBAEP) == 0u);
    assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == 0);
    tick_power_for(50000u);
    assert(!g_board.power);
    assert(g_pwr.state == MKR_POWER_STATE_UNCONFIGURED);
    /* Status IN success is the commit point. 9.999 ms is still off. */
    complete_status_in();
    assert(mkr_usb_dcd_test_configured());
    assert(mkr_usb_dcd_test_cfg_commit() == 0u);
    assert((USB_INEP(1)->DIEPCTL & H_USBAEP) != 0u);
    assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == 1);
    assert(g_pwr.deadline_us == mkr_hw_time_us() + 10000u);
    expect_power_at_deadline();
    /* Reconfigure restarts the floor. Failed status must not. */
    {
        const int before = count_event(MKR_POWER_EVENT_USB_CONFIGURED);
        send_setup(0x00u, 0x09u, 1u);
        assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == before);
        assert(g_board.power);
        send_setup(0x00u, 0x05u, 3u); /* SET_ADDRESS aborts the reconfigure */
        complete_status_in();
        assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == before);
        assert(mkr_usb_dcd_test_configured());
        assert(g_board.power);
        send_setup(0x00u, 0x09u, 1u);
        complete_status_in();
        assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == before + 1);
        assert(!g_board.power);
        expect_power_at_deadline();
    }
    /* In-flight TX, then reset, must not keep TXFE or a busy queue. */
    USB_INEP(1)->DTXFSTS = 0u;
    assert(mkr_usb_dcd_write(pkt, 4u) == 0);
    assert(mkr_usb_dcd_test_inflight_wait(1u));
    assert(mkr_usb_dcd_test_in_busy(1u));
    assert((USB_OTG_DEVICE->DIEPEMPMSK & (1u << 1)) != 0u);
    assert((USB_INEP(1)->DIEPCTL & H_EPENA) != 0u);
    bus_reset();
    assert(!mkr_usb_dcd_test_configured());
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);
    assert(!mkr_usb_dcd_test_inflight_wait(0u));
    assert(!mkr_usb_dcd_test_inflight_wait(1u));
    assert(!mkr_usb_dcd_test_inflight_wait(2u));
    assert(!mkr_usb_dcd_test_in_busy(1u));
    assert(mkr_usb_dcd_test_tx_count() == 0u);
    assert(!mkr_usb_dcd_test_bulk_zlp());
    assert(!g_board.power);
    assert(mkr_usb_dcd_write(pkt, 4u) == -1);
    USB_OTG_DEVICE->DAINT = 1u << 1;
    g_diepint[1] = H_TXFE;
    g_gintsts = H_IEPINT;
    irq();
    assert((USB_INEP(1)->DIEPCTL & H_EPENA) == 0u);
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);
    /* Stale status-IN completion after reset must not configure. */
    g_diepint[0] = H_XFRC;
    USB_OTG_DEVICE->DAINT = 1u;
    g_gintsts = H_IEPINT;
    irq();
    assert(!mkr_usb_dcd_test_configured());
    tick_power_for(20000u);
    assert(!g_board.power);
    /* Re-enumerate and a new IN must leave no TXFE wait and must not stick busy. */
    send_setup(0x00u, 0x09u, 1u);
    assert(!mkr_usb_dcd_test_configured());
    complete_status_in();
    assert(mkr_usb_dcd_test_configured());
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);
    assert(!mkr_usb_dcd_test_inflight_wait(1u));
    USB_OTG_DEVICE->DAINT = 1u << 1;
    g_diepint[1] = H_TXFE;
    g_gintsts = H_IEPINT;
    irq();
    assert(!mkr_usb_dcd_test_inflight_wait(1u));
    assert((USB_OTG_DEVICE->DIEPEMPMSK & (1u << 1)) == 0u);
    assert((USB_INEP(1)->DIEPCTL & H_EPENA) == 0u);
    USB_INEP(1)->DTXFSTS = 64u;
    assert(mkr_usb_dcd_write(pkt, 4u) == 0);
    assert((USB_INEP(1)->DIEPTSIZ & 0x7FFu) == 4u);
    assert((USB_INEP(1)->DIEPCTL & H_EPENA) != 0u);
    assert(!mkr_usb_dcd_test_inflight_wait(1u));
    assert((USB_OTG_DEVICE->DIEPEMPMSK & (1u << 1)) == 0u);
    USB_OTG_DEVICE->DAINT = 1u << 1;
    g_diepint[1] = H_XFRC;
    g_gintsts = H_IEPINT;
    irq();
    assert(!mkr_usb_dcd_test_in_busy(1u));
    {
        const uint8_t next[2] = {9u, 8u};
        USB_INEP(1)->DTXFSTS = 64u;
        assert(mkr_usb_dcd_write(next, 2u) == 0);
        assert((USB_INEP(1)->DIEPTSIZ & 0x7FFu) == 2u);
        assert((USB_INEP(1)->DIEPCTL & H_EPENA) != 0u);
    }
    bus_reset();
    bus_reset();
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);
    assert(!mkr_usb_dcd_test_configured());
    /* Unconfigure applies on status success. An aborted unconfigure keeps the old config. */
    send_setup(0x00u, 0x09u, 1u);
    complete_status_in();
    expect_power_at_deadline();
    USB_INEP(1)->DTXFSTS = 0u;
    assert(mkr_usb_dcd_write(pkt, 4u) == 0);
    assert((USB_OTG_DEVICE->DIEPEMPMSK & (1u << 1)) != 0u);
    {
        const int uncfg = count_event(MKR_POWER_EVENT_USB_UNCONFIGURED);
        send_setup(0x00u, 0x09u, 0u);
        assert(mkr_usb_dcd_test_configured());
        assert(mkr_usb_dcd_test_cfg_commit() == 2u);
        assert(count_event(MKR_POWER_EVENT_USB_UNCONFIGURED) == uncfg);
        assert(g_board.power);
        send_setup(0x00u, 0x05u, 4u);
        complete_status_in();
        assert(mkr_usb_dcd_test_configured());
        assert(count_event(MKR_POWER_EVENT_USB_UNCONFIGURED) == uncfg);
        assert(g_board.power);
        send_setup(0x00u, 0x09u, 0u);
        complete_status_in();
        assert(!mkr_usb_dcd_test_configured());
        assert(count_event(MKR_POWER_EVENT_USB_UNCONFIGURED) == uncfg + 1);
        assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);
        assert(!mkr_usb_dcd_test_inflight_wait(1u));
        assert(mkr_usb_dcd_test_tx_count() == 0u);
        assert(!g_board.power);
        assert(mkr_usb_dcd_write(pkt, 1u) == -1);
        tick_power_for(30000u);
        assert(!g_board.power);
    }
    /* EP0 timeout cancels a pending configuration. */
    send_setup(0x00u, 0x09u, 1u);
    assert(mkr_usb_dcd_test_cfg_commit() == 1u);
    USB_OTG_DEVICE->DAINT |= 1u;
    g_diepint[0] = H_TOC;
    g_gintsts = H_IEPINT;
    irq();
    assert(mkr_usb_dcd_test_cfg_commit() == 0u);
    assert(!mkr_usb_dcd_test_configured());
    g_diepint[0] = H_XFRC;
    g_gintsts = H_IEPINT;
    irq();
    assert(!mkr_usb_dcd_test_configured());
    tick_power_for(30000u);
    assert(!g_board.power);
    /* USBRST concurrent with RXFLVL + OUT XFRC must take priority.
     * Simulate an active high Marker; FIFO waits fail on purpose. The
     * expected behavior is safe-off before even the first flush attempt,
     * and no delivery of the stale OUT marker. */
    send_setup(0x00u, 0x09u, 1u);
    complete_status_in();
    expect_power_at_deadline();
    tick_power_for(MKR_POWER_ISO_SETTLE_MS * 1000u);
    assert(g_board.ready && g_board.power);
    g_board.code = 0x2Au;
    g_rx_calls = 0u;
    g_fifo[0] = 0x39u;
    g_fifo_n = 1u;
    g_fifo_i = 0u;
    g_rxq[0] = (2u << 17) | (1u << 4) | 1u; /* EP1 OUT, one byte */
    g_rx_n = 1u;
    g_rx_i = 0u;
    USB_OTG_DEVICE->DAINT |= 1u << 17;
    g_doepint[1] = H_XFRC;
    g_gintsts = H_USBRST | H_RXFLVL | H_OEPINT;
    const int resets_before = count_event(MKR_POWER_EVENT_USB_BUS_RESET);
    g_flush_checks = 0u;
    g_check_flush_safe = true;
    g_fail_flush = true;
    irq();
    g_check_flush_safe = false;
    g_fail_flush = false;
    assert(g_flush_checks >= 2u); /* both TX and RX flush attempts */
    assert(count_event(MKR_POWER_EVENT_USB_BUS_RESET) == resets_before + 1);
    assert(g_rx_calls == 0u);
    assert(!g_board.power && !g_board.ready && g_board.code == 0u);
    assert(!mkr_usb_dcd_test_configured());
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);

    /* Cancel configuration while a Marker is high and TX is stalled.
     * Status IN commits the cancellation; output must fall before
     * close_data_eps() attempts a potentially blocking FIFO flush. */
    send_setup(0x00u, 0x09u, 1u);
    complete_status_in();
    expect_power_at_deadline();
    tick_power_for(MKR_POWER_ISO_SETTLE_MS * 1000u);
    assert(g_board.ready && g_board.power);
    g_board.code = 0x47u;
    USB_INEP(1)->DTXFSTS = 0u;
    assert(mkr_usb_dcd_write(pkt, 4u) == 0);
    assert(mkr_usb_dcd_test_inflight_wait(1u));
    send_setup(0x00u, 0x09u, 0u);
    assert(g_board.ready && g_board.power);
    const int unconfigs_before = count_event(MKR_POWER_EVENT_USB_UNCONFIGURED);
    g_flush_checks = 0u;
    g_check_flush_safe = true;
    g_fail_flush = true;
    complete_status_in();
    g_check_flush_safe = false;
    g_fail_flush = false;
    assert(g_flush_checks >= 1u);
    assert(count_event(MKR_POWER_EVENT_USB_UNCONFIGURED) == unconfigs_before + 1);
    assert(!g_board.power && !g_board.ready && g_board.code == 0u);
    assert(!mkr_usb_dcd_test_configured());
    assert(USB_OTG_DEVICE->DIEPEMPMSK == 0u);

    /* An EP0 status IN interrupt simultaneous with USBRST must not
     * commit pending SET_CONFIGURATION or start the PA15 countdown. */
    send_setup(0x00u, 0x09u, 1u);
    assert(mkr_usb_dcd_test_cfg_commit() == 1u);
    const int configs_before = count_event(MKR_POWER_EVENT_USB_CONFIGURED);
    USB_OTG_DEVICE->DAINT |= 1u;
    g_diepint[0] = H_XFRC;
    g_gintsts = H_USBRST | H_IEPINT;
    irq();
    assert(count_event(MKR_POWER_EVENT_USB_CONFIGURED) == configs_before);
    assert(!mkr_usb_dcd_test_configured());
    assert(mkr_usb_dcd_test_cfg_commit() == 0u);
    assert(!g_board.power && !g_board.ready);
    return 0;
}
