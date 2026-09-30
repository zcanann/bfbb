#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rtintsec.h>
#include <rwsdk/rpcollis.h>
#include <rwsdk/rpcollbsptree.h>

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

RwInt32 _rpCollBSPTreeStreamGetSize(RpCollBSPTree* tree)
{
    return (tree->numLeafNodes - 1) * sizeof(RpCollBSPBranchNode) +
           tree->numLeafNodes * sizeof(RpCollBSPLeafNode);
}

RpCollBSPTree* _rpCollBSPTreeStreamWrite(RpCollBSPTree* tree, RwStream* stream)
{
    RwInt32 i;
    RpCollBSPBranchNode* branchNode;
    RpCollBSPLeafNode* leafNode;
    RwInt32 leaf;
    RwInt32 nodes;
    RwInt32 types;

    i = tree->numLeafNodes;
    branchNode = tree->branchNodes;

    while (--i)
    {
        types = (branchNode->type << 16) | (branchNode->leftType << 8) | branchNode->rightType;
        nodes = *(RwInt32*)&branchNode->leftNode;

        if (!RwStreamWriteInt32(stream, &types, sizeof(RwInt32)) ||
            !RwStreamWriteInt32(stream, &nodes, sizeof(RwInt32)) ||
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
        leaf = *(RwInt32*)leafNode;

        if (!RwStreamWriteInt32(stream, &leaf, sizeof(RwInt32)))
        {
            return (RpCollBSPTree*)NULL;
        }

        leafNode++;
    }

    return tree;
}
