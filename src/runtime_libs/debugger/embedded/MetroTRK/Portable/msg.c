#include "PowerPC_EABI_Support/MetroTRK/trk.h"

UARTError WriteUART1(s8 arg0);
UARTError WriteUARTFlush(void);

/*
 * --INFO--
 * Address:	........
 * Size:	00002C
 */
/*
void TRKMessageAdd(void)
{
    // UNUSED FUNCTION
}
*/
/*
 * --INFO--
 * Address:	........
 * Size:	000008
 */
/*
void TRKMessageGet(void)
{
    // UNUSED FUNCTION
}
*/
/*
 * --INFO--
 * Address:	8021C4A4
 * Size:	0001DC
 */
DSError TRKMessageSend(TRKBuffer* msg)
{
	s32 checksum;
	u8 ch;
	u8 checksumByte;
	s32 result;
	s32 i;

	checksum = 0;
	for (i = 0; i < msg->length; i++) {
		checksum += msg->data[i];
	}
	checksum ^= 0xFF;

	result = WriteUART1(0x7E);
	if (result == DS_NoError) {
		for (i = 0; i < msg->length; i++) {
			ch = msg->data[i];
			if (ch == 0x7E || ch == 0x7D) {
				result = WriteUART1(0x7D);
				ch ^= 0x20;
				if (result != DS_NoError) {
					break;
				}
			}
			result = WriteUART1(ch);
			if (result != DS_NoError) {
				break;
			}
		}
	}

	if (result == DS_NoError) {
		checksumByte = checksum;
		for (i = 0; i < 1; i++) {
			if (checksumByte == 0x7E || checksumByte == 0x7D) {
				result = WriteUART1(0x7D);
				checksumByte ^= 0x20;
				if (result != DS_NoError) {
					break;
				}
			}
			result = WriteUART1(checksumByte);
			if (result != DS_NoError) {
				break;
			}
		}
	}

	if (result == DS_NoError) {
		result = WriteUART1(0x7E);
	}

	if (result == DS_NoError) {
		result = WriteUARTFlush();
	}

	return result;
}
