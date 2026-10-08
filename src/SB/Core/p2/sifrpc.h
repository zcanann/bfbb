#ifndef PS2_SIFRPC_H
#define PS2_SIFRPC_H

// The subset of the Sony SIF RPC interface (sifrpc.h) used by the PS2
// platform layer.

#ifdef __cplusplus
extern "C" {
#endif

void sceSifInitRpc(unsigned int mode);
int sceSifLoadModule(const char* filename, int args, const char* argp);

#ifdef __cplusplus
}
#endif

#endif
