#include "power_sequence.h"
#include "time_us.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
typedef enum { ACTION_MARKER=1, ACTION_POWER=2 } action_t;
typedef struct { action_t actions[64]; uint32_t action_count; uint8_t code; bool ready; bool power_enabled; } fake_board_t;
static void wr(uint8_t code,bool ready,void *context){fake_board_t*b=context;b->actions[b->action_count++]=ACTION_MARKER;b->code=ready?code:0u;b->ready=ready;}
static void pw(bool enabled,void *context){fake_board_t*b=context;b->actions[b->action_count++]=ACTION_POWER;b->power_enabled=enabled;}
static void clear_actions(fake_board_t*b){b->action_count=0u;}
static void one_tick(mkr_power_sequence_t *s)
{
    mkr_time_soft_advance_us(1000u);
    mkr_power_sequence_tick_1ms(s);
}
static void ticks(mkr_power_sequence_t *s, uint32_t count)
{
    for (uint32_t i = 0u; i < count; i++) {
        one_tick(s);
    }
}
static void assert_safe(const fake_board_t*b){assert(b->code==0u);assert(!b->ready);assert(!b->power_enabled);}
static void assert_off_order(const fake_board_t*b){assert(b->action_count==2u);assert(b->actions[0]==ACTION_MARKER);assert(b->actions[1]==ACTION_POWER);}
int main(void){
 fake_board_t b={0};mkr_power_sequence_t s;mkr_power_io_t io={wr,pw,&b};
 mkr_power_sequence_init(&s,&io);assert(s.state==MKR_POWER_STATE_UNCONFIGURED);assert_off_order(&b);assert_safe(&b);
 clear_actions(&b);mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);
 assert(s.state==MKR_POWER_STATE_WAIT_CONFIG_DELAY);assert(s.timer_ms==MKR_POWER_CONFIG_DELAY_MS);assert_safe(&b);
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS-1u);assert(!b.power_enabled);one_tick(&s);
 assert(s.state==MKR_POWER_STATE_WAIT_ISO_STABLE);assert(s.timer_ms==MKR_POWER_ISO_SETTLE_MS);assert(b.power_enabled&&!b.ready);
 ticks(&s,MKR_POWER_ISO_SETTLE_MS-1u);assert(!b.ready);one_tick(&s);
 assert(s.state==MKR_POWER_STATE_READY&&b.ready&&b.power_enabled&&b.code==0u);
 clear_actions(&b);mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_SUSPEND);
 assert(s.state==MKR_POWER_STATE_SUSPENDED);assert_off_order(&b);assert_safe(&b);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_RESUME);assert(s.state==MKR_POWER_STATE_WAIT_CONFIG_DELAY);
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS-1u);assert(!b.power_enabled);one_tick(&s);
 assert(b.power_enabled&&!b.ready);ticks(&s,MKR_POWER_ISO_SETTLE_MS);assert(s.state==MKR_POWER_STATE_READY&&b.ready);
 /* Suspend while waiting for initial power-up; resume must restart the 10 ms delay. */
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);ticks(&s,3u);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_SUSPEND);assert(s.state==MKR_POWER_STATE_SUSPENDED);assert_safe(&b);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_RESUME);assert(s.timer_ms==MKR_POWER_CONFIG_DELAY_MS);
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS-1u);assert(!b.power_enabled);one_tick(&s);assert(b.power_enabled);
 /* Suspend while settling must cut power; resume restarts both waits. */
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_SUSPEND);assert_safe(&b);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_RESUME);ticks(&s,MKR_POWER_CONFIG_DELAY_MS);
 assert(s.state==MKR_POWER_STATE_WAIT_ISO_STABLE&&b.power_enabled);ticks(&s,MKR_POWER_ISO_SETTLE_MS);
 assert(s.state==MKR_POWER_STATE_READY&&b.ready);
 /* Unconfigure during WAIT_CONFIG_DELAY cancels enable immediately. */
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);ticks(&s,4u);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_UNCONFIGURED);
 assert(s.state==MKR_POWER_STATE_UNCONFIGURED&&!s.usb_configured);assert_safe(&b);
 ticks(&s,2u*MKR_POWER_ISO_SETTLE_MS);assert(s.state==MKR_POWER_STATE_UNCONFIGURED);assert_safe(&b);
 /* Unconfigure during WAIT_ISO_STABLE also shuts down immediately. */
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);ticks(&s,MKR_POWER_CONFIG_DELAY_MS);
 assert(s.state==MKR_POWER_STATE_WAIT_ISO_STABLE&&b.power_enabled);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_UNCONFIGURED);assert_safe(&b);
 ticks(&s,2u*MKR_POWER_ISO_SETTLE_MS);assert_safe(&b);
 /* Resume without a configured USB device must leave isolated power off. */
 mkr_power_sequence_init(&s,&io);mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_SUSPEND);
 assert(s.state==MKR_POWER_STATE_SUSPENDED&&!s.usb_configured);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_RESUME);
 assert(s.state==MKR_POWER_STATE_UNCONFIGURED);assert_safe(&b);
 /* Repeated CONFIGURED restarts the delay, including if already settling. */
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);ticks(&s,MKR_POWER_CONFIG_DELAY_MS-1u);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);
 assert(s.timer_ms==MKR_POWER_CONFIG_DELAY_MS);assert_safe(&b);
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS-1u);assert(!b.power_enabled);one_tick(&s);assert(b.power_enabled);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);assert_safe(&b);
 assert(s.state==MKR_POWER_STATE_WAIT_CONFIG_DELAY&&s.timer_ms==MKR_POWER_CONFIG_DELAY_MS);
 /* Clock faults force safe-off and remain latched until MCU reset. */
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS);assert(b.power_enabled);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_CLOCK_FAULT);assert(s.state==MKR_POWER_STATE_FAULT);assert_safe(&b);
 clear_actions(&b);mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);
 ticks(&s,2u*MKR_POWER_ISO_SETTLE_MS);assert(s.state==MKR_POWER_STATE_FAULT&&b.action_count==0u);assert_safe(&b);
 /* Bus reset clears configuration and disables power immediately. */
 mkr_power_sequence_init(&s,&io);mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_CONFIGURED);
 ticks(&s,MKR_POWER_CONFIG_DELAY_MS);assert(b.power_enabled);
 mkr_power_sequence_handle_event(&s,MKR_POWER_EVENT_USB_BUS_RESET);
 assert(s.state==MKR_POWER_STATE_UNCONFIGURED&&!s.usb_configured);assert_safe(&b);
 /* Worst 1 ms phase: event 1 µs after a tick. PA15 stays off through the
  * tick 9.999 ms later and rises at the 10.000 ms deadline. */
 {
  const uint32_t t = mkr_hw_time_us();
  mkr_time_soft_set_us(t + 1u);
  mkr_power_sequence_handle_event(&s, MKR_POWER_EVENT_USB_CONFIGURED);
  assert(s.deadline_us == t + 1u + 10000u);
  assert(!b.power_enabled);
  for (uint32_t step = 1000u; step <= 10000u; step += 1000u) {
   mkr_time_soft_set_us(t + step);
   mkr_power_sequence_tick_1ms(&s);
   assert(!b.power_enabled);
  }
  mkr_time_soft_set_us(t + 1u + 9999u);
  mkr_power_sequence_tick_1ms(&s);
  assert(!b.power_enabled);
  mkr_time_soft_set_us(s.deadline_us);
  mkr_power_sequence_tick_1ms(&s);
  assert(b.power_enabled);
  assert(mkr_hw_time_us() - (t + 1u) >= 10000u);
  assert(s.deadline_us == mkr_hw_time_us() + (uint32_t)MKR_POWER_ISO_SETTLE_MS * 1000u);
  mkr_time_soft_set_us(s.deadline_us - 1u);
  mkr_power_sequence_tick_1ms(&s);
  assert(!b.ready);
  mkr_time_soft_set_us(s.deadline_us);
  mkr_power_sequence_tick_1ms(&s);
  assert(b.ready && b.power_enabled);
 }
 return 0;
}
