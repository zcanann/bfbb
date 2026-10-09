#include "iFile.h"

#include "xDebug.h"
#include "xFile.h"

#include "iSystem.h"
#include "iTime.h"

#include "his/HISAPI.h"

#include <libcdvd.h>
#include <rwplcore.h>
#include <sifdev.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

extern "C" {
char* strupr(char* string);
void abort(void);
}

// Host I/O server debug views.
struct HISFileIndexDebug
{
    S32 logicalSectorNumber;
    S32 size;
    S32 sizeInSectors;
    char name[48];
};

struct HISRequestDebug
{
    struct
    {
        S32 nextRequest : 8;
        HISStatus status : 8;
        HISMemoryType destinationType : 8;
    } small;
    S32 destination;
    S32 currentPriority;
    S32 originalPriority;
    S32 fileIndex;
    S32 startSector;
    S32 sectorsToRead;
};

S32 HISGetFirstValidRequest();
void HISGetRequestDebug(S32 requestID, HISRequestDebug* requestDebug);
void HISGetFileIndexDebug(S32 fileIndex, HISFileIndexDebug* fileIndexDebug);
void HISInitStubs();

struct AsyncRequest
{
    U8 inUse;
    S32 id;
    IFILE_READSECTOR_STATUS status;
    void (*callback)(tag_xFile*);
    tag_xFile* file;
    S32 framesLeft;
};

// tag_iFile::flags
#define IFILE_FLAG_OPEN 0x1
#define IFILE_FLAG_HIS 0x2

static AsyncRequest requests[32];
static S32 num_open_files;
char gHostPath[256];

extern S32 DVD;

namespace
{
void iFileDebugMode()
{
    static char* STATUSES[] = { "INVALID", "DONE",    "IN_PROG", "IN_QUEU", "PARTIAL",
                                "FAILED",  "CANCELL", "CANCEL_", "DMA_WAI" };
    static char* TYPES[] = { "EE ", "IOP", "SPU" };

    S32 id;
    HISRequestDebug requestDebug;
    HISFileIndexDebug fileIndexDebug;

    for (id = HISGetFirstValidRequest(); id != -1; id = requestDebug.small.nextRequest)
    {
        HISGetRequestDebug(id, &requestDebug);
        if (requestDebug.small.status != HIS_STATUS_INVALID_ID)
        {
            xprintf("r%02d: ST=%-8s D=%08X (%s) P=%-5d/%-5d\n", id,
                    STATUSES[requestDebug.small.status], requestDebug.destination,
                    TYPES[requestDebug.small.destinationType], requestDebug.currentPriority,
                    requestDebug.originalPriority);
            xprintf(".    F=%-4d SEC=%-4d TOR=%-4d\n", requestDebug.fileIndex,
                    requestDebug.sectorsToRead, requestDebug.sectorsToRead);

            HISGetFileIndexDebug(requestDebug.fileIndex, &fileIndexDebug);
            xprintf(".    LSN=%-4d SIZE=%-7d (%-4d) %s\n", fileIndexDebug.logicalSectorNumber,
                    fileIndexDebug.size, fileIndexDebug.sizeInSectors, fileIndexDebug.name);
        }
    }
}
} // namespace

void iFileInit()
{
    S32 hisVersion;

    iLoadModule("libsd.irx", NULL);
    iLoadModule("his.irx", NULL);
    HISInitStubs();

    hisVersion = HISGetVersion();
    if (hisVersion < 0x10510)
    {
        printf("**** FATAL **** Your HIS.IRX is not the latest version.\n");
        abort();
    }
    else if (hisVersion > 0x10510)
    {
        printf("**** FATAL **** Your HIS.IRX is newer then this version.\n");
        abort();
    }

    HISInit(DVD == 0 ? HIS_MEDIA_CDROM : DVD == 1 ? HIS_MEDIA_DVD : HIS_MEDIA_HOSTIO);

    memset(requests, 0, sizeof(requests));
    xDebugModeAdd("DM_FILE", iFileDebugMode);
}

void iFileExit()
{
    sceCdInit(SCECdEXIT);
}

U32* iFileLoad(char* name, U32* buffer, U32* size)
{
    S32 index;
    S32 fileSize;
    S32 alignedSize;

    index = HISGetFileIndex(name);
    if (index == -1)
    {
        printf("----> iFileLoad: Cannot find %s\n", name);
        return NULL;
    }

    fileSize = HISGetFileSize(index);
    alignedSize = (fileSize + 15) / 16 * 16;

    if (buffer == NULL)
    {
        buffer = (U32*)RwMalloc(alignedSize);
    }

    HISLoadBlock(index, 0, (alignedSize + 0x7FF) / 2048, buffer, HIS_MEMORY_EE, 0x100,
                 (alignedSize % 2048) << 8);

    if (size != NULL)
    {
        *size = fileSize;
    }

    return buffer;
}

U32 iFileOpen(const char* name, S32 flags, tag_xFile* file)
{
    tag_iFile* ps = &file->ps;

    if (flags & 0x40)
    {
        // A file packed into the disc archive, read through the host I/O server.
        ps->fd = HISGetFileIndex((char*)name);
        if (ps->fd == -1)
        {
            return 1;
        }

        ps->offset = 0;
        ps->length = HISGetFileSize(ps->fd);
        ps->flags = IFILE_FLAG_HIS;
    }
    else
    {
        if (flags & IFILE_OPEN_ABSPATH)
        {
            strcpy(ps->path, name);
            flags &= ~IFILE_OPEN_ABSPATH;
        }
        else
        {
            iFileFullPath(name, ps->path);
        }

        while (true)
        {
            ps->fd = sceOpen(ps->path, flags);
            if (ps->fd >= 0)
            {
                break;
            }

            printf("----> iFileOpen fails (err=%d)\n", ps->fd);
            if (ps->fd == -16)
            {
                printf("iFileOpen: (wait=%d)\n", 1000);
            }
            else
            {
                return 1;
            }
        }

        ps->flags = IFILE_FLAG_OPEN;
    }

    num_open_files++;
    return 0;
}

S32 iFileSeek(tag_xFile* file, S32 offset, S32 whence)
{
    tag_iFile* ps = &file->ps;
    S32 position;
    S32 new_pos;

    if (ps->flags & IFILE_FLAG_HIS)
    {
        new_pos = ps->offset;
        switch (whence)
        {
        case IFILE_SEEK_SET:
            new_pos = offset;
            break;
        case IFILE_SEEK_CUR:
            new_pos += offset;
            break;
        case IFILE_SEEK_END:
            new_pos = ps->length - offset;
            break;
        }

        if (new_pos > ps->length)
        {
            new_pos = ps->length;
        }

        ps->offset = new_pos;
        return new_pos;
    }

    switch (whence)
    {
    case IFILE_SEEK_SET:
        whence = SCE_SEEK_SET;
        break;
    case IFILE_SEEK_CUR:
        whence = SCE_SEEK_CUR;
        break;
    case IFILE_SEEK_END:
        whence = SCE_SEEK_END;
        break;
    }

    position = sceLseek(ps->fd, offset, whence);
    if (position < 0)
    {
        printf("----> iFileSeek: sceLseek fails (err=%d)\n", position);
        return -1;
    }

    return position;
}

U32 iFileRead(tag_xFile* file, void* buf, U32 size)
{
    tag_iFile* ps = &file->ps;
    S32 num;

    iTimeGet();

    if (ps->flags & IFILE_FLAG_HIS)
    {
        HISLoadBlock(ps->fd, ps->offset / 2048, size / 2048, buf, HIS_MEMORY_EE, 0x100, 0);
        ps->offset += size;
        num = size;
    }
    else
    {
        num = sceRead(ps->fd, buf, size);
    }

    if (num < 0)
    {
        printf("----> iFileRead fails (err=%d)\n", num);
        return 0;
    }

    return num;
}

static inline bool iFileRequestFinished(HISStatus status)
{
    return status == HIS_STATUS_DONE || status == HIS_STATUS_FAILED ||
           status == HIS_STATUS_CANCELLED;
}

U32 iFileClose(tag_xFile* file)
{
    tag_iFile* ps = &file->ps;
    S32 ret;
    S32 i;

    for (i = 0; i < 32; i++)
    {
        if (requests[i].inUse && requests[i].file == file && requests[i].id != -1)
        {
            HISCancelRequest(requests[i].id);
            while (!iFileRequestFinished(HISGetRequestStatus(requests[i].id)))
            {
                HISWaitForRequest();
            }

            HISCloseRequest(requests[i].id);
            memset(&requests[i], 0, sizeof(AsyncRequest));
        }
    }

    if (!(ps->flags & IFILE_FLAG_HIS))
    {
        ret = sceClose(ps->fd);
        if (ret != 0)
        {
            printf("----> iFileClose (fd=%d) fails (err=%d)\n", ps->fd, ret);
            return 1;
        }
    }

    ps->flags = 0;
    num_open_files--;
    return 0;
}

U32 iFileGetSize(tag_xFile* file)
{
    S32 size;
    S32 pos;
    tag_iFile* ps = &file->ps;
    S32 rc;

    if (ps->flags & IFILE_FLAG_HIS)
    {
        return HISGetFileSize(ps->fd);
    }

    if (ps->fd < 0)
    {
        return 0;
    }

    pos = sceLseek(ps->fd, 0, SCE_SEEK_CUR);
    if (pos < 0)
    {
        printf("----> iFileGetSize: sceLseek fails (err=%d)\n", pos);
        return 0;
    }

    size = sceLseek(ps->fd, 0, SCE_SEEK_END);
    if (size < 0)
    {
        printf("----> iFileGetSize: sceLseek(2) fails (err=%d)\n", size);
        return 0;
    }

    rc = sceLseek(ps->fd, pos, SCE_SEEK_SET);
    if (rc < 0)
    {
        printf("----> iFileGetSize: sceLseek(3) fails (err=%d)\n", rc);
        return 0;
    }

    return size;
}

void iFileFullPath(const char* relname, char* fullname)
{
    strupr((char*)relname);

    if (DVD != 2)
    {
        char temp[256] = {};

        strcpy(temp, relname);
        strupr(temp);
        sprintf(fullname, "cdrom0:\\%s;1", temp);
    }
    else
    {
        sprintf(fullname, "host:%s%s", gHostPath, relname);
    }
}

void iFileSetPath(const char* path)
{
    U32 len = strlen(path);

    printf("iFileSetPath:  \"%s\"\n", path);
    strcpy(gHostPath, path);

    if (len != 0 && gHostPath[len - 1] != '/' && gHostPath[len - 1] != '\\')
    {
        gHostPath[len] = '/';
        gHostPath[len + 1] = '\0';
    }
}

S32 iFileReadAsync(tag_xFile* file, void* buf, U32 aSize, void (*callback)(tag_xFile*),
                   S32 priority)
{
    tag_iFile* ps = &file->ps;
    S32 i;
    S32 id;

    for (i = 0; i < 32; i++)
    {
        if (!requests[i].inUse)
        {
            if (ps->flags & IFILE_FLAG_HIS)
            {
                id = HISLoadBlockAsync(file->ps.fd, ps->offset / 2048, aSize / 2048, buf,
                                       HIS_MEMORY_EE, priority, 0);
                if (id == -1)
                {
                    return -1;
                }

                requests[i].inUse = 1;
                requests[i].framesLeft = 0;
                requests[i].callback = callback;
                requests[i].id = id;
                requests[i].file = file;
                requests[i].status = IFILE_RDSTAT_QUEUED;
                ps->offset += aSize;
            }
            else
            {
                iFileRead(file, buf, aSize);

                requests[i].inUse = 1;
                requests[i].framesLeft = 8;
                requests[i].callback = callback;
                requests[i].id = -1;
                requests[i].file = file;
                requests[i].status = IFILE_RDSTAT_DONE;
                callback(file);
            }

            return i;
        }
    }

    return -1;
}

void iFileReadStop()
{
    S32 i;

    for (i = 0; i < 32; i++)
    {
        if (requests[i].inUse && requests[i].id != -1)
        {
            HISCancelRequest(requests[i].id);
            HISCloseRequest(requests[i].id);
            requests[i].id = -1;
            requests[i].status = IFILE_RDSTAT_FAIL;
            requests[i].framesLeft = 8;
        }
    }
}

IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* amtToFar)
{
    if (!requests[key].inUse)
    {
        return IFILE_RDSTAT_NOOP;
    }

    if (amtToFar != NULL)
    {
        *amtToFar = 0;
    }

    return requests[key].status;
}

void iFileAsyncService()
{
    S32 i;
    HISStatus status;

    for (i = 0; i < 32; i++)
    {
        if (!requests[i].inUse)
        {
            continue;
        }

        // Finished requests linger a few frames so their status can be polled.
        if (requests[i].framesLeft > 0)
        {
            if (--requests[i].framesLeft == 0)
            {
                requests[i].inUse = 0;
            }
            continue;
        }

        status = HISGetRequestStatus(requests[i].id);
        switch (status)
        {
        case HIS_STATUS_DONE:
            requests[i].status = IFILE_RDSTAT_DONE;
            if (requests[i].callback != NULL)
            {
                requests[i].callback(requests[i].file);
            }
            HISCloseRequest(requests[i].id);
            requests[i].framesLeft = 8;
            requests[i].id = -1;
            break;
        case HIS_STATUS_FAILED:
        case HIS_STATUS_CANCELLED:
            requests[i].status = IFILE_RDSTAT_FAIL;
            HISCloseRequest(requests[i].id);
            requests[i].framesLeft = 8;
            requests[i].id = -1;
            break;
        case HIS_STATUS_IN_PROGRESS:
        case HIS_STATUS_PARTIAL:
        case HIS_STATUS_DMA_WAIT:
            requests[i].status = IFILE_RDSTAT_INPROG;
            break;
        case HIS_STATUS_IN_QUEUE:
            requests[i].status = IFILE_RDSTAT_QUEUED;
            break;
        }
    }
}
