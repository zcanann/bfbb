#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>

struct xLightKit;
struct RpAtomic;
struct xQuat;
struct xVec3;
struct xModelTag;

void iModel_SetLightKit(xLightKit* lightKit);
void iModelRender(RpAtomic* model, RwMatrix* mat);

void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* matrices);

unsigned int iModelVertEval(RpAtomic* model, unsigned int index, unsigned int count,
                            RwMatrix* mat, xVec3* vert, xVec3* dest);
void iModelTagEval(RpAtomic* model, const xModelTag* tag, RwMatrix* mat, xVec3* dest);

RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
void iModelSetMaterialTexture(RpAtomic* model, void* texture);
void iModelResetMaterial(RpAtomic* model);
signed int iModelCull(RpAtomic* model, RwMatrix* mat);
signed int iModelCullPlusShadow(RpAtomic* model, RwMatrix* mat, xVec3* shadowVec,
                               signed int* shadowOutside);

#endif
