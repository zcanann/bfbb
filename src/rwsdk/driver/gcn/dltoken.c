#include <rwsdk/rwcore.h>
#include <dolphin/gx.h>

volatile RwUInt16 _RwDlTokenCurrent = 1;
volatile RwUInt16 _RwDlTokenLastSeen;

RwBool _rwDlTokenQueryDone(RwUInt16 token)
{
    _RwDlTokenLastSeen = GXReadDrawSync();

    if (_RwDlTokenLastSeen >= 0xE000)
    {
        return FALSE;
    }

    if (_RwDlTokenCurrent >= _RwDlTokenLastSeen)
    {
        return ((token <= _RwDlTokenLastSeen) || (token > _RwDlTokenCurrent));
    }
    else
    {
        return ((token > _RwDlTokenCurrent) && (token <= _RwDlTokenLastSeen));
    }
}
