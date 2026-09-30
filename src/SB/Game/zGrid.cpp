#include "zBase.h"
#include "zGrid.h"
#include "xEnt.h"
#include "xGrid.h"
#include "xString.h"

#include "xVec3.h"
#include <types.h>
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

xGrid colls_grid;
xGrid colls_oso_grid;
xGrid npcs_grid;
static S32 zGridInitted;

static void hack_flag_shadows(zScene* s)
{
    static U32 special_models[25] = {
        xStrHash("bb_arrow"),
        xStrHash("metal_sheets"),
        xStrHash("clam_poop"),
        xStrHash("beach_chair_yellow"),
        xStrHash("debris_pile_rb_smll"),
        xStrHash("debris_pile_rb"),
        xStrHash("floor_panel"),
        xStrHash("gy_woodpieceA"),
        xStrHash("quarter_note_on"),
        xStrHash("eighth_note_on"),
        xStrHash("db03_path_a"),
        xStrHash("db03_path_b"),
        xStrHash("db03_path_c"),
        xStrHash("db03_path_d"),
        xStrHash("db03_path_e"),
        xStrHash("db03_path_f"),
        xStrHash("db03_path_g"),
        xStrHash("db03_path_h"),
        xStrHash("db03_path_i"),
        xStrHash("db03_path_j"),
        xStrHash("db03_path_k"),
        xStrHash("db03_path_l"),
        xStrHash("db03_path_m"),
        xStrHash("db03_path_o"),
        xStrHash("db03_path_p"),
    }; // non-matching: stb happens too early?

    zEnt** it = s->ents;
    zEnt** end = s->ents + s->num_ents;

    while (it != end)
    {
        // non-matching: extra `mr` instruction?
        xEnt* ent = *it;
        if (ent && (ent->baseFlags & 0x20) && ent->asset)
        {
            U32* id = special_models;
            U32* end_special_models = id + 25;

            while (id != end_special_models)
            {
                if (ent->asset->modelInfoID == *id)
                {
                    ent->chkby |= 0x80;
                    ent->baseFlags |= 0x10;
                    ent->asset->baseFlags |= 0x10;
                    break;
                }
                id++;
            }
        }
        it++;
    }
}

void zGridReset(zScene* s)
{
    hack_flag_shadows(s);

    for (U32 i = 0; i < s->num_ents; i++)
    {
        xBase* base = s->ents[i];
        if (base && (base->baseFlags & 0x20))
        {
            if (base->baseType != eBaseTypeTrigger && base->baseType != eBaseTypeUI &&
                base->baseType != eBaseTypeUIFont && base->baseType != eBaseTypePlayer)
            {
                xEnt* ent = (xEnt*)base;
                if (ent->bupdate)
                {
                    ent->bupdate(ent, (xVec3*)&ent->model->Mat->pos);
                }
                else
                {
                    xEntDefaultBoundUpdate(ent, (xVec3*)&ent->model->Mat->pos);
                }
                zGridUpdateEnt(ent);
            }
        }
    }
}

void zGridInit(zScene* s)
{
    gGridIterActive = NULL;
    xBox* ebox = xEntGetAllEntsBox();
    F32 min_csize;

    F32 size_x = MAX(0.001f, ebox->upper.x - ebox->lower.x);
    F32 size_z = MAX(0.001f, ebox->upper.z - ebox->lower.z);

    min_csize = 10.0f;

    xGridInit(&colls_grid, ebox, (U16)MAX(1.0f, MIN(32.0f, std::floorf(size_x / min_csize))),
              (U16)MAX(1.0f, MIN(32.0f, std::floorf(size_z / min_csize))), 1);

    xBox osobox = *ebox;
    osobox.lower.x -= 1.0f;
    osobox.lower.z -= 1.0f;
    osobox.upper.x += 3.4567f;
    osobox.upper.z += 3.4567f;

    F32 osize_x = MAX(0.001f, osobox.upper.x - osobox.lower.x);
    F32 osize_z = MAX(0.001f, osobox.upper.z - osobox.lower.z);

    min_csize *= 6.0f;

    xGridInit(&colls_oso_grid, &osobox,
              (U16)MAX(1.0f, MIN(8.0f, std::floorf(osize_x / min_csize))),
              (U16)MAX(1.0f, MIN(8.0f, std::floorf(osize_z / min_csize))), 2);

    xGridInit(&npcs_grid, ebox, (U16)MAX(1.0f, MIN(16.0f, std::floorf(osize_x / 20.0f))),
              (U16)MAX(1.0f, MIN(16.0f, std::floorf(osize_z / 20.0f))), 3);

    zGridInitted = TRUE;
    zGridReset(s);
}

void zGridExit(zScene* s)
{
    xGridKill(&colls_grid);
    xGridKill(&colls_oso_grid);
    xGridKill(&npcs_grid);

    gGridIterActive = NULL;
    zGridInitted = FALSE;
}

// WIP
void zGridUpdateEnt(xEnt* ent)
{
    if (!zGridInitted)
        return;

    S32 oversize = 0;
    xGrid* grid = NULL;

    switch (ent->gridb.ingrid)
    {
    case 1:
        grid = &colls_grid;
        break;
    case 2:
        grid = &colls_oso_grid;
        oversize = (ent->gridb.oversize == 2);
        break;
    case 3:
        grid = &npcs_grid;
        oversize = (ent->gridb.oversize == 1);
        break;
    default:
        break;
    }

    if ((ent->chkby & 0x98) != 0 || ent->baseType == eBaseTypePickup)
    {
        if (grid != NULL)
        {
            if (oversize == 0)
            {
                xGridUpdate(grid, ent);
            }
        }
        else if (ent->collType == XENT_COLLTYPE_NPC)
        {
            if (xGridEntIsTooBig(&npcs_grid, ent))
            {
                ent->gridb.oversize = 1;
            }
            else
            {
                ent->gridb.oversize = 0;
            }
            xGridAdd(&npcs_grid, ent);
        }
        else
        {
            if (xGridEntIsTooBig(&colls_grid, ent))
            {
                if (xGridEntIsTooBig(&colls_oso_grid, ent))
                {
                    ent->gridb.oversize = 2;
                }
                else
                {
                    ent->gridb.oversize = 1;
                }
                xGridAdd(&colls_oso_grid, ent);
            }
            else
            {
                xGridAdd(&colls_grid, ent);
                ent->gridb.oversize = 0;
            }
        }
    }
    else if (grid != NULL)
    {
        xGridRemove(&ent->gridb);
    }
}
