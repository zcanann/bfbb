#ifndef PS2_CTYPE_H
#define PS2_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

int isprint(int character);
extern char _ctype_[];

#ifdef __cplusplus
}
#endif

// Slot zero represents EOF; the following 256 bytes classify character values.
#define isprint(character) ((_ctype_ + 1)[(int)(character)] & 0x97)

#endif
