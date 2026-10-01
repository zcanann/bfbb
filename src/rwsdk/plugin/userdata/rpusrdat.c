#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpusrdat.h>
#include <string.h>

#define rwID_USERDATAPLUGIN 0x11F

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct RpUserDataList RpUserDataList;
struct RpUserDataList
{
    RwInt32 numElements;
    RpUserDataArray* userData;
};

#define RPUSERDATALISTGETDATA(_object, _offset) (RWPLUGINOFFSET(RpUserDataList, _object, _offset))

/* Duplicate a string with RwMalloc (RenderWare's rwstrdup) */
#define UserDataStringDuplicate(_dst, _src)                                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_dst) = (RwChar*)NULL;                                                                    \
                                                                                                   \
        if (((RwChar*)NULL) != (_src))                                                             \
        {                                                                                          \
            (_dst) = (RwChar*)RwMalloc(rwstrlen(_src) + 1);                                        \
                                                                                                   \
            if (((RwChar*)NULL) != (_dst))                                                         \
            {                                                                                      \
                rwstrcpy(_dst, _src);                                                              \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RwModuleInfo userDataModule;

static RwInt32 userDataTextureStreamOffset;
static RwInt32 userDataTextureOffset;
static RwInt32 userDataMaterialStreamOffset;
static RwInt32 userDataMaterialOffset;
static RwInt32 userDataLightStreamOffset;
static RwInt32 userDataLightOffset;
static RwInt32 userDataCameraStreamOffset;
static RwInt32 userDataCameraOffset;
static RwInt32 userDataFrameStreamOffset;
static RwInt32 userDataFrameOffset;
static RwInt32 userDataWorldSectorStreamOffset;
static RwInt32 userDataWorldSectorOffset;
static RwInt32 userDataGeometryStreamOffset;
static RwInt32 userDataGeometryOffset;

static void* UserDataOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    userDataModule.numInstances++;

    return instance;
}

static void* UserDataClose(void* instance, RwInt32 offset, RwInt32 size)
{
    userDataModule.numInstances--;

    return instance;
}

static RwStream* UserDataStreamRead(RpUserDataArray* userData, RwStream* stream)
{
    RwInt32 nameLength;
    RwInt32 i;

    if (!RwStreamReadInt32(stream, &nameLength, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (nameLength > 0)
    {
        userData->name = (RwChar*)RwMalloc(nameLength);
        if (!userData->name)
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamRead(stream, userData->name, nameLength))
        {
            return (RwStream*)NULL;
        }
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&userData->format, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, &userData->numElements, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    switch (userData->format)
    {
    case rpINTUSERDATA:
    {
        RwInt32* intData;

        userData->data = RwMalloc(sizeof(RwInt32) * userData->numElements);
        if (!userData->data)
        {
            return (RwStream*)NULL;
        }

        intData = (RwInt32*)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (!RwStreamReadInt32(stream, &intData[i], sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

        }

        break;
    }
    case rpREALUSERDATA:
    {
        RwReal* realData;

        userData->data = RwMalloc(sizeof(RwReal) * userData->numElements);
        if (!userData->data)
        {
            return (RwStream*)NULL;
        }

        realData = (RwReal*)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (!RwStreamReadReal(stream, &realData[i], sizeof(RwReal)))
            {
                return (RwStream*)NULL;
            }

        }

        break;
    }
    case rpSTRINGUSERDATA:
    {
        RwChar** stringData;

        userData->data = RwMalloc(sizeof(RwChar*) * userData->numElements);
        if (!userData->data)
        {
            return (RwStream*)NULL;
        }

        stringData = (RwChar**)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (!RwStreamReadInt32(stream, &nameLength, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (nameLength > 0)
            {
                stringData[i] = (RwChar*)RwMalloc(nameLength);
                if (!stringData[i])
                {
                    return (RwStream*)NULL;
                }

                if (!RwStreamRead(stream, stringData[i], nameLength))
                {
                    return (RwStream*)NULL;
                }
            }
            else
            {
                stringData[i] = (RwChar*)NULL;
            }
        }

        break;
    }
    default:
        return (RwStream*)NULL;
    }

    return stream;
}

static RwStream* UserDataStreamWrite(RpUserDataArray* userData, RwStream* stream)
{
    RwInt32 nameLength;
    RwInt32 i;

    if (userData->name)
    {
        nameLength = rwstrlen(userData->name) + 1;
    }
    else
    {
        nameLength = 0;
    }

    if (!RwStreamWriteInt32(stream, &nameLength, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (nameLength > 0)
    {
        if (!RwStreamWrite(stream, userData->name, nameLength))
        {
            return (RwStream*)NULL;
        }
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&userData->format, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(stream, &userData->numElements, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    switch (userData->format)
    {
    case rpINTUSERDATA:
    {
        RwInt32* intData = (RwInt32*)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (!RwStreamWriteInt32(stream, &intData[i], sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

        }

        break;
    }
    case rpREALUSERDATA:
    {
        RwReal* realData = (RwReal*)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (!RwStreamWriteReal(stream, &realData[i], sizeof(RwReal)))
            {
                return (RwStream*)NULL;
            }

        }

        break;
    }
    case rpSTRINGUSERDATA:
    {
        RwChar** stringData = (RwChar**)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (stringData[i])
            {
                nameLength = rwstrlen(stringData[i]) + 1;
            }
            else
            {
                nameLength = 0;
            }

            if (!RwStreamWriteInt32(stream, &nameLength, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (nameLength > 0)
            {
                if (!RwStreamWrite(stream, stringData[i], nameLength))
                {
                    return (RwStream*)NULL;
                }
            }
        }

        break;
    }
    default:
        return (RwStream*)NULL;
    }

    return stream;
}

static void UserDataDestruct(RpUserDataArray* userData)
{
    RwInt32 i;
    RwChar** charData;

    if (NULL != userData->name)
    {
        RwFree(userData->name);
    }

    if (userData->format == rpSTRINGUSERDATA)
    {
        charData = (RwChar**)userData->data;

        for (i = 0; i < userData->numElements; i++)
        {
            if (NULL != charData[i])
            {
                RwFree(charData[i]);
            }
        }
    }

    if (NULL != userData->data)
    {
        RwFree(userData->data);
    }
}

static void UserDataListDestruct(RpUserDataList* list)
{
    RwInt32 i;

    if (NULL != list->userData)
    {
        for (i = 0; i < list->numElements; i++)
        {
            UserDataDestruct(&list->userData[i]);
        }

        RwFree(list->userData);
    }

    list->userData = (RpUserDataArray*)NULL;
    list->numElements = 0;
}

static void UserDataCopy(RpUserDataArray* dstUserData, RpUserDataArray* srcUserData)
{
    RwInt32 dataSize;
    RwInt32 i;
    RwChar** srcCharData;
    RwChar** dstCharData;

    dstUserData->format = srcUserData->format;
    dstUserData->numElements = srcUserData->numElements;

    if (NULL != srcUserData->name)
    {
        UserDataStringDuplicate(dstUserData->name, srcUserData->name);
    }

    if (NULL != srcUserData->data)
    {
        dataSize = dstUserData->numElements * RpUserDataGetFormatSize(dstUserData->format);

        dstUserData->data = RwMalloc(dataSize);

        if (dstUserData->format == rpSTRINGUSERDATA)
        {
            srcCharData = (RwChar**)srcUserData->data;
            dstCharData = (RwChar**)dstUserData->data;

            for (i = 0; i < dstUserData->numElements; i++)
            {
                if (NULL == srcCharData[i])
                {
                    dstCharData[i] = (RwChar*)NULL;
                }
                else
                {
                    UserDataStringDuplicate(dstCharData[i], srcCharData[i]);
                }
            }
        }
        else
        {
            memcpy(dstUserData->data, srcUserData->data, dataSize);
        }
    }
}

static void UserDataListCopy(RpUserDataList* dstList, const RpUserDataList* srcList)
{
    RwInt32 i;

    UserDataListDestruct(dstList);

    dstList->numElements = srcList->numElements;

    if (dstList->numElements > 0)
    {
        dstList->userData =
            (RpUserDataArray*)RwMalloc(sizeof(RpUserDataArray) * dstList->numElements);

        for (i = 0; i < dstList->numElements; i++)
        {
            UserDataCopy(&dstList->userData[i], &srcList->userData[i]);
        }
    }
}

static RwInt32 UserDataListAddElement(RpUserDataList* list, RwChar* name, RpUserDataFormat format,
                                      RwInt32 numElements)
{
    RwInt32 index = -1;
    RwInt32 i;
    RpUserDataArray* userData;

    /* Look for a free slot */
    for (i = 0; i < list->numElements; i++)
    {
        if (!list->userData[i].data)
        {
            index = i;
        }
    }

    if (index == -1)
    {
        if (list->userData)
        {
            RpUserDataArray* newData = (RpUserDataArray*)RwMalloc(sizeof(RpUserDataArray) *
                                                                  (list->numElements + 1));
            if (!newData)
            {
                return -1;
            }

            memcpy(newData, list->userData, sizeof(RpUserDataArray) * list->numElements);
            RwFree(list->userData);
            list->userData = newData;
        }
        else
        {
            list->userData = (RpUserDataArray*)RwMalloc(sizeof(RpUserDataArray) *
                                                        (list->numElements + 1));
            if (!list->userData)
            {
                return -1;
            }
        }

        index = list->numElements;
        list->numElements++;
    }

    userData = &list->userData[index];

    userData->data = RwMalloc(numElements * RpUserDataGetFormatSize(format));
    if (!userData->data)
    {
        return -1;
    }

    UserDataStringDuplicate(userData->name, name);

    userData->format = format;
    userData->numElements = numElements;

    return index;
}

static void* UserDataObjectConstruct(void* object, RwInt32 offset, RwInt32 size)
{
    RpUserDataList* list = RPUSERDATALISTGETDATA(object, offset);

    list->numElements = 0;
    list->userData = (RpUserDataArray*)NULL;

    return object;
}

static void* UserDataObjectDestruct(void* object, RwInt32 offset, RwInt32 size)
{
    UserDataListDestruct(RPUSERDATALISTGETDATA(object, offset));

    return object;
}

static void* UserDataObjectCopy(void* dstObject, const void* srcObject, RwInt32 offset,
                                RwInt32 size)
{
    UserDataListCopy(RPUSERDATALISTGETDATA(dstObject, offset),
                     RPUSERDATALISTGETDATA(srcObject, offset));

    return dstObject;
}

static RwStream* UserDataListStreamRead(RpUserDataList* list, RwStream* stream)
{
    RwInt32 i;
    RwInt32 numElements;

    if (list)
    {
        if (!RwStreamReadInt32(stream, &numElements, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        list->numElements = numElements;
        list->userData = (RpUserDataArray*)RwMalloc(sizeof(RpUserDataArray) * list->numElements);

        for (i = 0; i < list->numElements; i++)
        {
            stream = UserDataStreamRead(&list->userData[i], stream);
        }
    }

    return stream;
}

static RwStream* UserDataObjectStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                          RwInt32 offset, RwInt32 size)
{
    RpUserDataList* userDataList = RPUSERDATALISTGETDATA(object, offset);

    UserDataListStreamRead(userDataList, stream);

    return stream;
}

static RwStream* UserDataListStreamWrite(const RpUserDataList* list, RwStream* stream)
{
    RwInt32 i;

    if (list && list->numElements > 0)
    {
        if (!RwStreamWriteInt32(stream, (RwInt32*)&list->numElements, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        for (i = 0; i < list->numElements; i++)
        {
            stream = UserDataStreamWrite(&list->userData[i], stream);
        }
    }

    return stream;
}

static RwStream* UserDataObjectStreamWrite(RwStream* stream, RwInt32 binaryLength,
                                           const void* object, RwInt32 offset, RwInt32 size)
{
    const RpUserDataList* userDataList = RPUSERDATALISTGETDATA(object, offset);

    UserDataListStreamWrite(userDataList, stream);

    return stream;
}

static RwInt32 UserDataStreamGetSize(const RpUserDataArray* userData)
{
    RwInt32 i;
    RwInt32 length;
    RwInt32 size = 0;
    RwChar** charData;

    if (userData)
    {
        size = sizeof(RwInt32);

        if (userData->name)
        {
            size += rwstrlen(userData->name) + 1;
        }

        size += sizeof(RwInt32) + sizeof(RwInt32);

        switch (userData->format)
        {
        case rpINTUSERDATA:
            size += sizeof(RwInt32) * userData->numElements;
            break;
        case rpREALUSERDATA:
            size += sizeof(RwReal) * userData->numElements;
            break;
        case rpSTRINGUSERDATA:
        {
            charData = (RwChar**)userData->data;

            for (i = 0; i < userData->numElements; i++)
            {
                size += sizeof(RwInt32);

                if (charData[i])
                {
                    length = rwstrlen(charData[i]) + 1;
                    size += length;
                }
            }

            break;
        }
        }
    }

    return size;
}

static RwInt32 UserDataListGetSize(const RpUserDataList* list)
{
    RwInt32 i;
    RwInt32 size;

    size = 0;

    if (list && list->numElements > 0)
    {
        size += sizeof(RwInt32);

        for (i = 0; i < list->numElements; i++)
        {
            size += UserDataStreamGetSize(&list->userData[i]);
        }
    }

    return size;
}

static RwInt32 UserDataObjectGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpUserDataList* userDataList = RPUSERDATALISTGETDATA(object, offset);

    return UserDataListGetSize(userDataList);
}

RwInt32 RpGeometryAddUserDataArray(RpGeometry* geometry, RwChar* name, RpUserDataFormat format,
                                   RwInt32 numElements)
{
    return UserDataListAddElement(RPUSERDATALISTGETDATA(geometry, userDataGeometryOffset), name,
                                  format, numElements);
}

static RwInt32 UserDataListGetNumElements(const RpUserDataList* list)
{
    RwInt32 numElements = 0;
    RwInt32 i;

    for (i = 0; i < list->numElements; i++)
    {
        if (list->userData[i].data)
        {
            numElements++;
        }
    }

    return numElements;
}

RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* geometry)
{
    const RpUserDataList* userDataList = RPUSERDATALISTGETDATA(geometry, userDataGeometryOffset);

    return UserDataListGetNumElements(userDataList);
}

RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, RwInt32 data)
{
    const RpUserDataList* list = RPUSERDATALISTGETDATA(geometry, userDataGeometryOffset);

    if (data < list->numElements)
    {
        return &list->userData[data];
    }

    return (RpUserDataArray*)NULL;
}

RwInt32 RpUserDataGetFormatSize(RpUserDataFormat format)
{
    switch (format)
    {
    case rpINTUSERDATA:
        return sizeof(RwInt32);
    case rpREALUSERDATA:
        return sizeof(RwReal);
    case rpSTRINGUSERDATA:
        return sizeof(RwChar*);
    default:
        return 0;
    }
}

RwBool RpUserDataPluginAttach(void)
{
    if (RwEngineRegisterPlugin(0, rwID_USERDATAPLUGIN, UserDataOpen, UserDataClose) < 0)
    {
        return FALSE;
    }

    /* Geometry */
    userDataGeometryOffset =
        RpGeometryRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                                 UserDataObjectConstruct, UserDataObjectDestruct,
                                 UserDataObjectCopy);
    if (userDataGeometryOffset < 0)
    {
        return FALSE;
    }

    userDataGeometryStreamOffset = RpGeometryRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite,
        UserDataObjectGetSize);
    if (userDataGeometryStreamOffset < 0)
    {
        return FALSE;
    }

    /* World sector */
    userDataWorldSectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                                    UserDataObjectConstruct, UserDataObjectDestruct,
                                    UserDataObjectCopy);
    if (userDataGeometryOffset < 0)
    {
        return FALSE;
    }

    userDataWorldSectorStreamOffset = RpWorldSectorRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite,
        UserDataObjectGetSize);
    if (userDataGeometryStreamOffset < 0)
    {
        return FALSE;
    }

    /* Frame */
    userDataFrameOffset =
        RwFrameRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                              UserDataObjectConstruct, UserDataObjectDestruct, UserDataObjectCopy);
    if (userDataFrameOffset < 0)
    {
        return FALSE;
    }

    userDataFrameStreamOffset =
        RwFrameRegisterPluginStream(rwID_USERDATAPLUGIN, UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (userDataFrameStreamOffset < 0)
    {
        return FALSE;
    }

    /* Camera */
    userDataCameraOffset =
        RwCameraRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                               UserDataObjectConstruct, UserDataObjectDestruct, UserDataObjectCopy);
    if (userDataCameraOffset < 0)
    {
        return FALSE;
    }

    userDataCameraStreamOffset =
        RwCameraRegisterPluginStream(rwID_USERDATAPLUGIN, UserDataObjectStreamRead,
                                     UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (userDataCameraStreamOffset < 0)
    {
        return FALSE;
    }

    /* Light */
    userDataLightOffset =
        RpLightRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                              UserDataObjectConstruct, UserDataObjectDestruct, UserDataObjectCopy);
    if (userDataLightOffset < 0)
    {
        return FALSE;
    }

    userDataLightStreamOffset =
        RpLightRegisterPluginStream(rwID_USERDATAPLUGIN, UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (userDataLightStreamOffset < 0)
    {
        return FALSE;
    }

    /* Material */
    userDataMaterialOffset =
        RpMaterialRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                                 UserDataObjectConstruct, UserDataObjectDestruct,
                                 UserDataObjectCopy);
    if (userDataMaterialOffset < 0)
    {
        return FALSE;
    }

    userDataMaterialStreamOffset =
        RpMaterialRegisterPluginStream(rwID_USERDATAPLUGIN, UserDataObjectStreamRead,
                                       UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (userDataMaterialStreamOffset < 0)
    {
        return FALSE;
    }

    /* Texture */
    userDataTextureOffset =
        RwTextureRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct, UserDataObjectDestruct,
                                UserDataObjectCopy);
    if (userDataTextureOffset < 0)
    {
        return FALSE;
    }

    userDataTextureStreamOffset =
        RwTextureRegisterPluginStream(rwID_USERDATAPLUGIN, UserDataObjectStreamRead,
                                      UserDataObjectStreamWrite, UserDataObjectGetSize);

    return userDataTextureStreamOffset >= 0;
}
