#ifndef PS2_RPPDS_H
#define PS2_RPPDS_H

// The subset of the RenderWare PS2 pipeline delivery system (rppds.h) used
// by the PS2 platform layer; layouts and pipe IDs from the PS2 DWARF.

#include <rwcore.h>

enum RpPDSPipeType
{
    rpNAPDSPIPETYPE = 0,
    rpPDSMATPIPE = 1,
    rpPDSOBJPIPE = 2,
    rpPDSPIPETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RpPDSPipeID
{
    rpNAPDSPIPEID = 0,
    rwPDS_G3_Generic_MatPipeID = 1,
    rwPDS_G3_Generic_AtmPipeID = 2,
    rwPDS_G3_Generic_SctPipeID = 3,
    rwPDS_G3_Im3D_TriPipeID = 4,
    rwPDS_G3_Im3D_SegPipeID = 5,
    rwPDS_G3_Im3D_TriObjPipeID = 6,
    rwPDS_G3_Im3D_SegObjPipeID = 7,
    rwPDS_G3_Generic_GrpMatPipeID = 4097,
    rwPDS_G3_Generic_GrpAtmPipeID = 4098,
    rwPDS_G3_Generic_GrpSctPipeID = 4099,
    rwPDS_G3_Skin_GrpMatPipeID = 69633,
    rwPDS_G3_Skin_GrpAtmPipeID = 69634,
    rwPDS_G3_MatfxUV1_GrpMatPipeID = 69643,
    rwPDS_G3_MatfxUV2_GrpMatPipeID = 69644,
    rwPDS_G3_MatfxUV1_GrpAtmPipeID = 69645,
    rwPDS_G3_MatfxUV2_GrpAtmPipeID = 69646,
    rwPDS_G3_MatfxUV1_GrpSctPipeID = 69647,
    rwPDS_G3_MatfxUV2_GrpSctPipeID = 69648,
    rwPDS_G3_SkinfxUV1_GrpMatPipeID = 69649,
    rwPDS_G3_SkinfxUV2_GrpMatPipeID = 69650,
    rwPDS_G3_SkinfxUV1_GrpAtmPipeID = 69651,
    rwPDS_G3_SkinfxUV2_GrpAtmPipeID = 69652,
    rwPDS_G3x_Generic_AtmPipeID = 327681,
    rwPDS_G3x_ADL_MatPipeID = 327683,
    rwPDS_G3x_A4D_MatPipeID = 327684,
    rwPDS_G3x_Skin_AtmPipeID = 327691,
    rwPDS_G3x_ADLSkin_MatPipeID = 327693,
    rwPDS_G3x_A4DSkin_MatPipeID = 327694,
    rwPDS_G3x_OPLClone_MatPipeID = 327703,
    rwPDS_G3x_OPLClone_AtmPipeID = 327704,
    rwPDS_G3xd_ADL_MatPipeID = 327726,
    rwPDS_G3xd_A4D_MatPipeID = 327727,
    rwPDS_G3xd_ADLGem_MatPipeID = 327732,
    rwPDS_G3xd_A4DGem_MatPipeID = 327733,
    rwPDS_G3xd_ADLSkin_MatPipeID = 327741,
    rwPDS_G3xd_A4DSkin_MatPipeID = 327742,
    rwPDS_G3xd_ADLSkinGem_MatPipeID = 327747,
    rwPDS_G3xd_A4DSkinGem_MatPipeID = 327748,
    rpPDSPIPEIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

struct RpPDSSkyMatTemplate;
struct RpPDSSkyObjTemplate;
struct RxPipeline;

union RpPDSPipeDefinition
{
    void* ptr;
    RpPDSSkyMatTemplate* mat;
    RpPDSSkyObjTemplate* obj;
};

struct RpPDSRegister
{
    RpPDSPipeDefinition def;
    RpPDSPipeID attachId;
    RpPDSPipeID id;
    RpPDSPipeType type;
    RxPipeline* pipe;
};

extern "C" {
extern RpPDSSkyObjTemplate rwPDS_G3_Generic_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_Generic_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_Generic_SctPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_Im3D_SegObjPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_Im3D_SegPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_Im3D_TriObjPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_Im3D_TriPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_MatfxUV1_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_MatfxUV1_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_MatfxUV1_SctPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_MatfxUV2_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_MatfxUV2_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_MatfxUV2_SctPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_Skin_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_Skin_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_SkinfxUV1_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_SkinfxUV1_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3_SkinfxUV2_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3_SkinfxUV2_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3x_A4DSkin_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3x_A4D_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3x_ADLSkin_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3x_ADL_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3x_Generic_AtmPipe;
extern RpPDSSkyObjTemplate rwPDS_G3x_OPLClone_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3x_OPLClone_MatPipe;
extern RpPDSSkyObjTemplate rwPDS_G3x_Skin_AtmPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_A4DGem_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_A4DSkinGem_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_A4DSkin_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_A4D_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_ADLGem_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_ADLSkinGem_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_ADLSkin_MatPipe;
extern RpPDSSkyMatTemplate rwPDS_G3xd_ADL_MatPipe;

RwBool RpPDSPluginAttach(RwInt32 maxPipes);
RwBool RpPDSRegisterPipe(RpPDSRegister* reg);
}

#define RPPDS_REGISTER_PIPE(_reg, _def, _attachId, _id, _type)                                     \
    MACRO_START                                                                                    \
    {                                                                                              \
        RpPDSRegister _reg;                                                                        \
        _reg.def.ptr = &(_def);                                                                    \
        _reg.id = (_id);                                                                           \
        _reg.type = (_type);                                                                       \
        _reg.attachId = (_attachId);                                                               \
        _reg.pipe = NULL;                                                                          \
        RpPDSRegisterPipe(&_reg);                                                                  \
    }                                                                                              \
    MACRO_STOP

#define rwPDS_G3x_Generic_AtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3x_Generic_AtmPipe, rpNAPDSPIPEID, rwPDS_G3x_Generic_AtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3x_ADL_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3x_ADL_MatPipe, rpNAPDSPIPEID, rwPDS_G3x_ADL_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3x_A4D_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3x_A4D_MatPipe, rpNAPDSPIPEID, rwPDS_G3x_A4D_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_Generic_AtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Generic_AtmPipe, rpNAPDSPIPEID, rwPDS_G3_Generic_AtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Generic_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_Generic_MatPipe, rpNAPDSPIPEID, rwPDS_G3_Generic_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3x_Skin_AtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3x_Skin_AtmPipe, rpNAPDSPIPEID, rwPDS_G3x_Skin_AtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3x_ADLSkin_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3x_ADLSkin_MatPipe, rpNAPDSPIPEID, rwPDS_G3x_ADLSkin_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3x_A4DSkin_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3x_A4DSkin_MatPipe, rpNAPDSPIPEID, rwPDS_G3x_A4DSkin_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_ADL_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_ADL_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_ADL_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_A4D_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_A4D_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_A4D_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_ADLSkin_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_ADLSkin_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_ADLSkin_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_A4DSkin_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_A4DSkin_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_A4DSkin_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_ADLGem_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_ADLGem_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_ADLGem_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_A4DGem_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_A4DGem_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_A4DGem_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_ADLSkinGem_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_ADLSkinGem_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_ADLSkinGem_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3xd_A4DSkinGem_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3xd_A4DSkinGem_MatPipe, rpNAPDSPIPEID, rwPDS_G3xd_A4DSkinGem_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3x_OPLClone_MatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3x_OPLClone_MatPipe, rpNAPDSPIPEID, rwPDS_G3x_OPLClone_MatPipeID, rpPDSMATPIPE)
#define rwPDS_G3x_OPLClone_AtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3x_OPLClone_AtmPipe, rpNAPDSPIPEID, rwPDS_G3x_OPLClone_AtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Generic_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_Generic_MatPipe, rpNAPDSPIPEID, rwPDS_G3_Generic_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_Generic_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Generic_AtmPipe, rwPDS_G3_Generic_GrpMatPipeID, rwPDS_G3_Generic_GrpAtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Generic_GrpSctPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Generic_SctPipe, rwPDS_G3_Generic_GrpMatPipeID, rwPDS_G3_Generic_GrpSctPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Im3D_TriPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_Im3D_TriPipe, rpNAPDSPIPEID, rwPDS_G3_Im3D_TriPipeID, rpPDSMATPIPE)
#define rwPDS_G3_Im3D_SegPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_Im3D_SegPipe, rpNAPDSPIPEID, rwPDS_G3_Im3D_SegPipeID, rpPDSMATPIPE)
#define rwPDS_G3_Im3D_TriObjPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Im3D_TriObjPipe, rwPDS_G3_Im3D_TriPipeID, rwPDS_G3_Im3D_TriObjPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Im3D_SegObjPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Im3D_SegObjPipe, rwPDS_G3_Im3D_SegPipeID, rwPDS_G3_Im3D_SegObjPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_MatfxUV1_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_MatfxUV1_MatPipe, rpNAPDSPIPEID, rwPDS_G3_MatfxUV1_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_MatfxUV2_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_MatfxUV2_MatPipe, rpNAPDSPIPEID, rwPDS_G3_MatfxUV2_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_MatfxUV1_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_MatfxUV1_AtmPipe, rwPDS_G3_MatfxUV1_GrpMatPipeID, rwPDS_G3_MatfxUV1_GrpAtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_MatfxUV2_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_MatfxUV2_AtmPipe, rwPDS_G3_MatfxUV2_GrpMatPipeID, rwPDS_G3_MatfxUV2_GrpAtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_MatfxUV1_GrpSctPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_MatfxUV1_SctPipe, rwPDS_G3_MatfxUV1_GrpMatPipeID, rwPDS_G3_MatfxUV1_GrpSctPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_MatfxUV2_GrpSctPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_MatfxUV2_SctPipe, rwPDS_G3_MatfxUV2_GrpMatPipeID, rwPDS_G3_MatfxUV2_GrpSctPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_Skin_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_Skin_MatPipe, rpNAPDSPIPEID, rwPDS_G3_Skin_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_Skin_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_Skin_AtmPipe, rwPDS_G3_Skin_GrpMatPipeID, rwPDS_G3_Skin_GrpAtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_SkinfxUV1_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_SkinfxUV1_MatPipe, rpNAPDSPIPEID, rwPDS_G3_SkinfxUV1_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_SkinfxUV2_GrpMatPipeRegister() RPPDS_REGISTER_PIPE(_matPipe, rwPDS_G3_SkinfxUV2_MatPipe, rpNAPDSPIPEID, rwPDS_G3_SkinfxUV2_GrpMatPipeID, rpPDSMATPIPE)
#define rwPDS_G3_SkinfxUV1_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_SkinfxUV1_AtmPipe, rwPDS_G3_SkinfxUV1_GrpMatPipeID, rwPDS_G3_SkinfxUV1_GrpAtmPipeID, rpPDSOBJPIPE)
#define rwPDS_G3_SkinfxUV2_GrpAtmPipeRegister() RPPDS_REGISTER_PIPE(_objPipe, rwPDS_G3_SkinfxUV2_AtmPipe, rwPDS_G3_SkinfxUV2_GrpMatPipeID, rwPDS_G3_SkinfxUV2_GrpAtmPipeID, rpPDSOBJPIPE)

#endif
