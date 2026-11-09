#ifndef MKR_BOARD_CRIT_H
#define MKR_BOARD_CRIT_H
/* PRIMASK critical section. Nesting is counted. NMI is not masked. */
void board_crit_lock(void *ctx);
void board_crit_unlock(void *ctx);
#endif
