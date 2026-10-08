#ifndef PS2_LIBPAD_H
#define PS2_LIBPAD_H

// The subset of the Sony controller library interface (libpad.h) used by the
// PS2 platform layer.

#define scePadStateDiscon 0
#define scePadStateFindPad 1
#define scePadStateFindCTP1 2
#define scePadStateExecCmd 5
#define scePadStateStable 6
#define scePadStateError 7

#define scePadReqStateComplete 0
#define scePadReqStateFaild 1
#define scePadReqStateBusy 2

#define InfoModeCurID 1
#define InfoModeCurExID 2
#define InfoModeCurExOffs 3
#define InfoModeIdTable 4

#define InfoActFunc 1
#define InfoActSub 2
#define InfoActSize 3
#define InfoActCurr 4

#ifdef __cplusplus
extern "C" {
#endif

int scePadInit(int mode);
int scePadPortOpen(int port, int slot, void* addr);
int scePadPortClose(int port, int slot);
int scePadRead(int port, int slot, unsigned char* rdata);
int scePadGetState(int port, int slot);
int scePadGetReqState(int port, int slot);
int scePadInfoMode(int port, int slot, int term, int offs);
int scePadSetMainMode(int port, int slot, int offs, int lock);
int scePadInfoAct(int port, int slot, int actno, int term);
int scePadSetActAlign(int port, int slot, const unsigned char* data);
int scePadSetActDirect(int port, int slot, const unsigned char* data);

#ifdef __cplusplus
}
#endif

#endif
