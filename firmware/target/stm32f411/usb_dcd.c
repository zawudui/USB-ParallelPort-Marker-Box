#include "usb_dcd.h"
#include "bringup_mode.h"
#include "board_clock.h"
#include "board_nvic.h"
#include "clock_plan.h"
#include "stm32f411xe.h"
#include "usb_setup.h"
/*
 * FIFO plan, 32-bit words, FS core total 320 words (1280 bytes):
 *   RX            128 at 0
 *   EP0 TX         64 at 128   (DIEPTXF0)
 *   EP1 bulk IN    64 at 192   (DIEPTXF[0] is FIFO 1, register 0x104)
 *   EP2 interrupt  16 at 256   (DIEPTXF[1])
 * Sum 272. EP2 is armed NAK-only; CDC serial-state notifications are not sent.
 *
 * Host tests compile this file with MKR_USB_HOST_TEST. Interrupt status
 * registers are write-1-to-clear on the silicon, so the test double uses
 * the same rule. Production reads the registers directly.
 */
#ifdef MKR_USB_HOST_TEST
uint32_t mkr_host_gintsts_read(void);
void mkr_host_gintsts_write(uint32_t value);
uint32_t mkr_host_diepint_read(uint8_t ep);
void mkr_host_diepint_write(uint8_t ep, uint32_t value);
uint32_t mkr_host_doepint_read(uint8_t ep);
void mkr_host_doepint_write(uint8_t ep, uint32_t value);
uint32_t mkr_host_rxstsp(void);
uint32_t mkr_host_dfifo_read(uint32_t ep);
void mkr_host_dfifo_write(uint32_t ep, uint32_t value);
#define MKR_GINTSTS_READ() mkr_host_gintsts_read()
#define MKR_GINTSTS_WRITE(v) mkr_host_gintsts_write(v)
#define MKR_DIEPINT_READ(ep) mkr_host_diepint_read(ep)
#define MKR_DIEPINT_WRITE(ep, v) mkr_host_diepint_write((ep), (v))
#define MKR_DOEPINT_READ(ep) mkr_host_doepint_read(ep)
#define MKR_DOEPINT_WRITE(ep, v) mkr_host_doepint_write((ep), (v))
#define MKR_USB_RXSTSP() mkr_host_rxstsp()
#define MKR_USB_DFIFO_READ(ep) mkr_host_dfifo_read(ep)
#define MKR_USB_DFIFO_WRITE(ep, v) mkr_host_dfifo_write((ep), (v))
#else
#define MKR_GINTSTS_READ() (USB_OTG_FS->GINTSTS)
#define MKR_GINTSTS_WRITE(v) do { USB_OTG_FS->GINTSTS = (v); } while (0)
#define MKR_DIEPINT_READ(ep) (USB_INEP(ep)->DIEPINT)
#define MKR_DIEPINT_WRITE(ep, v) do { USB_INEP(ep)->DIEPINT = (v); } while (0)
#define MKR_DOEPINT_READ(ep) (USB_OUTEP(ep)->DOEPINT)
#define MKR_DOEPINT_WRITE(ep, v) do { USB_OUTEP(ep)->DOEPINT = (v); } while (0)
#define MKR_USB_RXSTSP() (USB_OTG_FS->GRXSTSP)
#define MKR_USB_DFIFO_READ(ep) (USB_DFIFO(ep))
#define MKR_USB_DFIFO_WRITE(ep, v) do { USB_DFIFO(ep) = (v); } while (0)
#endif
#define RX_FIFO_WORDS 128u
#define TX0_FIFO_WORDS 64u
#define TX1_FIFO_WORDS 64u
#define TX2_FIFO_WORDS 16u
_Static_assert(RX_FIFO_WORDS + TX0_FIFO_WORDS + TX1_FIFO_WORDS + TX2_FIFO_WORDS <= 320u, "FIFO");
#define EPENA (1u << 31)
#define EPDIS (1u << 30)
#define SD0PID (1u << 28)
#define SNAK (1u << 27)
#define CNAK (1u << 26)
#define STALL_BIT (1u << 21)
#define USBAEP (1u << 15)
#define GINT_BIT (1u << 0)
#define PHYSEL (1u << 6)
#define FDMOD (1u << 30)
#define FHMOD (1u << 29)
#define CSRST (1u << 0)
#define AHBIDL (1u << 31)
#define RXFFLSH (1u << 4)
#define TXFFLSH (1u << 5)
#define RXFLVL (1u << 4)
#define USBSUSP (1u << 11)
#define USBRST (1u << 12)
#define ENUMDNE (1u << 13)
#define IEPINT (1u << 18)
#define OEPINT (1u << 19)
#define WKUINT (1u << 31)
#define XFRC (1u << 0)
#define TOC (1u << 3)
#define STUP (1u << 3)
#define TXFE (1u << 7)
#define SDIS (1u << 1)
#define GCCFG_PWRDWN (1u << 16)
#define GCCFG_VBUSASEN (1u << 18)
#define GCCFG_VBUSBSEN (1u << 19)
#define GCCFG_NOVBUSSENS (1u << 21)
/* OTG_FS_PCGCCTL bit 0. ST's low-power macro gates the PHY with this bit only.
 * GATEHCLK would block the AHB window used to see resume. PHYSUSP is status. */
#define STOPCLK (1u << 0)
/* OTG_FS_DSTS bit 0. USBSUSP can be set when the device is not suspended. */
#define DSTS_SUSPSTS (1u << 0)
#define PKTSTS_OUT 2u
#define PKTSTS_SETUP_DONE 4u
#define PKTSTS_SETUP 6u
#define TXQ_SIZE 256u
#define USB_REG_TIMEOUT (MKR_SYSCLK_HZ / 10u)
enum { EP0_SETUP = 0, EP0_DATA_IN, EP0_DATA_OUT, EP0_STATUS_IN, EP0_STATUS_OUT };
enum { CFG_COMMIT_NONE = 0, CFG_COMMIT_SET = 1, CFG_COMMIT_CLEAR = 2 };
static mkr_usb_event_fn g_on_event;
static mkr_usb_rx_fn g_on_rx;
static void *g_ctx;
static bool g_usb_clock_on;
static volatile uint8_t g_stop_requested;
static bool g_configured;
static uint8_t g_cfg_commit;
static uint8_t g_ctrl_gen;
static uint8_t g_status_gen;
static uint8_t g_line[7] = {0x00u, 0xC2u, 0x01u, 0x00u, 0x00u, 0x00u, 0x08u}; /* 115200 8N1 */
static uint8_t g_setup[8];
static bool g_setup_valid;
static uint8_t g_out[64];
static uint16_t g_out_len;
static uint8_t g_out_ep;
static uint8_t g_ep0_state;
static const uint8_t *g_ep0_src;
static uint16_t g_ep0_total;
static uint16_t g_ep0_sent;
static uint16_t g_ep0_wlength;
static bool g_ep0_zlp;
static uint8_t g_addr;
static uint8_t g_txq[TXQ_SIZE];
static uint16_t g_tx_head;
static uint16_t g_tx_tail;
static uint16_t g_tx_count;
static bool g_in_busy[3];
static uint8_t g_inflight[3][64];
static uint16_t g_inflight_len[3];
static bool g_inflight_wait[3];
static bool g_bulk_zlp;
#if MKR_BRINGUP_DIAGNOSTICS
/* Diagnostic RAM trace: no FIFO/status-pop reads. Entries are a circular log;
 * count is published after the four words have been written. */
volatile struct {
    uint32_t count;
    struct { uint32_t event, state, a, b; } entry[128];
} g_usb_trace;
static void trace(uint32_t event, uint32_t a, uint32_t b)
{
    const uint32_t n = g_usb_trace.count;
    const uint32_t i = n & 127u;
    g_usb_trace.entry[i].event = event;
    g_usb_trace.entry[i].state = g_ep0_state;
    g_usb_trace.entry[i].a = a;
    g_usb_trace.entry[i].b = b;
    g_usb_trace.count = n + 1u;
}
#else
#define trace(event, a, b) ((void)0)
#endif
static void emit(mkr_power_event_t event)
{
    if (g_on_event != 0) {
        g_on_event(event, g_ctx);
    }
}
static void fifo_read(uint8_t *dst, uint16_t bcnt, uint16_t cap)
{
    const uint16_t words = (uint16_t)((bcnt + 3u) / 4u);
    uint16_t filled = 0u;
    for (uint16_t w = 0u; w < words; w++) {
        uint32_t value = MKR_USB_DFIFO_READ(0u);
        for (uint8_t b = 0u; b < 4u; b++) {
            if (dst != 0 && filled < bcnt && filled < cap) {
                dst[filled] = (uint8_t)(value & 0xFFu);
            }
            value >>= 8;
            if (filled < bcnt) {
                filled++;
            }
        }
    }
}
static void fifo_write(uint8_t ep, const uint8_t *src, uint16_t len)
{
    const uint16_t words = (uint16_t)((len + 3u) / 4u);
    for (uint16_t w = 0u; w < words; w++) {
        uint32_t value = 0u;
        for (uint8_t b = 0u; b < 4u; b++) {
            const uint16_t index = (uint16_t)(w * 4u + b);
            if (index < len) {
                value |= (uint32_t)src[index] << (8u * b);
            }
        }
        MKR_USB_DFIFO_WRITE(ep, value);
    }
}
static bool flush_tx_all(void)
{
    if (!board_wait_set(&USB_OTG_FS->GRSTCTL, AHBIDL, USB_REG_TIMEOUT)) {
        return false;
    }
    USB_OTG_FS->GRSTCTL = TXFFLSH | (0x10u << 6);
    return board_wait_clear(&USB_OTG_FS->GRSTCTL, TXFFLSH, USB_REG_TIMEOUT);
}
static bool flush_rx(void)
{
    if (!board_wait_set(&USB_OTG_FS->GRSTCTL, AHBIDL, USB_REG_TIMEOUT)) {
        return false;
    }
    USB_OTG_FS->GRSTCTL = RXFFLSH;
    return board_wait_clear(&USB_OTG_FS->GRSTCTL, RXFFLSH, USB_REG_TIMEOUT);
}
/* Drop software TX state and the empty-FIFO mask. Safe to call twice.
 * Does not change g_configured or the control-transfer generation. */
static void clear_async_tx(void)
{
    g_tx_head = 0u;
    g_tx_tail = 0u;
    g_tx_count = 0u;
    g_bulk_zlp = false;
    g_ep0_zlp = false;
    g_ep0_src = 0;
    g_ep0_total = 0u;
    g_ep0_sent = 0u;
    for (uint8_t ep = 0u; ep < 3u; ep++) {
        g_in_busy[ep] = false;
        g_inflight_wait[ep] = false;
        g_inflight_len[ep] = 0u;
    }
    if (g_usb_clock_on) {
        USB_OTG_DEVICE->DIEPEMPMSK = 0u;
    }
}
static void start_in(uint8_t ep, const uint8_t *data, uint16_t len)
{
    USB_OTG_INEndpointTypeDef *in = USB_INEP(ep);
    /* A stale XFRC/TXFE must not complete the transfer we are arming. */
    MKR_DIEPINT_WRITE(ep, XFRC | TXFE);
    if (len > 64u) {
        len = 64u;
    }
    if (len > 0u && data != 0 && data != g_inflight[ep]) {
        for (uint16_t i = 0u; i < len; i++) {
            g_inflight[ep][i] = data[i];
        }
    }
    g_inflight_len[ep] = len;
    in->DIEPTSIZ = (1u << 19) | len;
    const uint16_t words = (uint16_t)((len + 3u) / 4u);
    const uint32_t free_words = in->DTXFSTS & 0xFFFFu;
    /* DWC2 non-DMA IN: arm the endpoint before filling its TX FIFO,
     * matching ST USB_EPStartXfer. Data written while disabled can be lost. */
    in->DIEPCTL |= EPENA | CNAK;
    if (len > 0u && free_words < words) {
        g_inflight_wait[ep] = true;
        USB_OTG_DEVICE->DIEPEMPMSK |= (1u << ep);
        return;
    }
    g_inflight_wait[ep] = false;
    if (len > 0u) {
        fifo_write(ep, g_inflight[ep], len);
    }
}
static void ep0_prepare_setup(void)
{
    USB_OTG_OUTEndpointTypeDef *ep = USB_OUTEP(0);
    if ((ep->DOEPCTL & EPENA) != 0u) {
        return;
    }
    ep->DOEPTSIZ = (3u << 29) | (1u << 19) | 24u;
    ep->DOEPCTL |= EPENA | CNAK;
}
static void ep0_status_in(void)
{
    trace(3u, USB_OTG_DEVICE->DCFG, USB_INEP(0)->DIEPINT);
    g_ep0_state = EP0_STATUS_IN;
    g_status_gen = g_ctrl_gen;
    g_in_busy[0] = true;
    start_in(0u, 0, 0u);
}
static void ep0_arm_status_out(void)
{
    USB_OUTEP(0)->DOEPTSIZ = (1u << 19) | 64u;
    USB_OUTEP(0)->DOEPCTL |= EPENA | CNAK;
}
static void ep0_stall(void)
{
    USB_INEP(0)->DIEPCTL |= STALL_BIT;
    USB_OUTEP(0)->DOEPCTL |= STALL_BIT;
    g_ep0_state = EP0_SETUP;
    g_in_busy[0] = false;
    ep0_prepare_setup();
}
static void ep0_send_next(void)
{
    const uint16_t left = (uint16_t)(g_ep0_total - g_ep0_sent);
    if (left == 0u) {
        if (g_ep0_zlp) {
            g_ep0_zlp = false;
            g_in_busy[0] = true;
            start_in(0u, 0, 0u);
            return;
        }
        g_ep0_state = EP0_STATUS_OUT;
        ep0_arm_status_out();
        return;
    }
    uint16_t n = left > 64u ? 64u : left;
    const uint16_t offset = g_ep0_sent;
    g_ep0_sent = (uint16_t)(g_ep0_sent + n);
    if (g_ep0_sent == g_ep0_total && n == 64u && g_ep0_total < g_ep0_wlength) {
        g_ep0_zlp = true;
    }
    g_in_busy[0] = true;
    start_in(0u, g_ep0_src + offset, n);
}
static void ep0_begin_in(const uint8_t *data, uint16_t len, uint16_t w_length)
{
    if (len > w_length) {
        len = w_length;
    }
    if (len == 0u || data == 0) {
        ep0_status_in();
        return;
    }
    g_ep0_src = data;
    g_ep0_total = len;
    g_ep0_sent = 0u;
    g_ep0_wlength = w_length;
    g_ep0_zlp = false;
    g_ep0_state = EP0_DATA_IN;
    ep0_send_next();
}
static void bulk_out_arm(void)
{
    USB_OUTEP(1)->DOEPTSIZ = (1u << 19) | 64u;
    USB_OUTEP(1)->DOEPCTL |= EPENA | CNAK;
}
static void open_data_eps(void)
{
    clear_async_tx();
    USB_INEP(1)->DIEPCTL = 64u | (2u << 18) | (1u << 22) | USBAEP | SD0PID;
    USB_OUTEP(1)->DOEPCTL = 64u | (2u << 18) | USBAEP | SD0PID;
    /* Interrupt IN stays NAK. No serial-state notifications. */
    USB_INEP(2)->DIEPCTL = 8u | (3u << 18) | (2u << 22) | USBAEP | SD0PID | SNAK;
    USB_OTG_DEVICE->DAINTMSK |= (1u << 1) | (1u << 2) | (1u << 17);
    g_configured = true;
    g_bulk_zlp = false;
    bulk_out_arm();
}
static void close_data_eps(void)
{
    g_configured = false;
    clear_async_tx();
    if (!g_usb_clock_on) {
        return;
    }
    USB_OTG_DEVICE->DAINTMSK &= ~((1u << 1) | (1u << 2) | (1u << 17));
    USB_INEP(1)->DIEPCTL |= SNAK;
    USB_OUTEP(1)->DOEPCTL |= SNAK;
    USB_INEP(2)->DIEPCTL |= SNAK;
    (void)flush_tx_all();
}
static void kick_bulk(void)
{
    if (!g_configured || g_in_busy[1] || g_tx_count == 0u) {
        return;
    }
    uint16_t n = g_tx_count > 64u ? 64u : g_tx_count;
    for (uint16_t i = 0u; i < n; i++) {
        g_inflight[1][i] = g_txq[g_tx_tail];
        g_tx_tail = (uint16_t)((g_tx_tail + 1u) & (TXQ_SIZE - 1u));
    }
    g_tx_count = (uint16_t)(g_tx_count - n);
    g_inflight_len[1] = n;
    g_in_busy[1] = true;
    g_bulk_zlp = false;
    start_in(1u, g_inflight[1], n);
}
/* A new SETUP cancels the application configuration commit. DWC2 address
 * programming follows the core's SET_ADDRESS handshake, like ST's LL driver. */
static void abort_pending_control(void)
{
    g_ctrl_gen++;
    g_cfg_commit = CFG_COMMIT_NONE;
}
static void handle_setup(const uint8_t setup[8])
{
    trace(1u, (uint32_t)setup[0] | ((uint32_t)setup[1] << 8) |
          ((uint32_t)setup[2] << 16) | ((uint32_t)setup[3] << 24),
          (uint32_t)setup[4] | ((uint32_t)setup[5] << 8) |
          ((uint32_t)setup[6] << 16) | ((uint32_t)setup[7] << 24));
    mkr_usb_setup_result_t result;
    abort_pending_control();
    mkr_usb_setup_decode(setup, g_configured, g_line, &result);
    switch (result.kind) {
    case MKR_USB_SETUP_CONFIGURED:
        /* USB 2.0 9.1.1.5 / 9.4.7: Configured begins after the status stage.
         * Data endpoints and the ISO-power event wait for that Status IN. */
        g_cfg_commit = CFG_COMMIT_SET;
        ep0_status_in();
        break;
    case MKR_USB_SETUP_UNCONFIGURED:
        g_cfg_commit = CFG_COMMIT_CLEAR;
        ep0_status_in();
        break;
    case MKR_USB_SETUP_SET_ADDRESS:
        g_addr = (uint8_t)result.value;
        /* DWC2 requires DAD before starting the status ZLP (ST USBD_SetAddress,
         * USB_SetDevAddress). Do not defer this register write to IN XFRC. */
        USB_OTG_DEVICE->DCFG = (USB_OTG_DEVICE->DCFG & ~(0x7Fu << 4)) | ((uint32_t)g_addr << 4);
        trace(7u, g_addr, USB_OTG_DEVICE->DCFG);
        ep0_status_in();
        break;
    case MKR_USB_SETUP_SET_LINE_CODING:
        g_ep0_state = EP0_DATA_OUT;
        USB_OUTEP(0)->DOEPTSIZ = (1u << 19) | 64u;
        USB_OUTEP(0)->DOEPCTL |= EPENA | CNAK;
        break;
    case MKR_USB_SETUP_IN_DATA:
        ep0_begin_in(result.data, result.len, result.w_length);
        break;
    case MKR_USB_SETUP_STATUS_IN:
        ep0_status_in();
        break;
    case MKR_USB_SETUP_HALT_EP: {
        const uint8_t ep = (uint8_t)(result.value & 0x7Fu);
        const bool is_in = ((result.value & 0x80u) != 0u);
        if (ep < 3u) {
            if (is_in) {
                USB_INEP(ep)->DIEPCTL |= STALL_BIT;
            } else {
                USB_OUTEP(ep)->DOEPCTL |= STALL_BIT;
            }
        }
        ep0_status_in();
        break;
    }
    case MKR_USB_SETUP_CLEAR_HALT: {
        const uint8_t ep = (uint8_t)(result.value & 0x7Fu);
        const bool is_in = ((result.value & 0x80u) != 0u);
        if (ep < 3u) {
            if (is_in) {
                USB_INEP(ep)->DIEPCTL = (USB_INEP(ep)->DIEPCTL & ~STALL_BIT) | SD0PID;
            } else {
                USB_OUTEP(ep)->DOEPCTL = (USB_OUTEP(ep)->DOEPCTL & ~STALL_BIT) | SD0PID;
            }
        }
        ep0_status_in();
        break;
    }
    case MKR_USB_SETUP_STALL:
    default:
        ep0_stall();
        break;
    }
}
static void drain_rx(void)
{
    for (uint32_t guard = 0u; guard < 16u && (MKR_GINTSTS_READ() & RXFLVL) != 0u; guard++) {
        const uint32_t pop = MKR_USB_RXSTSP();
        const uint8_t ep = (uint8_t)(pop & 0xFu);
        const uint16_t bcnt = (uint16_t)((pop >> 4) & 0x7FFu);
        const uint8_t pkt = (uint8_t)((pop >> 17) & 0xFu);
        if (pkt == PKTSTS_SETUP) {
            fifo_read(g_setup, bcnt, 8u);
            g_setup_valid = (bcnt == 8u);
        } else if (pkt == PKTSTS_OUT) {
            fifo_read(g_out, bcnt, 64u);
            g_out_len = bcnt > 64u ? 64u : bcnt;
            g_out_ep = ep;
        } else if (pkt == PKTSTS_SETUP_DONE && g_setup_valid) {
            g_setup_valid = false;
            handle_setup(g_setup);
        }
    }
}
static void commit_config(void)
{
    if (g_cfg_commit == CFG_COMMIT_SET) {
        g_cfg_commit = CFG_COMMIT_NONE;
        open_data_eps();
        emit(MKR_POWER_EVENT_USB_CONFIGURED);
    } else if (g_cfg_commit == CFG_COMMIT_CLEAR) {
        g_cfg_commit = CFG_COMMIT_NONE;
        /* Safe-off must happen before close_data_eps() waits for FIFO flush. */
        emit(MKR_POWER_EVENT_USB_UNCONFIGURED);
        close_data_eps();
    }
}
static void on_ep0_in_done(void)
{
    trace(4u, USB_OTG_DEVICE->DCFG, USB_INEP(0)->DIEPTSIZ);
    g_in_busy[0] = false;
    if (g_ep0_state == EP0_DATA_IN) {
        ep0_send_next();
        return;
    }
    if (g_ep0_state != EP0_STATUS_IN || g_status_gen != g_ctrl_gen) {
        return;
    }
    /* If reset became pending during endpoint handling, do not commit
     * configuration or power-up before the reset handler runs. */
    if ((MKR_GINTSTS_READ() & USBRST) != 0u) {
        return;
    }
    commit_config();
    g_ep0_state = EP0_SETUP;
    ep0_prepare_setup();
}
static void on_in_xfrc(uint8_t ep)
{
    if (ep == 0u) {
        on_ep0_in_done();
        return;
    }
    if (ep != 1u) {
        g_in_busy[ep] = false;
        return;
    }
    g_in_busy[1] = false;
    if (g_inflight_len[1] == 64u && g_tx_count == 0u && !g_bulk_zlp) {
        g_inflight_len[1] = 0u;
        g_in_busy[1] = true;
        g_bulk_zlp = true;
        start_in(1u, 0, 0u);
        return;
    }
    g_bulk_zlp = false;
    g_inflight_len[1] = 0u;
    kick_bulk();
}
static void on_txfe(uint8_t ep)
{
    if (!g_inflight_wait[ep]) {
        USB_OTG_DEVICE->DIEPEMPMSK &= ~(1u << ep);
        return;
    }
    const uint32_t words = ((uint32_t)g_inflight_len[ep] + 3u) / 4u;
    if ((USB_INEP(ep)->DTXFSTS & 0xffffu) < words) {
        return;
    }
    g_inflight_wait[ep] = false;
    if (g_inflight_len[ep] > 0u) {
        fifo_write(ep, g_inflight[ep], g_inflight_len[ep]);
    }
    USB_OTG_DEVICE->DIEPEMPMSK &= ~(1u << ep);
}
static void on_out_xfrc(uint8_t ep)
{
    trace(5u, ep, USB_OUTEP(0)->DOEPTSIZ);
    if (ep == 0u) {
        if (g_ep0_state == EP0_DATA_OUT && g_out_ep == 0u && g_out_len >= 7u) {
            for (uint8_t i = 0u; i < 7u; i++) {
                g_line[i] = g_out[i];
            }
            ep0_status_in();
        } else if (g_ep0_state == EP0_STATUS_OUT) {
            g_ep0_state = EP0_SETUP;
            ep0_prepare_setup();
        }
        return;
    }
    /* Do not dispatch a stale OUT marker once a bus reset is pending. */
    if (ep == 1u && (MKR_GINTSTS_READ() & USBRST) == 0u &&
        g_configured && g_out_ep == 1u && g_out_len > 0u && g_on_rx != 0) {
        g_on_rx(g_out, g_out_len, g_ctx);
    }
    if (g_configured) {
        bulk_out_arm();
    }
}
static void handle_oep(void)
{
    const uint32_t daint = USB_OTG_DEVICE->DAINT & USB_OTG_DEVICE->DAINTMSK;
    for (uint8_t ep = 0u; ep < 2u; ep++) {
        if ((daint & (1u << (16u + ep))) == 0u) {
            continue;
        }
        uint32_t intr = MKR_DOEPINT_READ(ep) & USB_OTG_DEVICE->DOEPMSK;
        if ((intr & XFRC) != 0u) {
            on_out_xfrc(ep);
        }
        if ((intr & STUP) != 0u && g_setup_valid) {
            g_setup_valid = false;
            handle_setup(g_setup);
        }
        if (intr != 0u) {
            MKR_DOEPINT_WRITE(ep, intr);
        }
    }
}
static void handle_iep(void)
{
    const uint32_t daint = USB_OTG_DEVICE->DAINT & USB_OTG_DEVICE->DAINTMSK;
    for (uint8_t ep = 0u; ep < 3u; ep++) {
        if ((daint & (1u << ep)) == 0u) {
            continue;
        }
        uint32_t mask = USB_OTG_DEVICE->DIEPMSK;
        if ((USB_OTG_DEVICE->DIEPEMPMSK & (1u << ep)) != 0u) {
            mask |= TXFE;
        }
        const uint32_t intr = MKR_DIEPINT_READ(ep) & mask;
        if ((intr & TOC) != 0u) {
            trace(6u, ep, intr);
            MKR_DIEPINT_WRITE(ep, TOC);
            if (ep == 0u) {
                abort_pending_control();
                g_in_busy[0] = false;
                g_ep0_state = EP0_SETUP;
                ep0_prepare_setup();
            }
        }
        if ((intr & XFRC) != 0u) {
            MKR_DIEPINT_WRITE(ep, XFRC);
            on_in_xfrc(ep);
        }
        if ((intr & TXFE) != 0u) {
            on_txfe(ep);
        }
        const uint32_t other = intr & ~(XFRC | TXFE | TOC);
        if (other != 0u) {
            MKR_DIEPINT_WRITE(ep, other);
        }
    }
}
static void handle_reset(void)
{
    abort_pending_control();
    g_configured = false;
    /* A USB reset is a safety event, not a FIFO housekeeping event.
     * Clear Marker/READY and then PA15 before any register wait/flush. */
    emit(MKR_POWER_EVENT_USB_BUS_RESET);
    clear_async_tx();
    g_out_len = 0u;
    g_out_ep = 0u;
    g_addr = 0u;
    g_setup_valid = false;
    g_ep0_state = EP0_SETUP;
    USB_OTG_PCGCCTL = 0u;
    (void)flush_tx_all();
    (void)flush_rx();
    USB_OTG_DEVICE->DIEPMSK = XFRC | TOC | (1u << 1);
    USB_OTG_DEVICE->DOEPMSK = XFRC | STUP;
    USB_OTG_DEVICE->DAINTMSK = (1u << 0) | (1u << 16);
    USB_OTG_DEVICE->DCFG &= ~(0x7Fu << 4);
    for (uint8_t ep = 0u; ep < 3u; ep++) {
        USB_INEP(ep)->DIEPTSIZ = 0u;
        MKR_DIEPINT_WRITE(ep, 0xFFu);
        USB_OUTEP(ep)->DOEPTSIZ = 0u;
        MKR_DOEPINT_WRITE(ep, 0xFFu);
    }
    USB_INEP(0)->DIEPCTL = USBAEP;
    USB_OUTEP(0)->DOEPCTL = USBAEP;
    /* Bus reset drops data-endpoint enables in the core. SNAK is the
     * software shadow so a stale EPENA is not what kick_bulk sees. */
    USB_INEP(1)->DIEPCTL = SNAK;
    USB_OUTEP(1)->DOEPCTL = SNAK;
    USB_INEP(2)->DIEPCTL = SNAK;
    ep0_prepare_setup();
}
/* A reset must preempt stale RX packets and endpoint completions, including
 * when several USB interrupt causes are asserted in the same IRQ entry. */
static bool service_pending_reset(void)
{
    if ((MKR_GINTSTS_READ() & USB_OTG_FS->GINTMSK & USBRST) == 0u) {
        return false;
    }
    MKR_GINTSTS_WRITE(USBRST);
    USB_OTG_PCGCCTL &= ~STOPCLK;
    g_stop_requested = 0u;
    /* A simultaneous wake/suspend flag must not emit a stale resume after
     * reset has already made the application unconfigured and safe. */
    const uint32_t stale = MKR_GINTSTS_READ() & (USBSUSP | WKUINT);
    if (stale != 0u) {
        MKR_GINTSTS_WRITE(stale);
    }
    handle_reset();
    return true;
}
static void handle_enum(void)
{
    USB_OTG_FS->GUSBCFG = (USB_OTG_FS->GUSBCFG & ~(0xFu << 10)) | (6u << 10);
    ep0_prepare_setup();
}
static void usb_pins(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    for (uint32_t pin = 11u; pin <= 12u; pin++) {
        const uint32_t shift = pin * 2u;
        const uint32_t afr = (pin - 8u) * 4u;
        GPIOA->MODER = (GPIOA->MODER & ~(3u << shift)) | (2u << shift);
        GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3u << shift)) | (3u << shift);
        GPIOA->OTYPER &= ~(1u << pin);
        GPIOA->PUPDR &= ~(3u << shift);
        GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFu << afr)) | (0xAu << afr);
    }
}
static bool core_reset(void)
{
    if (!board_wait_set(&USB_OTG_FS->GRSTCTL, AHBIDL, USB_REG_TIMEOUT)) {
        return false;
    }
    board_delay_cycles(64u);
    USB_OTG_FS->GRSTCTL = CSRST;
    if (!board_wait_clear(&USB_OTG_FS->GRSTCTL, CSRST, USB_REG_TIMEOUT)) {
        return false;
    }
    return board_wait_set(&USB_OTG_FS->GRSTCTL, AHBIDL, USB_REG_TIMEOUT);
}
bool mkr_usb_dcd_start(const mkr_usb_dcd_cbs_t *cbs)
{
    if (cbs != 0) {
        g_on_event = cbs->on_event;
        g_on_rx = cbs->on_rx;
        g_ctx = cbs->ctx;
    }
    usb_pins();
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)RCC->AHB2ENR;
    g_usb_clock_on = true;
    g_configured = false;
    g_cfg_commit = CFG_COMMIT_NONE;
    g_ctrl_gen = 0u;
    g_status_gen = 0u;
    clear_async_tx();
    USB_OTG_DEVICE->DCTL |= SDIS;
    USB_OTG_FS->GUSBCFG |= PHYSEL;
    if (!core_reset()) {
        return false;
    }
    USB_OTG_FS->GUSBCFG |= PHYSEL;
    USB_OTG_FS->GUSBCFG &= ~(FHMOD | FDMOD);
    USB_OTG_FS->GUSBCFG |= FDMOD;
    /* RM0383: wait at least 25 ms after forcing device mode.
     * 25 * (SYSCLK/1000) cycles. 25 * SYSCLK would be ~25 s at 84 MHz. */
    _Static_assert(MKR_CYCLES_FROM_MS_CONST(MKR_USB_DEVICE_MODE_WAIT_MS) == 2100000u,
                   "device-mode wait is 25 ms");
    board_delay_ms(MKR_USB_DEVICE_MODE_WAIT_MS);
    if ((MKR_GINTSTS_READ() & 1u) != 0u) {
        return false;
    }
    USB_OTG_FS->GCCFG |= GCCFG_PWRDWN | GCCFG_NOVBUSSENS;
    USB_OTG_FS->GCCFG &= ~(GCCFG_VBUSASEN | GCCFG_VBUSBSEN);
    USB_OTG_PCGCCTL = 0u;
    USB_OTG_DEVICE->DCFG = (USB_OTG_DEVICE->DCFG & ~3u) | 3u | (1u << 2);
    USB_OTG_FS->GUSBCFG = (USB_OTG_FS->GUSBCFG & ~(0xFu << 10)) | (6u << 10);
    if (!flush_tx_all() || !flush_rx()) {
        return false;
    }
    USB_OTG_FS->GRXFSIZ = RX_FIFO_WORDS;
    USB_OTG_FS->DIEPTXF0_HNPTXFSIZ = (TX0_FIFO_WORDS << 16) | RX_FIFO_WORDS;
    USB_OTG_FS->DIEPTXF[0] = (TX1_FIFO_WORDS << 16) | (RX_FIFO_WORDS + TX0_FIFO_WORDS);
    USB_OTG_FS->DIEPTXF[1] = (TX2_FIFO_WORDS << 16) | (RX_FIFO_WORDS + TX0_FIFO_WORDS + TX1_FIFO_WORDS);
    USB_OTG_DEVICE->DIEPMSK = XFRC | TOC | (1u << 1);
    USB_OTG_DEVICE->DOEPMSK = XFRC | STUP;
    USB_OTG_DEVICE->DAINTMSK = (1u << 0) | (1u << 16);
    USB_INEP(0)->DIEPCTL = USBAEP;
    USB_OUTEP(0)->DOEPCTL = USBAEP;
    USB_OTG_FS->GINTMSK = 0u;
    MKR_GINTSTS_WRITE(0xBFFFFFFFu);
    USB_OTG_FS->GINTMSK = RXFLVL | USBSUSP | USBRST | ENUMDNE | IEPINT | OEPINT | WKUINT;
    g_ep0_state = EP0_SETUP;
    ep0_prepare_setup();
    board_nvic_enable(OTG_FS_IRQn, 1u);
    /* EXTI line 18, rising edge. More urgent than the OTG IRQ so the PLL is
     * back before GINTSTS is serviced. Armed only after the core starts. */
    EXTI->FTSR &= ~MKR_EXTI_USB_WAKEUP;
    EXTI->RTSR |= MKR_EXTI_USB_WAKEUP;
    EXTI->PR = MKR_EXTI_USB_WAKEUP;
    EXTI->IMR |= MKR_EXTI_USB_WAKEUP;
    board_nvic_enable(OTG_FS_WKUP_IRQn, 0u);
    USB_OTG_FS->GAHBCFG |= GINT_BIT;
    USB_OTG_DEVICE->DCTL &= ~SDIS;
    return true;
}
int mkr_usb_dcd_write(const uint8_t *data, uint16_t len)
{
    if (!g_configured || data == 0) {
        return -1;
    }
    if ((uint16_t)(g_tx_count + len) > TXQ_SIZE) {
        return -1;
    }
    for (uint16_t i = 0u; i < len; i++) {
        g_txq[g_tx_head] = data[i];
        g_tx_head = (uint16_t)((g_tx_head + 1u) & (TXQ_SIZE - 1u));
    }
    g_tx_count = (uint16_t)(g_tx_count + len);
    kick_bulk();
    return 0;
}
void mkr_usb_dcd_disconnect(void)
{
    if (!g_usb_clock_on) {
        return;
    }
    USB_OTG_DEVICE->DCTL |= SDIS;
}
bool mkr_usb_dcd_stop_requested(void)
{
    return g_stop_requested != 0u;
}
void mkr_usb_dcd_on_exti_wakeup(void)
{
    uint32_t sts;
    if (!g_usb_clock_on) {
        return;
    }
    /* EXTI line 18 is the wakeup detector. Do not require WKUINT or
     * SUSPSTS==0; both can lag the edge. Clear the stale suspend flag so
     * this same wake does not immediately undo resume. A bus that is still
     * idle raises USBSUSP again later. */
    USB_OTG_PCGCCTL &= ~STOPCLK;
    if (g_stop_requested == 0u) {
        return;
    }
    sts = MKR_GINTSTS_READ();
    if ((sts & USBRST) != 0u) {
        g_stop_requested = 0u;
        return;
    }
    if ((sts & WKUINT) != 0u) {
        MKR_GINTSTS_WRITE(WKUINT);
    }
    if ((MKR_GINTSTS_READ() & USBSUSP) != 0u) {
        MKR_GINTSTS_WRITE(USBSUSP);
    }
    g_stop_requested = 0u;
    emit(MKR_POWER_EVENT_USB_RESUME);
}
void OTG_FS_IRQHandler(void)
{
    uint32_t sts;
#if MKR_BRINGUP_AWAKE
    /* Keep the USB peripheral awake throughout enumeration diagnostics. */
    USB_OTG_PCGCCTL &= ~STOPCLK;
#endif
    trace(2u, MKR_GINTSTS_READ(), USB_OTG_DEVICE->DAINT);
    /* Reset is serviced before draining RX or delivering a stale OUT/IN
     * completion. Recheck between stages if reset arrives during this IRQ. */
    if (service_pending_reset()) {
        return;
    }
    drain_rx();
    if (service_pending_reset()) {
        return;
    }
    sts = MKR_GINTSTS_READ() & USB_OTG_FS->GINTMSK;
    if ((sts & OEPINT) != 0u) {
        handle_oep();
    }
    if (service_pending_reset()) {
        return;
    }
    sts = MKR_GINTSTS_READ() & USB_OTG_FS->GINTMSK;
    if ((sts & IEPINT) != 0u) {
        handle_iep();
    }
    if (service_pending_reset()) {
        return;
    }
    sts = MKR_GINTSTS_READ() & USB_OTG_FS->GINTMSK;
    if ((sts & WKUINT) != 0u) {
        USB_OTG_PCGCCTL &= ~STOPCLK;
    }
    if ((sts & ENUMDNE) != 0u) {
        MKR_GINTSTS_WRITE(ENUMDNE);
        handle_enum();
    }
    if (service_pending_reset()) {
        return;
    }
    sts = MKR_GINTSTS_READ() & USB_OTG_FS->GINTMSK;
    if ((sts & WKUINT) != 0u) {
        MKR_GINTSTS_WRITE(WKUINT);
        if ((MKR_GINTSTS_READ() & USBSUSP) != 0u) {
            MKR_GINTSTS_WRITE(USBSUSP);
        }
        g_stop_requested = 0u;
        emit(MKR_POWER_EVENT_USB_RESUME);
    } else if ((sts & USBSUSP) != 0u && (USB_OTG_DEVICE->DSTS & DSTS_SUSPSTS) != 0u) {
        /* Markers and PA15 are cleared by the power sequence before Stop. */
        MKR_GINTSTS_WRITE(USBSUSP);
        emit(MKR_POWER_EVENT_USB_SUSPEND);
#if MKR_BRINGUP_AWAKE
        /* This build never enters MCU Stop; isolate PHY gating as a variable. */
        g_stop_requested = 0u;
#else
        USB_OTG_PCGCCTL |= STOPCLK;
        g_stop_requested = 1u;
#endif
    } else if ((sts & USBSUSP) != 0u) {
        MKR_GINTSTS_WRITE(USBSUSP);
    }
}
#ifdef MKR_USB_HOST_TEST
bool mkr_usb_dcd_test_configured(void) { return g_configured; }
bool mkr_usb_dcd_test_inflight_wait(uint8_t ep) { return ep < 3u && g_inflight_wait[ep]; }
bool mkr_usb_dcd_test_in_busy(uint8_t ep) { return ep < 3u && g_in_busy[ep]; }
uint16_t mkr_usb_dcd_test_tx_count(void) { return g_tx_count; }
bool mkr_usb_dcd_test_bulk_zlp(void) { return g_bulk_zlp; }
uint8_t mkr_usb_dcd_test_cfg_commit(void) { return g_cfg_commit; }
#endif
