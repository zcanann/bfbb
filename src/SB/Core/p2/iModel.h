#ifndef IMODEL_H
#define IMODEL_H

#include <rwcore.h>

struct xLightKit;
struct RpAtomic;

void iModel_SetLightKit(xLightKit* lightKit);
void iModelRender(RpAtomic* model, RwMatrix* mat);

#endif
