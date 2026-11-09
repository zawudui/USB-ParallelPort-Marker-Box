#include "protocol.h"
#include <assert.h>
int main(void) { const uint8_t expected[5]={0xAAu,0x03u,0x10u,0x2Au,0x93u}; uint8_t frame[5]; mkr_build_send_marker(42u,frame); for (unsigned i=0;i<5u;i++) assert(frame[i]==expected[i]); assert(mkr_xor_chk(frame,4u)==frame[4]); return 0; }
