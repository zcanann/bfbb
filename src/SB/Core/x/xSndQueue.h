#ifndef XSNDQUEUE_H
#define XSNDQUEUE_H

#include "xSnd.h"

// Include in the translation unit that owns the sound queue implementation.

template <S32 N> inline sound_queue<N>::sound_queue()
{
    head = 0;
    tail = 0;
}

template <S32 N>
void sound_queue<N>::play(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, U32 parentID,
                          sound_category snd_category)
{
    U32 assetID = xSndPlay(id, vol, pitch, priority, flags, parentID, snd_category, 0.0f);

    push(assetID);
}

template <S32 N> void sound_queue<N>::push(U32 id)
{
    _playing[tail] = id;

    S32 h = head;
    S32 t = tail + 1;

    if (t <= h)
    {
        t += (N + 1);
    }

    if (t - h > N)
    {
        xSndStop(_playing[h]);
        head = (h + 1) % (N + 1);
    }

    tail = t % (N + 1);
}

template <S32 N> inline U32 sound_queue<N>::recent(S32 index) const
{
    S32 i = tail - index - 1;
    if (i < 0)
    {
        i += (N + 1);
    }
    return _playing[i];
}

template <S32 N> inline bool sound_queue<N>::playing(S32 index, bool streaming) const
{
    S32 count = size();
    S32 i;

    if (index < 0 || index > count)
    {
        index = count;
    }

    if (streaming)
    {
        for (i = 0; i < index; i++)
        {
            if (!xSndIsPlayingByHandle(recent(i)))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        for (i = 0; i < index; i++)
        {
            if (xSndIsPlayingByHandle(recent(i)))
            {
                return true;
            }
        }
        return false;
    }
}

#endif
