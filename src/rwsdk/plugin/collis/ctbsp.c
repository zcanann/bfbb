#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rtintsec.h>
#include <rwsdk/rpcollis.h>
#include <rwsdk/rpcollbsptree.h>

#define rpCOLLBSPTREEMAXDEPTH 32

/* Node types */
#define rpCOLLBSPLEAFNODE 1
#define rpCOLLBSPBRANCHNODE 2

typedef struct RpCollBSPNodeRef RpCollBSPNodeRef;
struct RpCollBSPNodeRef
{
    RwUInt32 type;
    RwUInt32 index;
};

typedef RwInt32 (*RpCollBSPLeafCallBack)(RwInt32 numPolygons, RwInt32 firstPolygon, void* data);

void _rpCollBSPTreeInit(RpCollBSPTree* tree, RwInt32 numLeafNodes)
{
    RwInt32 numBranchNodes = numLeafNodes - 1;

    tree->numLeafNodes = numLeafNodes;

    if (numBranchNodes > 0)
    {
        tree->branchNodes = (RpCollBSPBranchNode*)(tree + 1);
        tree->leafNodes = (RpCollBSPLeafNode*)(tree->branchNodes + numBranchNodes);
    }
    else
    {
        tree->branchNodes = (RpCollBSPBranchNode*)NULL;
        tree->leafNodes = (RpCollBSPLeafNode*)(tree + 1);
    }
}

RwInt32 _rpCollBSPTreeMemGetSize(RwInt32 numLeafNodes)
{
    return sizeof(RpCollBSPTree) + (numLeafNodes - 1) * sizeof(RpCollBSPBranchNode) +
           numLeafNodes * sizeof(RpCollBSPLeafNode);
}

void _rpCollBSPTreeDestroy(RpCollBSPTree* tree)
{
    if (tree)
    {
        RwFree(tree);
    }
}

RpCollBSPTree* _rpCollBSPTreeStreamWrite(RpCollBSPTree* tree, RwStream* stream)
{
    RwUInt32 i;
    RpCollBSPBranchNode* branchNode;
    RpCollBSPLeafNode* leafNode;

    i = tree->numLeafNodes - 1;
    branchNode = tree->branchNodes;

    while (i--)
    {
        RwUInt32 types;
        RwUInt32 nodes;

        types = (branchNode->type << 16) | (branchNode->leftType << 8) | branchNode->rightType;
        nodes = (branchNode->leftNode << 16) | branchNode->rightNode;

        if (!RwStreamWriteInt32(stream, (RwInt32*)&types, sizeof(RwInt32)) ||
            !RwStreamWriteInt32(stream, (RwInt32*)&nodes, sizeof(RwInt32)) ||
            !RwStreamWriteReal(stream, &branchNode->leftValue, sizeof(RwReal)) ||
            !RwStreamWriteReal(stream, &branchNode->rightValue, sizeof(RwReal)))
        {
            return (RpCollBSPTree*)NULL;
        }

        branchNode++;
    }

    i = tree->numLeafNodes;
    leafNode = tree->leafNodes;

    while (i--)
    {
        RwUInt32 leaf;

        leaf = (leafNode->numPolygons << 16) | leafNode->firstPolygon;

        if (!RwStreamWriteInt32(stream, (RwInt32*)&leaf, sizeof(RwInt32)))
        {
            return (RpCollBSPTree*)NULL;
        }

        leafNode++;
    }

    return tree;
}

RpCollBSPTree* _rpCollBSPTreeStreamRead(RpCollBSPTree* tree, RwStream* stream)
{
    RwUInt32 i;
    RpCollBSPBranchNode* branchNode;
    RpCollBSPLeafNode* leafNode;

    i = tree->numLeafNodes - 1;
    branchNode = tree->branchNodes;

    while (i--)
    {
        RwUInt32 types;
        RwUInt32 nodes;

        if (!RwStreamReadInt32(stream, (RwInt32*)&types, sizeof(RwInt32)) ||
            !RwStreamReadInt32(stream, (RwInt32*)&nodes, sizeof(RwInt32)) ||
            !RwStreamReadReal(stream, &branchNode->leftValue, sizeof(RwReal)) ||
            !RwStreamReadReal(stream, &branchNode->rightValue, sizeof(RwReal)))
        {
            return (RpCollBSPTree*)NULL;
        }

        branchNode->type = (RwUInt16)(types >> 16);
        branchNode->leftType = (RwUInt8)(types >> 8);
        branchNode->rightType = (RwUInt8)types;
        branchNode->leftNode = (RwUInt16)(nodes >> 16);
        branchNode->rightNode = (RwUInt16)nodes;

        branchNode++;
    }

    i = tree->numLeafNodes;
    leafNode = tree->leafNodes;

    while (i--)
    {
        RwUInt32 leaf;

        if (!RwStreamReadInt32(stream, (RwInt32*)&leaf, sizeof(RwInt32)))
        {
            _rpCollBSPTreeDestroy(tree);
            return (RpCollBSPTree*)NULL;
        }

        leafNode->numPolygons = (RwUInt16)(leaf >> 16);
        leafNode->firstPolygon = (RwUInt16)leaf;

        leafNode++;
    }

    return tree;
}

RwInt32 _rpCollBSPTreeStreamGetSize(RpCollBSPTree* tree)
{
    return (tree->numLeafNodes - 1) * sizeof(RpCollBSPBranchNode) +
           tree->numLeafNodes * sizeof(RpCollBSPLeafNode);
}

RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                            RpCollBSPLeafCallBack callBack,
                                                            void* data)
{
    RpCollBSPNodeRef nodeStack[rpCOLLBSPTREEMAXDEPTH];
    RpCollBSPNodeRef node;
    RwInt32 nStack = 0;

    node.type = tree->branchNodes ? rpCOLLBSPBRANCHNODE : rpCOLLBSPLEAFNODE;
    node.index = 0;

    while (nStack >= 0)
    {
        if (node.type == rpCOLLBSPLEAFNODE)
        {
            RpCollBSPLeafNode* leaf = &tree->leafNodes[node.index];

            if (!callBack(leaf->numPolygons, leaf->firstPolygon, data))
            {
                return (RpCollBSPTree*)NULL;
            }

            node = nodeStack[nStack--];
        }
        else
        {
            RpCollBSPBranchNode* branch = &tree->branchNodes[node.index];

            if (*(RwReal*)((RwUInt8*)&box->inf + branch->type) < branch->leftValue)
            {
                node.type = branch->leftType;
                node.index = branch->leftNode;

                if (*(RwReal*)((RwUInt8*)&box->sup + branch->type) >= branch->rightValue)
                {
                    nStack++;
                    nodeStack[nStack].index = branch->rightNode;
                    nodeStack[nStack].type = branch->rightType;
                }
            }
            else
            {
                node.type = branch->rightType;
                node.index = branch->rightNode;
            }
        }
    }

    return tree;
}
