#ifndef PS2_EEKERNEL_H
#define PS2_EEKERNEL_H

// The subset of the Sony EE kernel interface (eekernel.h) used by the PS2
// platform layer.

#ifdef __cplusplus
extern "C" {
#endif

#define INTC_GS 0
#define INTC_SBUS 1
#define INTC_VBLANK_S 2
#define INTC_VBLANK_E 3
#define INTC_VIF0 4
#define INTC_VIF1 5
#define INTC_VU0 6
#define INTC_VU1 7
#define INTC_IPU 8
#define INTC_TIM0 9
#define INTC_TIM1 10

int AddIntcHandler(int cause, int (*handler)(int), int next);
int AddIntcHandler2(int cause, int (*handler)(int, void*, void*), int next, void* arg);
int RemoveIntcHandler(int cause, int hid);
int EnableIntc(int cause);
int DisableIntc(int cause);

void FlushCache(int operation);

#ifdef __cplusplus
}
#endif

// Interrupt handlers must re-enable interrupts on return.
#define ExitHandler() asm volatile("sync.l; ei")

#endif
