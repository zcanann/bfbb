#include "iCutscene.h"

#include "xCutscene.h"
#include "xFile.h"
#include "xMath.h"
#include "xpkrsvc.h"
#include "xSnd.h"
#include "xstransvc.h"

#include "iFile.h"
#include "iModel.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rwplcore.h>
#include <string.h>

extern xCutscene sActiveCutscene;

void xSndLoadExternalData(U32 snd, void* data, S32 forceBuffer);
U32 xSndIsReady(U32 id);

static void iCSSoundSetup(xCutscene* csn)
{
    U32 dataIndex;
    U32 numData;

    xCutsceneData* data = (xCutsceneData*)(csn->Play + 1);
    numData = csn->Play->NumData;

    for (dataIndex = 0; dataIndex < numData; dataIndex++)
    {
        if (data->DataType == XCUTSCENEDATA_TYPE_SOUND)
        {
            if (csn->SndNumChannel >= 2)
            {
                break;
            }

            csn->SndAssetID[csn->SndNumChannel] = data->AssetID;
            csn->SndNumChannel++;
        }

        data = (xCutsceneData*)((U8*)data + ALIGN(data->ChunkSize, 16) + sizeof(xCutsceneData));
    }
}

// Cutscene audio is streamed from the cutscene file itself: each time chunk
// carries the next block of sound data for each channel.
static void* iCSSoundGetData(xCutscene* csn, U32 channel, U32 chunk)
{
    void* retdata = NULL;
    xCutsceneData* data;
    U32 dataIndex;
    U32 numData;
    U32 channelIndex;

    if (!csn->Waiting && csn->Stream->ChunkIndex == chunk)
    {
        numData = csn->Stream->NumData;
        data = (xCutsceneData*)(csn->Stream + 1);
    }
    else
    {
        numData = csn->Play->NumData;
        data = (xCutsceneData*)(csn->Play + 1);
    }

    for (dataIndex = 0, channelIndex = 0; dataIndex < numData; dataIndex++)
    {
        if (data->DataType == XCUTSCENEDATA_TYPE_SOUND)
        {
            if (retdata == NULL)
            {
                retdata = data + 1;
            }

            if (channel == channelIndex)
            {
                retdata = data + 1;
                break;
            }

            channelIndex++;
        }

        data = (xCutsceneData*)((U8*)data + ALIGN(data->ChunkSize, 16) + sizeof(xCutsceneData));
    }

    if (retdata == NULL)
    {
        return NULL;
    }

    // The sound data is 64-byte aligned within its chunk.
    while ((U32)retdata & 0x3F)
    {
        retdata = (U8*)retdata + 16;
    }

    return retdata;
}

static void iCSSoundCutsceneCB(U32 id, bool first)
{
    void* data = iCSSoundGetData(&sActiveCutscene, id == sActiveCutscene.SndHandle[1],
                                 sActiveCutscene.SndChannelReq[id == sActiveCutscene.SndHandle[1]]);

    if (first)
    {
        xSndLoadExternalData(id, data, 0);
        xSndLoadExternalData(id, (void*)((U32)data + 0x6B80), 1);
    }
    else
    {
        xSndLoadExternalData(id, data, -1);
    }
}

static void iCSAsyncReadCB(tag_xFile* file)
{
    sActiveCutscene.Waiting = 0;
    sActiveCutscene.AsyncID = -1;
}

U32 iCSFileOpen(xCutscene* csn)
{
    U32 headerskip;
    st_PKR_ASSET_TOCINFO ainfo;

    headerskip = ALIGN(csn->Info->HeaderSize, 2048);

    if (!xSTGetAssetInfo(csn->Info->AssetID, &ainfo))
    {
        return 0;
    }

    char* filename = xST_xAssetID_HIPFullPath(csn->Info->AssetID);

    if (iFileOpen(filename, IFILE_OPEN_READ | 0x40, &csn->File) == 0)
    {
        iFileSeek(&csn->File, headerskip + (ainfo.sector << 11), IFILE_SEEK_SET);
        csn->AsyncID = -1;
        return 1;
    }

    return 0;
}

void iCSFileAsyncRead(xCutscene* csn, void* dest, U32 size)
{
    U32* tp = (U32*)dest;
    U32 i;

    csn->Waiting = 1;

    for (i = 0; i < size / 4; i++)
    {
        tp[i] = 0xDEADBEEF;
    }

    csn->AsyncID = iFileReadAsync(&csn->File, dest, size, iCSAsyncReadCB, 0x10000);
}

void iCSFileClose(xCutscene* csn)
{
    iFileReadStop();
    csn->ShutDownWait = 1;
    iFileClose(&csn->File);
    csn->Opened = 0;
}

static RpAtomic* FastPipeAtomicCB(RpAtomic* atomic, void*)
{
    return atomic;
}

S32 iCSLoadStep(xCutscene* csn)
{
    S32 bytes;
    XFILE_READSECTOR_STATUS cdstat;
    U32 skipAccum;
    U32 tmpSize;
    void* foundModel;
    U32 i;

    if (csn->Waiting)
    {
        cdstat = xFileReadAsyncStatus(csn->AsyncID, &bytes);

        if (cdstat == XFILE_RDSTAT_INPROG || cdstat == XFILE_RDSTAT_QUEUED)
        {
            return 0;
        }

        iCSAsyncReadCB(&csn->File);
    }

    if (csn->DataLoading >= 0)
    {
        skipAccum = 0;

        while (csn->DataLoading < (S32)csn->Info->NumData)
        {
            if (csn->Data[csn->DataLoading].DataType == XCUTSCENEDATA_TYPE_JDELTAMODEL)
            {
                foundModel = NULL;
            }
            else
            {
                foundModel = xSTFindAsset(csn->Data[csn->DataLoading].AssetID, &tmpSize);
            }

            if (foundModel || csn->Data[csn->DataLoading].ChunkSize == 0)
            {
                csn->Data[csn->DataLoading].DataPtr = foundModel;
                skipAccum += ALIGN(csn->Data[csn->DataLoading].ChunkSize, 2048);
                csn->DataLoading++;
            }
            else
            {
                if (skipAccum)
                {
                    iFileSeek(&csn->File, skipAccum, IFILE_SEEK_CUR);
                    skipAccum = 0;
                }

                if (csn->GotData)
                {
                    if (csn->Data[csn->DataLoading].DataType == XCUTSCENEDATA_TYPE_JDELTAMODEL)
                    {
                        csn->Data[csn->DataLoading].DataPtr =
                            RwMalloc(csn->Data[csn->DataLoading].ChunkSize);

                        memcpy(csn->Data[csn->DataLoading].DataPtr, csn->AlignBuf,
                               csn->Data[csn->DataLoading].ChunkSize);
                    }
                    else
                    {
                        csn->Data[csn->DataLoading].DataPtr =
                            iModelFileNew(csn->AlignBuf, csn->Data[csn->DataLoading].ChunkSize);

                        RpClumpForAllAtomics(
                            ((RpAtomic*)csn->Data[csn->DataLoading].DataPtr)->clump,
                            FastPipeAtomicCB, NULL);
                    }

                    csn->Data[csn->DataLoading].DataType |= 0x80000000;
                    csn->DataLoading++;
                    csn->GotData = 0;
                    csn->AsyncID = -1;
                }
                else
                {
                    iCSFileAsyncRead(csn, csn->AlignBuf,
                                     ALIGN(csn->Data[csn->DataLoading].ChunkSize, 2048));
                    csn->GotData = 1;
                    return 0;
                }
            }
        }

        if (skipAccum)
        {
            iFileSeek(&csn->File, skipAccum, IFILE_SEEK_CUR);
        }

        csn->DataLoading = -1;
    }

    if (csn->DataLoading == -1)
    {
        if (csn->GotData)
        {
            iCSSoundSetup(csn);

            if (csn->SndNumChannel != 0)
            {
                xSndPauseAll(1, 1);
                xSndUpdate();
                xSndSetExternalCallback(iCSSoundCutsceneCB);

                if (csn->SndNumChannel == 2)
                {
                    // Stereo
                    csn->SndHandle[0] = xSndPlay(csn->SndAssetID[0], 0.9f, -99999.0f, 255, 0x500,
                                                 0, SND_CAT_CUTSCENE, 0.0f);
                    csn->SndHandle[1] = xSndPlay(csn->SndAssetID[1], 0.9f, -99999.0f, 255, 0x480,
                                                 0, SND_CAT_CUTSCENE, 0.0f);
                }
                else
                {
                    // Mono
                    csn->SndHandle[0] = xSndPlay(csn->SndAssetID[0], 0.9f, -99999.0f, 255, 0x400,
                                                 0, SND_CAT_CUTSCENE, 0.0f);
                    csn->SndHandle[1] = 0;
                }
            }

            csn->GotData = 0;
            csn->DataLoading = -2;
        }
        else
        {
            iCSFileAsyncRead(csn, csn->Play, csn->TimeChunkOffs[1] - csn->TimeChunkOffs[0]);
            csn->GotData = 1;
            return 0;
        }
    }

    if (csn->DataLoading == -2)
    {
        if (csn->Info->NumTime > 1)
        {
            iCSFileAsyncRead(csn, csn->Stream, csn->TimeChunkOffs[2] - csn->TimeChunkOffs[1]);
        }

        csn->DataLoading = -3;
    }

    for (i = 0; i < csn->SndNumChannel; i++)
    {
        if (!xSndIsReady(csn->SndHandle[i]))
        {
            return 0;
        }
    }

    csn->Ready = 1;
    return 1;
}
