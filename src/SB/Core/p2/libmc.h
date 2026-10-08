#ifndef PS2_LIBMC_H
#define PS2_LIBMC_H

// The subset of the Sony memory card library interface (libmc.h) used by the
// PS2 platform layer.

#define sceMcFuncNoCardInfo 1
#define sceMcFuncNoOpen 2
#define sceMcFuncNoClose 3
#define sceMcFuncNoSeek 4
#define sceMcFuncNoRead 5
#define sceMcFuncNoWrite 6
#define sceMcFuncNoFlush 10
#define sceMcFuncNoMkdir 11
#define sceMcFuncNoChDir 12
#define sceMcFuncNoGetDir 13
#define sceMcFuncNoFileInfo 14
#define sceMcFuncNoDelete 15
#define sceMcFuncNoFormat 16
#define sceMcFuncNoUnformat 17
#define sceMcFuncNoEntSpace 18
#define sceMcFuncNoRename 19
#define sceMcFuncChgPrior 20

#define sceMcExecRun 0
#define sceMcExecFinish 1
#define sceMcExecIdle -1

#define sceMcIniSucceed 0
#define sceMcIniErrKernel -101
#define sceMcIniOldMcserv -120
#define sceMcIniOldMcman -121

#define sceMcTypeNoCard 0
#define sceMcTypePS1 1
#define sceMcTypePS2 2
#define sceMcTypePDA 3

#define sceMcFileAttrReadable 0x0001
#define sceMcFileAttrWriteable 0x0002
#define sceMcFileAttrExecutable 0x0004
#define sceMcFileAttrDupProhibit 0x0008
#define sceMcFileAttrFile 0x0010
#define sceMcFileAttrSubdir 0x0020
#define sceMcFileAttrClosed 0x0080
#define sceMcFileAttrPDAExec 0x0800
#define sceMcFileAttrPS1 0x1000

typedef struct
{
    unsigned char Resv2;
    unsigned char Sec;
    unsigned char Min;
    unsigned char Hour;
    unsigned char Day;
    unsigned char Month;
    unsigned short Year;
} sceMcStDateTime;

typedef struct
{
    sceMcStDateTime _Create;
    sceMcStDateTime _Modify;
    unsigned int FileSizeByte;
    unsigned short AttrFile;
    unsigned short Reserve1;
    unsigned int Reserve2;
    unsigned int PdaAplNo;
    unsigned char EntryName[32];
} sceMcTblGetDir __attribute__((aligned(64)));

typedef int iconIVECTOR[4];
typedef float iconFVECTOR[4];

typedef struct
{
    unsigned char Head[4];
    unsigned short Reserv1;
    unsigned short OffsLF;
    unsigned int Reserv2;
    unsigned int TransRate;
    iconIVECTOR BgColor[4];
    iconFVECTOR LightDir[3];
    iconFVECTOR LightColor[3];
    iconFVECTOR Ambient;
    unsigned char TitleName[68];
    unsigned char FnameView[64];
    unsigned char FnameCopy[64];
    unsigned char FnameDel[64];
    unsigned char Reserve3[512];
} sceMcIconSys;

#ifdef __cplusplus
extern "C" {
#endif

int sceMcInit(void);
int sceMcOpen(int port, int slot, const char* name, int mode);
int sceMcMkdir(int port, int slot, const char* name);
int sceMcClose(int fd);
int sceMcRead(int fd, void* buff, int size);
int sceMcWrite(int fd, const void* buff, int size);
int sceMcSync(int mode, int* cmd, int* result);
int sceMcGetInfo(int port, int slot, int* type, int* free, int* format);
int sceMcGetDir(int port, int slot, const char* name, unsigned int mode, int maxent,
                sceMcTblGetDir* table);
int sceMcChdir(int port, int slot, const char* newDir, char* curDir);
int sceMcFormat(int port, int slot);
int sceMcUnformat(int port, int slot);
int sceMcGetEntSpace(int port, int slot, const char* path);
int sceMcGetSlotMax(int port);

#ifdef __cplusplus
}
#endif

#endif
