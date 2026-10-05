#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>

struct xLightKit;
struct RpAtomic;
struct xQuat;
struct xVec3;

void iModel_SetLightKit(xLightKit* lightKit);
void iModelRender(RpAtomic* model, RwMatrix* mat);

void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* matrices);

#endif
