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

RpCollBSPTree* _rpCollBSPTreeStreamWrite(const RpCollBSPTree* tree, RwStream* stream)
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

    return (RpCollBSPTree*)tree;
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

/* Where a line crosses the axis aligned plane at _value. _type is the byte
 * offset of the plane's axis within an RwV3d. */
#define rpCOLLBSPLINEPLANEINTERSECT(_point, _line, _grad, _type, _value)                           \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwReal _delta;                                                                             \
                                                                                                   \
        switch (_type)                                                                             \
        {                                                                                          \
        case 0:                                                                                    \
            _delta = (_value) - (_line).start.x;                                                   \
            (_point).x = (_value);                                                                 \
            (_point).y = (_grad)->dydx * _delta + (_line).start.y;                                 \
            (_point).z = (_grad)->dzdx * _delta + (_line).start.z;                                 \
            break;                                                                                 \
        case 4:                                                                                    \
            _delta = (_value) - (_line).start.y;                                                   \
            (_point).x = (_grad)->dxdy * _delta + (_line).start.x;                                 \
            (_point).y = (_value);                                                                 \
            (_point).z = (_grad)->dzdy * _delta + (_line).start.z;                                 \
            break;                                                                                 \
        case 8:                                                                                    \
            _delta = (_value) - (_line).start.z;                                                   \
            (_point).x = (_grad)->dxdz * _delta + (_line).start.x;                                 \
            (_point).y = (_grad)->dydz * _delta + (_line).start.y;                                 \
            (_point).z = (_value);                                                                 \
            break;                                                                                 \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree, RwLine* line,
                                                             RpV3dGradient* grad,
                                                             RpCollBSPLeafCallBack callBack,
                                                             void* data)
{
    RpCollBSPNodeRef nodeStack[rpCOLLBSPTREEMAXDEPTH + 1];
    RwLine lineStack[rpCOLLBSPTREEMAXDEPTH + 1];
    RpCollBSPNodeRef node;
    RwLine currLine;
    RwInt32 nStack = 0;

    node.type = tree->branchNodes ? rpCOLLBSPBRANCHNODE : rpCOLLBSPLEAFNODE;
    node.index = 0;
    currLine = *line;

    while (nStack >= 0)
    {
        if (node.type == rpCOLLBSPLEAFNODE)
        {
            RpCollBSPLeafNode* leaf = &tree->leafNodes[node.index];

            if (!callBack(leaf->numPolygons, leaf->firstPolygon, data))
            {
                return (RpCollBSPTree*)NULL;
            }

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        }
        else
        {
            RpCollBSPBranchNode* branch = &tree->branchNodes[node.index];
            RwUInt32 type;
            RwUInt32 leftType;
            RwUInt32 rightType;
            RwUInt32 leftNode;
            RwUInt32 rightNode;
            RwSplitBits startLeft;
            RwSplitBits endLeft;
            RwSplitBits startRight;
            RwSplitBits endRight;

            type = branch->type;

            /* Distances of the line ends from the two splitting planes */
            startRight.nReal = *(RwReal*)((RwUInt8*)&currLine.start + type) - branch->rightValue;
            startLeft.nReal = *(RwReal*)((RwUInt8*)&currLine.start + type) - branch->leftValue;
            endLeft.nReal = *(RwReal*)((RwUInt8*)&currLine.end + type) - branch->leftValue;
            endRight.nReal = *(RwReal*)((RwUInt8*)&currLine.end + type) - branch->rightValue;

            leftType = branch->leftType;
            rightType = branch->rightType;
            leftNode = branch->leftNode;
            rightNode = branch->rightNode;

            if (startRight.nInt < 0 && endRight.nInt < 0)
            {
                /* Entirely on the left */
                node.type = leftType;
                node.index = leftNode;
            }
            else if (startLeft.nInt >= 0 && endLeft.nInt >= 0)
            {
                /* Entirely on the right */
                node.type = rightType;
                node.index = rightNode;
            }
            else if (!((startLeft.nInt ^ endLeft.nInt) & 0x80000000) &&
                     !((startRight.nInt ^ endRight.nInt) & 0x80000000))
            {
                /* Within the overlap: visit both sides with the whole line */
                if (startRight.nInt < endRight.nInt)
                {
                    nStack++;
                    nodeStack[nStack].type = rightType;
                    nodeStack[nStack].index = rightNode;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = leftType;
                    node.index = leftNode;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = leftType;
                    nodeStack[nStack].index = leftNode;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = rightType;
                    node.index = rightNode;
                }
            }
            else if (((startLeft.nInt ^ endLeft.nInt) & 0x80000000) &&
                     startRight.nInt >= 0 && endRight.nInt >= 0)
            {
                /* Crosses the left plane only */
                RwV3d leftPoint;

                rpCOLLBSPLINEPLANEINTERSECT(leftPoint, currLine, grad, type,
                                            branch->leftValue);

                if (startLeft.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = rightType;
                    nodeStack[nStack].index = rightNode;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = leftType;
                    node.index = leftNode;
                    currLine.end = leftPoint;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = leftType;
                    nodeStack[nStack].index = leftNode;
                    lineStack[nStack].start = leftPoint;
                    lineStack[nStack].end = currLine.end;
                    node.type = rightType;
                    node.index = rightNode;
                }
            }
            else if (((startRight.nInt ^ endRight.nInt) & 0x80000000) &&
                     startLeft.nInt < 0 && endLeft.nInt < 0)
            {
                /* Crosses the right plane only */
                RwV3d rightPoint;

                rpCOLLBSPLINEPLANEINTERSECT(rightPoint, currLine, grad, type,
                                            branch->rightValue);

                if (startRight.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = rightType;
                    nodeStack[nStack].index = rightNode;
                    lineStack[nStack].start = rightPoint;
                    lineStack[nStack].end = currLine.end;
                    node.type = leftType;
                    node.index = leftNode;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = leftType;
                    nodeStack[nStack].index = leftNode;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = rightType;
                    node.index = rightNode;
                    currLine.end = rightPoint;
                }
            }
            else
            {
                /* Crosses both planes */
                RwV3d leftPoint;
                RwV3d rightPoint;

                rpCOLLBSPLINEPLANEINTERSECT(leftPoint, currLine, grad, type,
                                            branch->leftValue);
                rpCOLLBSPLINEPLANEINTERSECT(rightPoint, currLine, grad, type,
                                            branch->rightValue);

                if (startLeft.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = rightType;
                    nodeStack[nStack].index = rightNode;
                    lineStack[nStack].start = rightPoint;
                    lineStack[nStack].end = currLine.end;
                    node.type = leftType;
                    node.index = leftNode;
                    currLine.end = leftPoint;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = leftType;
                    nodeStack[nStack].index = leftNode;
                    lineStack[nStack].start = leftPoint;
                    lineStack[nStack].end = currLine.end;
                    node.type = rightType;
                    node.index = rightNode;
                    currLine.end = rightPoint;
                }
            }
        }
    }

    return tree;
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
