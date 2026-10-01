#include <rwsdk/rwcore.h>

#include "rwsdk/driver/common/palquant.h"

#define RwFreeListAlloc(_f) (RWSRCGLOBAL(memoryAlloc)(_f))
#define RwFreeListFree(_f, _p) (RWSRCGLOBAL(memoryFree)((_f), (_p)))

struct _rwPalQuantRGBABox
{
    RwRGBA col0; /* inclusive */
    RwRGBA col1; /* exclusive */
};

typedef struct _rwPalQuantLeafNode _rwPalQuantLeafNode;
struct _rwPalQuantLeafNode
{
    RwReal weight;
    RwRGBAReal ac;
    RwReal var;
    RwUInt8 palIndex;
};

typedef struct _rwPalQuantBranchNode _rwPalQuantBranchNode;
struct _rwPalQuantBranchNode
{
    _rwPalQuantOctNode* dir[16];
};

union _rwPalQuantOctNode
{
    _rwPalQuantLeafNode Leaf;
    _rwPalQuantBranchNode Branch;
};

static RwUInt8 MaxDepth = 5;

static RwUInt32 splice[256];

static _rwPalQuantLeafNode* InitLeaf(_rwPalQuantLeafNode* Leaf)
{
    Leaf->palIndex = 0xFF;
    Leaf->weight = 0.0f;
    Leaf->ac.red = 0.0f;
    Leaf->ac.green = 0.0f;
    Leaf->ac.blue = 0.0f;
    Leaf->ac.alpha = 0.0f;
    Leaf->var = 0.0f;

    return Leaf;
}

static _rwPalQuantBranchNode* InitBranch(_rwPalQuantBranchNode* Branch)
{
    RwInt32 i;

    for (i = 0; i < 16; i++)
    {
        Branch->dir[i] = NULL;
    }

    return Branch;
}

static _rwPalQuantOctNode* CreateCube(RwFreeList* freelist)
{
    return (_rwPalQuantOctNode*)RwFreeListAlloc(freelist);
}

static _rwPalQuantOctNode* AllocateToLeaf(RwPalQuant* pq, _rwPalQuantOctNode* root, RwUInt32 Octs,
                                          RwInt32 depth)
{
    if (depth == 0)
    {
        return root;
    }

    /* Allocate the next level down if it does not exist yet */
    if (!root->Branch.dir[Octs & 15])
    {
        _rwPalQuantOctNode* node = CreateCube(pq->cubefreelist);

        root->Branch.dir[Octs & 15] = node;

        if (depth == 1)
        {
            InitLeaf(&node->Leaf);
        }
        else
        {
            InitBranch(&node->Branch);
        }
    }

    return AllocateToLeaf(pq, root->Branch.dir[Octs & 15], Octs >> 4, depth - 1);
}

static RwUInt32 GetOctAdr(RwRGBA* c)
{
    RwInt32 ColShift = 8 - MaxDepth;

    /* Interleave the bits of the four components */
    return (splice[c->red >> ColShift] << 3) | (splice[c->green >> ColShift] << 2) |
           (splice[c->blue >> ColShift] << 1) | splice[c->alpha >> ColShift];
}

static void LeafAddPixel(_rwPalQuantLeafNode* leaf, RwRGBA* color, RwReal weight)
{
    RwRGBAReal rColor;

    rColor.red = ((RwReal)(1.0 / 255.0)) * (RwReal)color->red;
    rColor.green = ((RwReal)(1.0 / 255.0)) * (RwReal)color->green;
    rColor.blue = ((RwReal)(1.0 / 255.0)) * (RwReal)color->blue;
    rColor.alpha = ((RwReal)(1.0 / 255.0)) * (RwReal)color->alpha;

    rColor.red = rColor.red * weight;
    rColor.green = rColor.green * weight;
    rColor.blue = rColor.blue * weight;
    rColor.alpha = rColor.alpha * weight;

    leaf->weight += weight;
    leaf->ac.red += rColor.red;
    leaf->ac.green += rColor.green;
    leaf->ac.blue += rColor.blue;
    leaf->ac.alpha += rColor.alpha;
    leaf->var += weight * (rColor.red * rColor.red + rColor.green * rColor.green +
                           rColor.blue * rColor.blue + rColor.alpha * rColor.alpha);
}

void _rwPalQuantAddImage(RwPalQuant* pq, RwImage* img, RwReal weight)
{
    RwInt32 width;
    RwInt32 height;
    RwInt32 stride;
    RwUInt8* pixels;
    RwRGBA* palette;
    RwUInt8* linePixels;

    stride = img->stride;
    pixels = img->cpPixels;
    palette = img->palette;
    height = img->height;

    switch (img->depth)
    {
    case 4:
    case 8:
    {
        while (height--)
        {
            width = img->width;
            linePixels = pixels;

            while (width--)
            {
                RwRGBA* color = &palette[*linePixels];
                _rwPalQuantOctNode* leaf;

                leaf = AllocateToLeaf(pq, pq->root, GetOctAdr(color), MaxDepth);
                LeafAddPixel(&leaf->Leaf, color, weight);

                linePixels++;
            }

            pixels += stride;
        }
        break;
    }
    case 32:
    {
        while (height--)
        {
            RwRGBA* color = (RwRGBA*)pixels;

            width = img->width;

            while (width--)
            {
                _rwPalQuantOctNode* leaf;

                leaf = AllocateToLeaf(pq, pq->root, GetOctAdr(color), MaxDepth);
                LeafAddPixel(&leaf->Leaf, color, weight);

                color++;
            }

            pixels += stride;
        }
        break;
    }
    }
}

static void assignindex(_rwPalQuantOctNode* root, RwRGBA* origin, RwInt32 depth,
                        _rwPalQuantRGBABox* region, RwInt32 palIndex)
{
    if (root)
    {
        RwInt32 width = 1 << depth;
        RwInt32 dR = origin->red - region->col1.red;
        RwInt32 dG = origin->green - region->col1.green;
        RwInt32 dB = origin->blue - region->col1.blue;
        RwInt32 dA = origin->alpha - region->col1.alpha;
        RwInt32 uR;
        RwInt32 uG;
        RwInt32 uB;
        RwInt32 uA;

        /* Does this node overlap the region at all? */
        if ((dR >= 0) || (dG >= 0) || (dB >= 0) || (dA >= 0))
        {
            return;
        }

        uR = region->col0.red - origin->red;
        uG = region->col0.green - origin->green;
        uB = region->col0.blue - origin->blue;
        uA = region->col0.alpha - origin->alpha;

        if ((uR >= width) || (uG >= width) || (uB >= width) || (uA >= width))
        {
            return;
        }

        if ((dR <= -width) && (dG <= -width) && (dB <= -width) && (dA <= -width) && (uR <= 0) &&
            (uG <= 0) && (uB <= 0) && (uA <= 0) && (depth == 0))
        {
            /* Leaf fully inside the region */
            root->Leaf.palIndex = (RwUInt8)palIndex;
        }
        else
        {
            RwInt32 i;
            RwRGBA suborigin;

            for (i = 0; i < 16; i++)
            {
                suborigin.red = origin->red + (((i >> 3) & 1) << (depth - 1));
                suborigin.green = origin->green + (((i >> 2) & 1) << (depth - 1));
                suborigin.blue = origin->blue + (((i >> 1) & 1) << (depth - 1));
                suborigin.alpha = origin->alpha + ((i & 1) << (depth - 1));

                assignindex(root->Branch.dir[i], &suborigin, depth - 1, region, palIndex);
            }
        }
    }
}

static void addvolume(_rwPalQuantOctNode* root, RwRGBA* origin, RwInt32 depth,
                      _rwPalQuantRGBABox* region, _rwPalQuantLeafNode* volume)
{
    if (root)
    {
        RwInt32 width = 1 << depth;
        RwInt32 dR = origin->red - region->col1.red;
        RwInt32 dG = origin->green - region->col1.green;
        RwInt32 dB = origin->blue - region->col1.blue;
        RwInt32 dA = origin->alpha - region->col1.alpha;
        RwInt32 uR;
        RwInt32 uG;
        RwInt32 uB;
        RwInt32 uA;

        /* Does this node overlap the region at all? */
        if ((dR >= 0) || (dG >= 0) || (dB >= 0) || (dA >= 0))
        {
            return;
        }

        uR = region->col0.red - origin->red;
        uG = region->col0.green - origin->green;
        uB = region->col0.blue - origin->blue;
        uA = region->col0.alpha - origin->alpha;

        if ((uR >= width) || (uG >= width) || (uB >= width) || (uA >= width))
        {
            return;
        }

        if ((dR <= -width) && (dG <= -width) && (dB <= -width) && (dA <= -width) && (uR <= 0) &&
            (uG <= 0) && (uB <= 0) && (uA <= 0) && (depth == 0))
        {
            /* Leaf fully inside the region */
            volume->weight += root->Leaf.weight;
            volume->ac.red += root->Leaf.ac.red;
            volume->ac.green += root->Leaf.ac.green;
            volume->ac.blue += root->Leaf.ac.blue;
            volume->ac.alpha += root->Leaf.ac.alpha;
            volume->var += root->Leaf.var;
        }
        else
        {
            RwInt32 i;
            RwRGBA suborigin;

            for (i = 0; i < 16; i++)
            {
                suborigin.red = origin->red + (((i >> 3) & 1) << (depth - 1));
                suborigin.green = origin->green + (((i >> 2) & 1) << (depth - 1));
                suborigin.blue = origin->blue + (((i >> 1) & 1) << (depth - 1));
                suborigin.alpha = origin->alpha + ((i & 1) << (depth - 1));

                addvolume(root->Branch.dir[i], &suborigin, depth - 1, region, volume);
            }
        }
    }
}

static _rwPalQuantLeafNode* BoxStats(_rwPalQuantLeafNode* Vol, _rwPalQuantOctNode* root,
                                     _rwPalQuantRGBABox* cube)
{
    RwRGBA origin;

    origin.red = 0;
    origin.green = 0;
    origin.blue = 0;
    origin.alpha = 0;

    InitLeaf(Vol);
    addvolume(root, &origin, MaxDepth, cube, Vol);

    return Vol;
}

#define MAXIMIZECHANNEL(_chn)                                                                      \
    MACRO_START                                                                                    \
    {                                                                                              \
        for (i = cube->col0._chn; i < cube->col1._chn; i++)                                        \
        {                                                                                          \
            leftcube.col1._chn = (RwUInt8)i;                                                       \
            BoxStats(&left, root, &leftcube);                                                      \
                                                                                                   \
            right.ac.red = whole->ac.red - left.ac.red;                                            \
            right.ac.green = whole->ac.green - left.ac.green;                                      \
            right.ac.blue = whole->ac.blue - left.ac.blue;                                         \
            right.ac.alpha = whole->ac.alpha - left.ac.alpha;                                      \
            right.weight = whole->weight - left.weight;                                            \
                                                                                                   \
            if ((left.weight > 0.0f) && (right.weight > 0.0f))                                     \
            {                                                                                      \
                sum = (left.ac.red * left.ac.red + left.ac.green * left.ac.green +                 \
                       left.ac.blue * left.ac.blue + left.ac.alpha * left.ac.alpha) /              \
                      left.weight;                                                                 \
                sum += (right.ac.red * right.ac.red + right.ac.green * right.ac.green +            \
                        right.ac.blue * right.ac.blue + right.ac.alpha * right.ac.alpha) /         \
                       right.weight;                                                               \
                                                                                                   \
                if (sum > maxsum)                                                                  \
                {                                                                                  \
                    maxsum = sum;                                                                  \
                    *cut = i;                                                                      \
                }                                                                                  \
                else if (sum < lastsum)                                                            \
                {                                                                                  \
                    /* The variance is falling again so stop looking */                           \
                    break;                                                                         \
                }                                                                                  \
                                                                                                   \
                lastsum = sum;                                                                     \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

static RwReal nMaximize(_rwPalQuantOctNode* root, _rwPalQuantRGBABox* cube, RwInt32 chn,
                        RwInt32* cut, _rwPalQuantLeafNode* whole)
{
    RwReal maxsum = 0.0f;
    RwReal lastsum = maxsum;
    _rwPalQuantRGBABox leftcube;
    _rwPalQuantLeafNode left;
    _rwPalQuantLeafNode right;
    RwReal sum;
    RwInt32 i;

    *cut = -1;
    leftcube = *cube;

    switch (chn)
    {
    case 1:
    {
        MAXIMIZECHANNEL(red);
        break;
    }
    case 2:
    {
        MAXIMIZECHANNEL(green);
        break;
    }
    case 3:
    {
        MAXIMIZECHANNEL(blue);
        break;
    }
    case 4:
    {
        MAXIMIZECHANNEL(alpha);
        break;
    }
    }

    return maxsum;
}

static RwInt32 nCut(_rwPalQuantOctNode* root, _rwPalQuantRGBABox* set1, _rwPalQuantRGBABox* set2)
{
    RwInt32 cutr, cutg, cutb, cuta;
    RwReal maxr, maxg, maxb, maxa;
    _rwPalQuantLeafNode whole;

    BoxStats(&whole, root, set1);

    maxr = nMaximize(root, set1, 1, &cutr, &whole);
    maxg = nMaximize(root, set1, 2, &cutg, &whole);
    maxb = nMaximize(root, set1, 3, &cutb, &whole);
    maxa = nMaximize(root, set1, 4, &cuta, &whole);

    /* Can the box be split at all? */
    if ((maxr <= 0.0f) && (maxg <= 0.0f) && (maxb <= 0.0f) && (maxa <= 0.0f))
    {
        return 0;
    }

    *set2 = *set1;

    /* Split along the channel that gives the best improvement */
    if (maxr >= maxg)
    {
        if (maxr >= maxb)
        {
            if (maxr >= maxa)
            {
                set1->col1.red = set2->col0.red = (RwUInt8)cutr;
            }
            else
            {
                set1->col1.alpha = set2->col0.alpha = (RwUInt8)cuta;
            }
        }
        else if (maxb >= maxa)
        {
            set1->col1.blue = set2->col0.blue = (RwUInt8)cutb;
        }
        else
        {
            set1->col1.alpha = set2->col0.alpha = (RwUInt8)cuta;
        }
    }
    else if (maxg >= maxb)
    {
        if (maxg >= maxa)
        {
            set1->col1.green = set2->col0.green = (RwUInt8)cutg;
        }
        else
        {
            set1->col1.alpha = set2->col0.alpha = (RwUInt8)cuta;
        }
    }
    else if (maxb >= maxa)
    {
        set1->col1.blue = set2->col0.blue = (RwUInt8)cutb;
    }
    else
    {
        set1->col1.alpha = set2->col0.alpha = (RwUInt8)cuta;
    }

    return 1;
}

static RwInt32 CountLeafs(_rwPalQuantOctNode* root, RwInt32 depth)
{
    RwInt32 i;
    RwInt32 n = 0;

    if (root)
    {
        if (depth > 0)
        {
            for (i = 0; i < 16; i++)
            {
                n += CountLeafs(root->Branch.dir[i], depth - 1);
            }
        }
        else
        {
            n = 1;
        }
    }

    return n;
}

static RwInt32 ExtractNodes(_rwPalQuantOctNode* root, RwRGBA* palette, RwInt32 nodeIndex,
                            RwInt32 depth)
{
    RwInt32 i;

    if (root)
    {
        if (depth > 0)
        {
            for (i = 0; i < 16; i++)
            {
                nodeIndex = ExtractNodes(root->Branch.dir[i], palette, nodeIndex, depth - 1);
            }
        }
        else
        {
            RwReal recip;

            if (root->Leaf.weight > 0.0f)
            {
                recip = 255.9999f / root->Leaf.weight;
            }
            else
            {
                recip = 0.0f;
            }

            palette[nodeIndex].red = (RwUInt8)(RwInt32)(root->Leaf.ac.red * recip);
            palette[nodeIndex].green = (RwUInt8)(RwInt32)(root->Leaf.ac.green * recip);
            palette[nodeIndex].blue = (RwUInt8)(RwInt32)(root->Leaf.ac.blue * recip);
            palette[nodeIndex].alpha = (RwUInt8)(RwInt32)(root->Leaf.ac.alpha * recip);

            root->Leaf.palIndex = (RwUInt8)nodeIndex;
            nodeIndex++;
        }
    }

    return nodeIndex;
}

static RwReal Var(_rwPalQuantLeafNode* Vol)
{
    RwReal sqr;

    sqr = Vol->ac.red * Vol->ac.red + Vol->ac.green * Vol->ac.green +
          Vol->ac.blue * Vol->ac.blue + Vol->ac.alpha * Vol->ac.alpha;

    return Vol->var - sqr / Vol->weight;
}

RwInt32 _rwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, RwPalQuant* pq)
{
    RwInt32 numcols = maxcols;
    RwInt32 uniquecols;
    RwInt32 i;

    uniquecols = CountLeafs(pq->root, MaxDepth);

    if (maxcols >= uniquecols)
    {
        /* Every colour gets its own palette entry */
        numcols = uniquecols;

        i = ExtractNodes(pq->root, palette, 0, MaxDepth);

        while (i < maxcols)
        {
            palette[i].red = 0;
            palette[i].green = 0;
            palette[i].blue = 0;
            palette[i].alpha = 0;
            i++;
        }
    }
    else
    {
        _rwPalQuantLeafNode boxvol;
        _rwPalQuantLeafNode vol;
        _rwPalQuantLeafNode vol1;
        _rwPalQuantLeafNode vol2;

        /* Start with one box containing everything */
        pq->Mcube[0].col0.red = 0;
        pq->Mcube[0].col0.green = 0;
        pq->Mcube[0].col0.blue = 0;
        pq->Mcube[0].col0.alpha = 0;
        pq->Mcube[0].col1.red = 1 << MaxDepth;
        pq->Mcube[0].col1.green = 1 << MaxDepth;
        pq->Mcube[0].col1.blue = 1 << MaxDepth;
        pq->Mcube[0].col1.alpha = 1 << MaxDepth;

        pq->Mvv[0] = Var(BoxStats(&vol, pq->root, &pq->Mcube[0]));

        /* Repeatedly split the box with the largest variance */
        for (i = 1; i < maxcols; i++)
        {
            RwInt32 nextsplit = -1;
            RwReal maxvar = 0.0f;
            RwInt32 k;

            for (k = 0; k < i; k++)
            {
                if (pq->Mvv[k] > maxvar)
                {
                    maxvar = pq->Mvv[k];
                    nextsplit = k;
                }
            }

            if (nextsplit == -1)
            {
                break;
            }

            if (nCut(pq->root, &pq->Mcube[nextsplit], &pq->Mcube[i]))
            {
                pq->Mvv[nextsplit] = Var(BoxStats(&vol1, pq->root, &pq->Mcube[nextsplit]));
                pq->Mvv[i] = Var(BoxStats(&vol2, pq->root, &pq->Mcube[i]));
            }
            else
            {
                /* Box cannot be split, so don't try again */
                pq->Mvv[nextsplit] = 0.0f;
                i--;
            }
        }

        /* Build the palette from the boxes */
        for (i = 0; i < maxcols; i++)
        {
            if (i < numcols)
            {
                RwRGBA origin;
                RwReal recip;

                origin.red = 0;
                origin.green = 0;
                origin.blue = 0;
                origin.alpha = 0;
                assignindex(pq->root, &origin, MaxDepth, &pq->Mcube[i], i);

                BoxStats(&boxvol, pq->root, &pq->Mcube[i]);

                if (boxvol.weight > 0.0f)
                {
                    recip = 255.9999f / boxvol.weight;
                }
                else
                {
                    recip = 0.0f;
                }

                palette[i].red = (RwUInt8)(RwInt32)(boxvol.ac.red * recip);
                palette[i].green = (RwUInt8)(RwInt32)(boxvol.ac.green * recip);
                palette[i].blue = (RwUInt8)(RwInt32)(boxvol.ac.blue * recip);
                palette[i].alpha = (RwUInt8)(RwInt32)(boxvol.ac.alpha * recip);
            }
            else
            {
                palette[i].red = 0;
                palette[i].green = 0;
                palette[i].blue = 0;
                palette[i].alpha = 0;
            }
        }
    }

    return numcols;
}

static RwUInt8 GetIndex(_rwPalQuantOctNode* root, RwUInt32 Octs, RwInt32 depth)
{
    if (depth == 0)
    {
        return root->Leaf.palIndex;
    }

    return GetIndex(root->Branch.dir[Octs & 15], Octs >> 4, depth - 1);
}

void _rwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth, RwBool dstPacked,
                           RwPalQuant* pq, RwImage* img)
{
    RwUInt32 width;
    RwUInt32 x;
    RwUInt32 height;
    RwUInt32 stride;
    RwUInt8* pixels;
    RwUInt8* dstLinePixels;
    RwUInt8 nodeIndex;

    stride = img->stride;
    pixels = img->cpPixels;

    if ((dstdepth == 4) && dstPacked)
    {
        /* Two pixels per byte */
        switch (img->depth)
        {
        case 4:
        case 8:
        {
            RwRGBA* palette = img->palette;
            height = img->height;

            while (height--)
            {
                RwUInt8* srcLinePixels = pixels;

                dstLinePixels = dstpixels;
                width = img->width;

                for (x = 0; x < width; x++)
                {
                    RwRGBA* color = &palette[*srcLinePixels++];

                    nodeIndex = GetIndex(pq->root, GetOctAdr(color), MaxDepth);

                    if (x & 1)
                    {
                        *dstLinePixels &= 0x0F;
                        *dstLinePixels |= (nodeIndex & 0x0F) << 4;
                        dstLinePixels++;
                    }
                    else
                    {
                        *dstLinePixels &= 0xF0;
                        *dstLinePixels |= nodeIndex & 0x0F;
                    }
                }

                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        case 32:
        {
            height = img->height;

            while (height--)
            {
                RwRGBA* srcLinePixels = (RwRGBA*)pixels;

                dstLinePixels = dstpixels;
                width = img->width;

                for (x = 0; x < width; x++)
                {
                    RwRGBA* color = srcLinePixels++;

                    nodeIndex = GetIndex(pq->root, GetOctAdr(color), MaxDepth);

                    if (x & 1)
                    {
                        *dstLinePixels &= 0x0F;
                        *dstLinePixels |= (nodeIndex & 0x0F) << 4;
                        dstLinePixels++;
                    }
                    else
                    {
                        *dstLinePixels &= 0xF0;
                        *dstLinePixels |= nodeIndex & 0x0F;
                    }
                }

                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        }
    }
    else
    {
        /* One pixel per byte */
        switch (img->depth)
        {
        case 4:
        case 8:
        {
            RwRGBA* palette = img->palette;
            height = img->height;

            while (height--)
            {
                RwUInt8* srcLinePixels = pixels;

                dstLinePixels = dstpixels;
                width = img->width;


                while (width--)
                {
                    RwRGBA* color = &palette[*srcLinePixels++];

                    *dstLinePixels++ = GetIndex(pq->root, GetOctAdr(color), MaxDepth);
                }

                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        case 32:
        {
            height = img->height;

            while (height--)
            {
                RwRGBA* srcLinePixels = (RwRGBA*)pixels;

                dstLinePixels = dstpixels;
                width = img->width;


                while (width--)
                {
                    RwRGBA* color = srcLinePixels++;

                    *dstLinePixels++ = GetIndex(pq->root, GetOctAdr(color), MaxDepth);
                }

                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        }
    }
}

RwBool _rwPalQuantInit(RwPalQuant* pq)
{
    RwInt32 i;
    RwInt32 j;
    RwInt32 maxval = 1 << MaxDepth;

    /* Build the bit interleaving table */
    for (i = 0; i < maxval; i++)
    {
        RwUInt32 mask = 0;

        for (j = 0; j < MaxDepth; j++)
        {
            mask |= (i & (1 << j)) ? (1 << ((MaxDepth - 1 - j) << 2)) : 0;
        }

        splice[i] = mask;
    }

    pq->Mcube = (_rwPalQuantRGBABox*)RwCalloc(sizeof(_rwPalQuantRGBABox), 256);
    pq->Mvv = (RwReal*)RwCalloc(sizeof(RwReal), 256);

    pq->cubefreelist = RwFreeListCreate(sizeof(_rwPalQuantOctNode), 64, 4);
    pq->root = CreateCube(pq->cubefreelist);
    InitBranch(&pq->root->Branch);

    return TRUE;
}

static void DeleteOctTree(RwPalQuant* pq, _rwPalQuantOctNode* root, RwInt32 depth)
{
    if (root)
    {
        if (depth > 0)
        {
            RwInt32 i;

            for (i = 0; i < 16; i++)
            {
                DeleteOctTree(pq, root->Branch.dir[i], depth - 1);
            }
        }

        RwFreeListFree(pq->cubefreelist, root);
    }
}

void _rwPalQuantTerm(RwPalQuant* pq)
{
    DeleteOctTree(pq, pq->root, MaxDepth);
    pq->root = NULL;

    RwFreeListDestroy(pq->cubefreelist);

    RwFree(pq->Mvv);
    RwFree(pq->Mcube);
}
