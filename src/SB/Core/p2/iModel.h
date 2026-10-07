#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>

struct xLightKit;
struct RpAtomic;
struct xQuat;
struct xVec3;
struct xSphere;
struct xModelTag;

void iModelInit();
void iModelSetMaterialAlpha(RpAtomic* model, unsigned char alpha);
unsigned int iModelVertCount(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, float rm, float gm, float bm);

void iModel_SetLightKit(xLightKit* lightKit);
void iModelRender(RpAtomic* model, RwMatrix* mat);

void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* matrices);

unsigned int iModelVertEval(RpAtomic* model, unsigned int index, unsigned int count,
                            RwMatrix* mat, xVec3* vert, xVec3* dest);
unsigned int iModelNormalEval(xVec3* out, const RpAtomic& model, const RwMatrixTag* mat,
                               unsigned int index, signed int size, const xVec3* in);
void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest);
unsigned int iModelTagSetup(xModelTag* tag, RpAtomic* model, float x, float y, float z);

RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
void iModelSetMaterialTexture(RpAtomic* model, void* texture);
void iModelResetMaterial(RpAtomic* model);
signed int iModelCull(RpAtomic* model, RwMatrix* mat);
signed int iModelCullPlusShadow(RpAtomic* model, RwMatrix* mat, xVec3* shadowVec,
                               signed int* shadowOutside);

unsigned int iModelNumBones(RpAtomic* model);
signed int iModelSphereCull(xSphere* sphere);

RpAtomic* iModelFileNew(void* buffer, unsigned int size);
void iModelUnload(RpAtomic* userdata);

// Defined in iModelBucketPDS.cpp; the PS2 model layer builds its fast pipelines first.
void iModelInitFastPipes();

#endif
