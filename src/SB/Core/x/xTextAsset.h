#ifndef XTEXTASSET_H
#define XTEXTASSET_H

#include <types.h>

struct xTextAsset
{
    U32 len;
};

#define xTextAssetGetText(t) ((char*)((xTextAsset*)(t) + 1))

#endif
