#ifndef ZTALKBOXASSETS_H
#define ZTALKBOXASSETS_H

struct location_asset : xDynAsset
{
    xVec3 loc; // offset 0x10, size 0xC

    static const char* type_name()
    {
        return "location";
    }
};
struct pointer_asset : xDynAsset
{
    xVec3 loc; // offset 0x10, size 0xC
    float yaw; // offset 0x1C, size 0x4
    float pitch; // offset 0x20, size 0x4
    float roll; // offset 0x24, size 0x4

    static const char* type_name()
    {
        return "pointer";
    }
};

#endif
