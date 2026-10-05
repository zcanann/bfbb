#include <types.h>

#include "xEvent.h"
#include "xString.h"
#include "xstransvc.h"

#if defined(PS2)
#include "zSceneLookup.h"
#else
#include "zScene.h"
#endif

char zEventLogBuf[20][256];

void zEntEvent(char* to, U32 toEvent)
{
    U32 id = xStrHash(to);
    xBase* sendTo = zSceneFindObject(id);

    if (sendTo)
    {
#if defined(PS2)
        zEntEvent(NULL, 0, sendTo, toEvent, NULL, NULL, 0);
#else
        zEntEvent(sendTo, toEvent);
#endif
    }
}

void zEntEvent(U32 toID, U32 toEvent)
{
    xBase* sendTo = zSceneFindObject(toID);

    if (sendTo)
    {
#if defined(PS2)
        zEntEvent(NULL, 0, sendTo, toEvent, NULL, NULL, 0);
#else
        zEntEvent(sendTo, toEvent);
#endif
    }
}

void zEntEvent(U32 toID, U32 toEvent, F32 toParam0, F32 toParam1, F32 toParam2,
               F32 toParam3)
{
    xBase* sendTo;
    F32 toParam[4];

    toParam[0] = toParam0;
    toParam[1] = toParam1;
    toParam[2] = toParam2;
    toParam[3] = toParam3;

    sendTo = zSceneFindObject(toID);

    if (sendTo)
    {
#if defined(PS2)
        zEntEvent(NULL, 0, sendTo, toEvent, toParam, NULL, 0);
#else
        zEntEvent(sendTo, toEvent, toParam);
#endif
    }
}

void zEntEvent(xBase* to, U32 toEvent)
{
    zEntEvent(NULL, 0, to, toEvent, NULL, NULL, 0);
}

void zEntEvent(xBase* to, U32 toEvent, F32 toParam0, F32 toParam1, F32 toParam2,
               F32 toParam3)
{
    F32 toParam[4];

    toParam[0] = toParam0;
    toParam[1] = toParam1;
    toParam[2] = toParam2;
    toParam[3] = toParam3;

#if defined(PS2)
    zEntEvent(NULL, 0, to, toEvent, toParam, NULL, 0);
#else
    zEntEvent(to, toEvent, toParam, NULL);
#endif
}

void zEntEvent(xBase* to, U32 toEvent, const F32* toParam)
{
    zEntEvent(NULL, 0, to, toEvent, toParam, NULL, 0);
}

void zEntEvent(xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget)
{
    zEntEvent(NULL, 0, to, toEvent, toParam, toParamWidget, 0);
}

void zEntEvent(xBase* from, xBase* to, U32 toEvent)
{
    zEntEvent(from, 0, to, toEvent, NULL, NULL, 0);
}

void zEntEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam)
{
    zEntEvent(from, 0, to, toEvent, toParam, NULL, 0);
}

void zEntEvent(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32* toParam,
               xBase* toParamWidget, S32 forceEvent)
{
    if (toEvent == eEventDisable)
    {
        xBaseDisable(to);
    }
    else if (toEvent == eEventEnable)
    {
        xBaseEnable(to);
    }

    if (to->eventFunc && (xBaseIsEnabled(to) || forceEvent))
    {
        to->eventFunc(from, to, toEvent, toParam, toParamWidget);
    }

    if (xBaseIsEnabled(to) && to->linkCount > 0)
    {
        xLinkAsset* idx = to->link;

        for (S32 i = 0; i < to->linkCount; i++, idx++)
        {
            if (toEvent == idx->srcEvent)
            {
                if (!idx->chkAssetID || (from && idx->chkAssetID == from->id))
                {
                    xBase* sendTo = zSceneFindObject(idx->dstAssetID);

                    if (sendTo)
                    {
                        xBase* b = NULL;

                        if (idx->paramWidgetAssetID)
                        {
                            b = zSceneFindObject(idx->paramWidgetAssetID);

                            if (!b)
                            {
                                b = (xBase*)xSTFindAsset(idx->paramWidgetAssetID, NULL);
                            }
                        }

                        zEntEvent(to, toEvent, sendTo, idx->dstEvent, idx->param, b, 0);
                    }
                }
            }
        }
    }
}
