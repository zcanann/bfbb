#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

#define rwID_GCNMULTITEXPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x29)

typedef struct rpGameCubeMTExtension rpGameCubeMTExtension;
struct rpGameCubeMTExtension
{
    RwUInt32 pad[3];
};

extern RwBool _rpMultiTexturePluginAttach(void);
extern RwBool _rpMaterialRegisterMultiTexturePlugin(RwPlatformID platformID, RwUInt32 pluginID,
                                                    RwUInt32 extensionSize);
extern RwBool _rpGameCubeMTDataPluginAttach(void);
extern RwBool _rpGameCubeMTPipePluginAttach(void);

RwBool _rpMultiTexturePlatformPluginsAttach(void)
{
    RwBool result;

    result = _rpMultiTexturePluginAttach();
    if (!result)
    {
        return FALSE;
    }

    result = _rpMaterialRegisterMultiTexturePlugin(rwID_GAMECUBE, rwID_GCNMULTITEXPLUGIN,
                                                   sizeof(rpGameCubeMTExtension));
    if (!result)
    {
        return FALSE;
    }

    result = _rpGameCubeMTDataPluginAttach();
    if (!result)
    {
        return FALSE;
    }

    result = _rpGameCubeMTPipePluginAttach();

    return result;
}
