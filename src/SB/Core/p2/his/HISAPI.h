#ifndef PS2_HISAPI_H
#define PS2_HISAPI_H

// Host I/O server (IOP streaming and sound) client interface; signatures from
// the PS2 DWARF of SB/Core/p2/his/HISAPI.cpp.

enum HISMediaType
{
    HIS_MEDIA_CDROM = 0,
    HIS_MEDIA_DVD = 1,
    HIS_MEDIA_HOSTIO = 2
};

enum HISMemoryType
{
    HIS_MEMORY_EE = 0,
    HIS_MEMORY_IOP = 1,
    HIS_MEMORY_SPU = 2
};

enum HISStatus
{
    HIS_STATUS_INVALID_ID = 0,
    HIS_STATUS_DONE = 1,
    HIS_STATUS_IN_PROGRESS = 2,
    HIS_STATUS_IN_QUEUE = 3,
    HIS_STATUS_PARTIAL = 4,
    HIS_STATUS_FAILED = 5,
    HIS_STATUS_CANCELLED = 6,
    HIS_STATUS_DMA_WAIT = 7
};

void HISInit(HISMediaType mediaType);
signed int HISGetVersion();
signed int HISGetFileIndex(char* filename);
signed int HISGetFileSize(signed int fileIndex);
signed int HISLoadBlockAsync(signed int fileIndex, signed int sourceBlock, signed int sourceSize,
                             void* destinationAddress, HISMemoryType destinationType,
                             signed int priority, signed int flags);
HISStatus HISLoadBlock(signed int fileIndex, signed int sourceBlock, signed int sourceSize,
                       void* destinationAddress, HISMemoryType destinationType, signed int priority,
                       signed int flags);
HISStatus HISGetRequestStatus(signed int requestID);
unsigned char HISCancelRequest(signed int requestID);
unsigned char HISCloseRequest(signed int requestID);
void HISWaitForRequest();
void HISFlushHostIOHandles();

void HISGetVoiceStatus(unsigned int* data);
void HISSetMasterVolume(signed int leftVolume, signed int rightVolume);
signed int HISGetExternalStreamBuffer(signed int voice);
void HISLoadExternalStream(signed int voice, signed int buffer, void* address);
void HISPlaySoundAsync(signed int voice, signed int leftVolume, signed int rightVolume,
                       signed int pitch, unsigned int address, signed int attack,
                       signed int release, unsigned char paused);
void HISPlayStreamAsync(signed int voice, signed int leftVolume, signed int rightVolume,
                        signed int pitch, signed int fileIndex, signed int logicalSectorNumber,
                        signed int dataSize, signed int flags, signed int attack,
                        signed int release, signed int blockSize, signed int interleaveSectors);
void HISPlayExternalStreamAsync(signed int voice, signed int leftVolume, signed int rightVolume,
                                signed int pitch, signed int flags, signed int attack,
                                signed int release, signed int blockSize);
void HISSetVoiceVolumeAsync(signed int voice, signed int leftVolume, signed int rightVolume);
void HISSetVoicePitchAsync(signed int voice, signed int pitch);
void HISStopVoiceAsync(signed int voice);
void HISPauseVoiceAsync(signed int voice);
void HISResumeVoiceAsync(signed int voice);
void HISJoinStereoVoicesAsync(signed int voice1, signed int voice2);
void HISFlushAsyncRequests();
void HISFlushAsyncRequestsNoWait();

#endif
