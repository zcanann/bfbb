#include "zNPCTypeKingJelly.h"

#include <types.h>
#include "xMathInlines.h"
#include "xColor.h"
#include "zNPCGoalCommon.h"
#include "zCamera.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zNPCSndLists.h"
#include "xDebug.h"

typedef void (*tweak_change_cb)(tweak_info&);
#include "zMusic.h"
#include "xGroup.h"
#include "xutil.h"
#include "string.h"
#include "stdlib.h"

#define ANIM_Unknown 0 // 0x0
#define ANIM_Idle01 1 // 0x04
#define ANIM_Idle02 2 // 0x08
#define ANIM_Idle03 3 // 0xC
#define ANIM_Fidget01 4 //
#define ANIM_Fidget02 5
#define ANIM_Fidget03 6
#define ANIM_Taunt01 7 // 0x1c
#define ANIM_Attack01 8 //0x20
#define ANIM_Damage01 9 //0x24
#define ANIM_Damage02 10 //0x28
#define ANIM_Death01 11 //0x2c
#define ANIM_AttackWindup01 12 //0x30
#define ANIM_AttackLoop01 13 //0x34
#define ANIM_AttackEnd01 14 //0x38
#define ANIM_SpawnKids01 15 //0x3C
#define ANIM_Attack02Windup01 16
#define ANIM_Attack02Loop01 17
#define ANIM_Attack02End01 18
#define ANIM_LassoGrab01 19

#define LYT_TYPE_LINE 0
#define LYT_TYPE_ROTATING 1

#define SOUND_AMBIENT_RING 0
#define SOUND_BIRTH 1
#define SOUND_CHARGE 2
#define SOUND_CHEER 3
#define SOUND_GRUNT 4
#define SOUND_LAND 5
#define SOUND_MOVE 6
#define SOUND_OSCILLATE 7
#define SOUND_RISE 8
#define SOUND_TAUNT 9
#define SOUND_WAVE_RING 10

namespace auto_tweak
{
    template <>
    inline void load_param<iColor_tag, S32>(iColor_tag& value, S32 scale, S32 lo, S32 hi,
                                     xModelAssetParam* ap, U32 apsize, const char* name)
    {
        F32 def[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        F32 result[4];

        def[0] = value.r;
        def[1] = value.g;
        def[2] = value.b;
        def[3] = value.a;

        zParamGetFloatList(ap, apsize, name, 4, def, result);

        value.r = result[0];
        value.g = result[1];
        value.b = result[2];
        value.a = result[3];
    }

    template <>
    inline void load_param<bool, S32>(bool& value, S32 scale, S32 lo, S32 hi, xModelAssetParam* ap,
                               U32 apsize, const char* name)
    {
        value = zParamGetInt(ap, apsize, name, value);
    }

    template <>
    inline void load_param<F32, F32>(F32& value, F32 scale, F32 lo, F32 hi, xModelAssetParam* ap,
                              U32 apsize, const char* name)
    {
        value = zParamGetFloat(ap, apsize, name, value);

        if (value < lo)
        {
            value = lo;
        }
        else if (value > hi)
        {
            value = hi;
        }

        value = value * scale;
    }

    template <>
    inline void load_param<S32, S32>(S32& value, S32 scale, S32 lo, S32 hi, xModelAssetParam* ap,
                              U32 apsize, const char* name)
    {
        S32 v = zParamGetInt(ap, apsize, name, value);

        if (v < lo)
        {
            v = lo;
        }
        else if (v > hi)
        {
            v = hi;
        }

        v = v * scale;
        value = v;
    }
} // namespace auto_tweak

namespace
{
    struct tweak_group
    {
        void* context;
        tweak_callback* cb_fade_obstructions;
        tweak_callback* cb_ambient_ring;
        S32 max_life;
        F32 min_dist;
        F32 move_radius;
        F32 vel_decay;
        F32 repel_radius;
        F32 repel_radius_ground;
        F32 fade_obstructions;
        F32 music_fade;
        F32 music_fade_delay;
        struct
        {
            F32 duration;
            S32 amount;
            F32 drop_off;
            struct
            {
                F32 r;
                F32 g;
                F32 b;
                F32 a;
            } color;
        } blink;
        struct
        {
            F32 variance;
            F32 attack[3];
            F32 warm_up;
            F32 release;
            F32 cool_down;
        } interval;
        struct
        {
            S32 cycles;
            F32 voffset;
            F32 hoffset;
            F32 delay;
            F32 fall_time;
            struct
            {
                F32 speed;
                F32 drop_off;
                F32 delay;
                F32 voffset;
            } spew;
        } spawn;
        wave_ring_type wave_ring;
        struct
        {
            F32 radius;
            F32 min_height;
            F32 max_height;
            F32 speed;
            F32 segment_length;
            F32 thickness;
            iColor_tag color;
            F32 knock_back;
            struct
            {
                F32 radius;
                F32 max_height;
                F32 speed;
                F32 thickness;
                iColor_tag color;
            } charge;
        } ambient_ring;
        struct
        {
            F32 thickness;
            F32 rand_radius;
            F32 rot_radius;
            F32 move_degrees;
            iColor_tag color;
            F32 delay;
            F32 time;
            S32 max;
            F32 particles;
            F32 knock_back;
            F32 damage_width;
            struct
            {
                F32 thickness;
                iColor_tag color;
                F32 move_degrees;
            } charge;
        } tentacle;
        struct
        {
            F32 delay;
            S32 rings;
            F32 voffset;
            F32 particles;
            F32 radius;
            F32 width;
            F32 vel;
            F32 particle_drop_off;
            F32 vel_drop_off;
        } thump;
        struct
        {
            F32 volume;
            F32 delay;
            F32 radius_inner;
            F32 radius_outer;
            S32 priority;
        } sound[11];

        void load(xModelAssetParam* ap, U32 apsize);
        void register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*);
    };

    struct sound_data_type 
    {
        U32 id[2]; // offset 0x0, size 0x8
        U8 delayed; // offset 0x8, size 0x1
        S8 amount; // offset 0x9, size 0x1
        S8 playing; // offset 0xA, size 0x1
        F32 time; // offset 0xC, size 0x4
        U32 handle; // offset 0x10, size 0x4
        xVec3 * loc; // offset 0x14, size 0x4
    };

    static tweak_group tweak;
    static sound_data_type sound_data[11];
    static zParEmitter* spawn_emitter;
    static xParEmitterCustomSettings spawn_emitter_settings;
    static zParEmitter* zap_emitter;
    static xParEmitterCustomSettings zap_emitter_settings;
    static zParEmitter* shock_ring_emitter;
    static xParEmitterCustomSettings shock_ring_emitter_settings;
    static zParEmitter* thump_ring_emitter;
    static xParEmitterCustomSettings thump_ring_emitter_settings;
    static xVec3 ring_segments[64];
    
    static char* sound_name[11][3] = {
        {
            "KJ_pulseupdown",
            NULL,
            NULL
        },
        {
            "KJ_grunt",
            NULL,
            NULL
        },
        {
            "KJ_Charge",
            NULL,
            NULL
        },
        {
            "KJ_Cheer",
            NULL,
            NULL
        },
        {
            "KJ_grunt",
            NULL,
            NULL
        },
        {
            "KJ_Land1",
            "KJ_Land2",
            NULL
        },
        {
            "KJ_Mov",
            NULL,
            NULL
        },
        {
            "KJ_Osc",
            NULL,
            NULL
        },
        {
            "KJ_rise",
            NULL,
            NULL
        },
        {
            "KJ_Taunt",
            NULL,
            NULL
        },
        {
            "KJ_Pulse",
            NULL,
            NULL
        },
    };

    static const U8 sound_flags[11] = { 0x1, 0x0, 0x0, 0x0, 0x0,
                                        0x0, 0x1, 0x1, 0x0, 0x0, 0x0};

    static const U8 tentacle_bone[7][4] = {
        { 0x14, 0x15, 0x16, 0x17 }, { 0x10, 0x11, 0x12, 0x13 }, { 0x1D, 0x1E, 0x1F, 0x20 },
        { 0x05, 0x08, 0x09, 0x0B }, { 0x21, 0x24, 0x25, 0x27 }, { 0x0C, 0x0D, 0x0E, 0x0F },
        { 0x18, 0x19, 0x1A, 0x1C },
    };

    static xBinaryCamera boss_cam = { { { 8.0f, 4.0f, 3.0f },
                                        { 0.2f, 2.2f, -1.0f },
                                        { 1.0f, 0.2f, 1.5f },
                                        10.0f,
                                        10.0f,
                                        10.0f,
                                        10.0f,
                                        50.0f,
                                        0.0f } };
}

namespace
{
    void init_sound()
    {
        memset(sound_data, NULL, sizeof(sound_data));

        for (S32 i = 0; i < 11; i++)
        {
            for (S32 j = 0; j < 2; j++)
            {
                if (sound_name[i][j] == NULL)
                {
                    break;
                }

                sound_data[i].id[j] = xStrHash(sound_name[i][j]);
                sound_data[i].amount++;
            }
            
            sound_data[i].playing = -1;
        }
    }

    void play_sound_immediate(S32 sound_index, const xVec3* pos)
    {
        sound_data_type& sound = sound_data[sound_index];

        if (sound.handle != 0 && sound_flags[sound_index] & 0x1)
        {
            return;
        }

        sound.playing = 0;
        sound.delayed = FALSE;

        if (sound.amount > 1)
        {
            sound.playing = (xrand() >> 13) % sound.amount;
        }

        sound.handle = xSndPlay3D(
            sound.id[sound.playing],
            tweak.sound[sound_index].volume,
            1.0f,
            tweak.sound[sound_index].priority,
            0x0,
            pos,
            tweak.sound[sound_index].radius_inner,
            tweak.sound[sound_index].radius_outer,
            SND_CAT_GAME,
            0.0f
        );
    }

    void play_sound(S32 sound_index, const xVec3* pos)
    {
        if (tweak.sound[sound_index].delay <= 0.0f)
        {
            play_sound_immediate(sound_index, pos);
            return;
        }

        sound_data_type& sound = sound_data[sound_index];
        if (sound.handle != 0 && sound_flags[sound_index] & 0x1)
        {
            return;
        }
     
        sound.delayed = TRUE;
        sound.time = 0.0f;
    }

    void sound_update(F32 dt)
    {
        for (S32 i = 0; i < 11; i++)
        {
            sound_data_type& sound = sound_data[i];

            if (sound.delayed == FALSE)
            {
                continue;
            }

            sound.time += dt;

            if (sound.time >= tweak.sound[i].delay)
            {
                play_sound_immediate(i, sound.loc);
            }
        }
    }

} // namespace

namespace
{
    static const S32 bored_anims[2] = { ANIM_Idle02, ANIM_Idle03 };

    S32 set_ring_segments(const xVec3& center, F32 radius, F32 segment_length)
    {
        static U8 sclookup_inited = FALSE;
        static F32 sin_lookup[9];
        static F32 cos_lookup[9];

        if (!sclookup_inited)
        {
            sclookup_inited = TRUE;

            F32 angle = 0.0f;
            for (S32 i = 0; i < 9; i++)
            {
                sin_lookup[i] = isin(angle);
                cos_lookup[i] = icos(angle);

                angle += PI / 32.0f;
            }
        }

        S32 size = 0.99999f + ((2.0f * radius * PI) / segment_length);
        size = (size + 7) & ~7;

        if (size > 64)
        {
            size = 64;
        }
        if (size <= 0)
        {
            return 0;
        }

        xVec3* it = ring_segments;
        xVec3* end = &ring_segments[size / 8] + 1;
        S32 step = 64 / size;
        const F32* sin_it = sin_lookup;
        const F32* cos_it = cos_lookup;
        xVec3* dst;

        // First octant, straight out of the lookup tables.
        for (; it != end; it++)
        {
            it->x = radius * *sin_it;
            it->z = radius * *cos_it;
            sin_it += step;
            cos_it += step;
        }

        // Second octant: mirror about the 45 degree line.
        it = ring_segments;
        dst = it + size / 4;
        for (; it != &ring_segments[size / 8]; it++, dst--)
        {
            dst->x = it->z;
            dst->z = it->x;
        }

        // Second quadrant: mirror about the x axis.
        it = ring_segments;
        dst = it + size / 2;
        for (; it != &ring_segments[size / 4]; it++, dst--)
        {
            dst->x = it->x;
            dst->z = -it->z;
        }

        // Lower half: mirror about the z axis.
        it = &ring_segments[1];
        end = it + size / 2 - 1;
        dst = &ring_segments[size] - 1;
        for (; it != end; it++, dst--)
        {
            dst->x = -it->x;
            dst->z = it->z;
        }

        F32 cz;
        F32 cy;
        F32 cx;
        cx = center.x;
        cy = center.y;
        cz = center.z;

        for (it = ring_segments; it != &ring_segments[size]; it++)
        {
            it->x += cx;
            it->y = cy;
            it->z += cz;
        }

        return size;
    }

    void updown_ring_update(lightning_ring& ring, F32 dt)
    {
        ring.current.time += ring.current.accel * dt;
        ring.current.time = xfmod(ring.current.time, ring.delay);

        const F32 t = ring.current.time / ring.delay;
        const F32 range = ring.max_height - ring.min_height;
        ring.current.height =
            ring.min_height + 0.5f * (range * (1.0f + isin((2.0f * PI) * t)));
    }

    void expand_ring_update(lightning_ring& ring, F32 dt)
    {
        ring.current.vel += ring.current.accel * dt;
        if (ring.current.vel > ring.max_vel)
        {
            ring.current.vel = ring.max_vel;
        }

        ring.current.radius += ring.current.vel * dt;
        if (ring.current.radius > ring.max_height)
        {
            ring.current.time += dt;

            const F32 t = ring.current.time / ring.delay;
            const F32 frac = 1.0f - t;
            if (frac < 0.0f)
            {
                ring.property.color.a = 0;
            }
            else
            {
                ring.property.color.a = 255.0f * frac + 0.5f;
            }
        }
    }
    
    void kill_sound(S32 sound_index)
    {
        sound_data_type& sound = sound_data[sound_index];

        if (sound.handle != 0)
        {
            xSndStop(sound.handle);
            sound.handle = 0;
            sound.playing = -1;
            sound.delayed = FALSE;
        }
    }

    void kill_sounds()
    {
        for (S32 i = 0; i < 11; i++)
        {
            kill_sound(i);
        }
    }

} // namespace

void lightning_ring::create()
{
    // store 1 into 0x0
    active = 1;
    arcs_size = 0;

    //store 0 into 0x7c
}

void lightning_ring::destroy()
{
    for (S32 i = 0; i < arcs_size; i++)
    {
        zLightningKill(arcs[i]);
    }
    arcs_size = 0;
    active = 0;
}

void lightning_ring::refresh()
{
    if (!active)
    {
        return;
    }

    xVec3 center = this->center;
    center.y += current.height;

    S32 size = set_ring_segments(center, current.radius, segment_length);

    S32 current_size = 0;
    for (U32 i = 0; i < arcs_size; i++)
    {
        current_size += arcs[i]->legacy.total_points - 1;
    }

    if (current_size == size)
    {
        arcs_size = 0;

        for (S32 i = 0; i < size;)
        {
            zLightning& arc = *arcs[arcs_size];

            S32 points = 16;
            if (i + 16 > size)
            {
                points = size - i + 1;
            }

            zLightningModifyEndpoints(&arc, &ring_segments[i],
                                      &ring_segments[(i + points - 1) % size]);

            F32* it = arc.legacy.thickness;
            F32* end = it + points;
            for (; it != end; it++)
            {
                *it = property.thickness;
            }

            arc.color = property.color;

            i += points - 1;
            arcs_size++;
        }
    }
    else
    {
        for (U32 i = 0; i < arcs_size; i++)
        {
            zLightningKill(arcs[i]);
        }
        arcs_size = 0;

        for (S32 i = 0; i < size;)
        {
            S32 points = 16;
            if (i + 16 > size)
            {
                points = size - i + 1;
            }

            arcs[arcs_size] =
                create_arc(&ring_segments[i], &ring_segments[(i + points - 1) % size], points - 1, 1);
            if (arcs[arcs_size] == NULL)
            {
                return;
            }

            arcs_size++;
            i += points - 1;
        }
    }
}

zLightning* lightning_ring::create_arc(xVec3* start, xVec3* end, int points, int end_points)
{
    _tagLightningAdd add;

    add.type = property.line ? LYT_TYPE_LINE : LYT_TYPE_ROTATING;
    add.total_points = points + end_points;
    add.end_points = end_points;
    add.thickness = property.thickness;
    add.color = property.color;
    add.rand_radius = 1.0f;
    add.arc_height = 0.0f;
    add.rot_radius = property.rot_radius;
    add.time = 100000.0f;
    add.flags = 0x2ba;
    add.setup_degrees = 360.0f * xurand();
    add.move_degrees = property.degrees * add.total_points * (0.75f + 0.5f * xurand());
    add.start = start;
    add.end = end;

    return zLightningAdd(&add);
}

xAnimTable* ZNPC_AnimTable_KingJelly()
{
    // clang-format off
    S32 ourAnims[11] = {
        ANIM_Idle01,
        ANIM_Idle02,
        ANIM_Idle03,
        ANIM_Taunt01,
        ANIM_Attack01,
        ANIM_AttackWindup01,        
        ANIM_AttackLoop01,
        ANIM_AttackEnd01,
        ANIM_Damage01,
        ANIM_SpawnKids01,
        ANIM_Unknown,
        
    };
    // clang-format on
    xAnimTable* table = xAnimTableNew("zNPCKingJelly", NULL, 0);

    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle02], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle03], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Taunt01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Attack01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackWindup01], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackLoop01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackEnd01], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Damage01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_SpawnKids01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_subbanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Attack01], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Attack01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01],
                            g_strz_subbanim[ANIM_AttackLoop01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_AttackEnd01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle02], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle03], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Taunt01], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01], g_strz_subbanim[ANIM_Damage01],
                            0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackEnd01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_SpawnKids01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle02], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle03], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Taunt01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Taunt01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackEnd01], g_strz_subbanim[ANIM_Taunt01],
                            0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_SpawnKids01], g_strz_subbanim[ANIM_Taunt01],
                            0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Damage01],
                            g_strz_subbanim[ANIM_SpawnKids01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);

    return table;
}

zNPCKingJelly::zNPCKingJelly(S32 myType) : zNPCSubBoss(myType)
{
    this->show_vertex = -1;
    this->enabled = TRUE;
    memset(&this->tentacle_lightning, 0, 7 * sizeof(zLightning*));
    init_sound();
}

void zNPCKingJelly::Init(xEntAsset* asset)
{
    zNPCCommon::Init(asset);
    flags1.flg_basenpc |= 0x10;
    memset(&flag.fighting, 0, 5);
    boss_cam.init();
}

void zNPCKingJelly::Setup()
{
    this->children_size = 0; //0x88C
    load_model();
    load_curtain_model();
    zNPCSubBoss::Setup();
}

void zNPCKingJelly::Reset()
{
    zNPCCommon::Reset();

    if (!flag.died)
    {
        decompose();
        post_decompose();
        reappear();
    }

    memset(&flag.fighting, 0, 5);

    round = 0;
    attack = 0;
    life = tweak.max_life;
    player_life = globals.player.Health;
    last_tentacle_shock = 0.0f;
    disable_tentacle_damage = 0;
    first_update = 1;
    blink.active = 0;

    show_shower_model();

    spawn_emitter = zParEmitterFind("PAREMIT_KJ_SPAWN");
    spawn_emitter_settings.custom_flags = 0x110;
    spawn_emitter_settings.pos = g_O3;
    spawn_emitter_settings.rate.val[0] = 0.0f;
    spawn_emitter_settings.rate.interp = 0;
    spawn_emitter_settings.rate.oofreq = 1.0f;
    spawn_emitter_settings.rate.freq = 1.0f;
    spawn_particle_vel = 0.0f;
    spawn_emitter_settings.rate.val[0] = 0.0f;

    zap_emitter = zParEmitterFind("PAREMIT_KJ_ZAP");
    zap_emitter_settings.custom_flags = 0x100;
    zap_emitter_settings.pos = g_O3;

    shock_ring_emitter = zParEmitterFind("PAREMIT_KJ_SHOCK_RING");
    shock_ring_emitter_settings.custom_flags = 0x110;
    shock_ring_emitter_settings.pos = g_O3;
    shock_ring_emitter_settings.rate.val[0] = 59.999996f;
    shock_ring_emitter_settings.rate.interp = 0;
    shock_ring_emitter_settings.rate.oofreq = 1.0f;
    shock_ring_emitter_settings.rate.freq = 1.0f;

    thump_ring_emitter = zParEmitterFind("PAREMIT_KJ_THUMP_RING");
    thump_ring_emitter_settings.custom_flags = 0x1310;
    thump_ring_emitter_settings.pos = g_O3;
    thump_ring_emitter_settings.rate.val[0] = 59.999996f;
    thump_ring_emitter_settings.rate.interp = 0;
    thump_ring_emitter_settings.rate.oofreq = 1.0f;
    thump_ring_emitter_settings.rate.freq = 1.0f;
    thump_ring_emitter_settings.vel.assign(0.0f, 1.0f, 0.0f);
    thump_ring_emitter_settings.radius = 1.0f;

    create_ambient_rings();
    create_tentacle_lightning();

    for (U32 i = 0; i < children_size; i++)
    {
        ((zNPCJelly*)children[i].npc)->MeetTheKing(this);
        disable_child(children[i]);
    }

    entShadow->radius[xEntShadow::RADIUS_CACHE] = 9.0f;
    entShadow->radius[xEntShadow::RADIUS_RASTER] = 9.1f;
    entShadow->dst_cast = 4.5f;

    psy_instinct->GoalSet(NPC_GOAL_KJIDLE, GOAL_STAT_PROCESS);
}

void zNPCKingJelly::Destroy()
{
    decompose();
    post_decompose();
    zNPCCommon::Destroy();
}

void zNPCKingJelly::Process(xScene* xscn, F32 dt)
{
    if (!flag.updated)
    {
        xEnt* tub = (xEnt*)zSceneFindObject(xStrHash("TUB_WATER_SIMP"));
        if (tub != NULL)
        {
            tub->baseFlags |= 0x10;
        }

        flag.updated = true;
    }

    if (psy_instinct->GIDOfActive() == NPC_GOAL_LIMBO)
    {
        zNPCCommon::Process(xscn, dt);
        return;
    }

    if (first_update)
    {
        first_update = false;
    }

    sound_update(dt);

    if (flag.died || !flag.fighting)
    {
        zNPCCommon::Process(xscn, dt);
        return;
    }

    if (flag.stop_moving || !enabled)
    {
        xVec3& vel = frame->vel;

        vel *= tweak.vel_decay;

        if (vel.length2() < 0.001f)
        {
            vel = 0.0f;
            flag.stop_moving = false;
        }
    }

    if (enabled)
    {
        psy_instinct->Timestep(dt, NULL);
    }

    if (flag.died || !flag.fighting)
    {
        zNPCCommon::Process(xscn, dt);
        return;
    }

    update_camera(dt);
    update_rings(dt);
    update_tentacle_lightning(dt);
    update_spawn_particles(dt);
    update_blink(dt);
    repel_player();
    check_player_damage();

    if (globals.player.Health < player_life)
    {
        taunt();
    }

    player_life = globals.player.Health;

    zNPCCommon::Process(xscn, dt);
}

void zNPCKingJelly::BUpdate(xVec3* pos)
{
    xVec3& subloc = (xVec3&)model->Mat[2].pos;
    xVec3 loc = *pos + subloc;

    zNPCCommon::BUpdate(&loc);
}

S32 zNPCKingJelly::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                            xBase* toParamWidget, S32* handled)
{
    switch ((S32)toEvent)
    {
    case eEventNPCFightOn:
        start_fight();
        break;
    case eEventNPCKillQuietly:
        break;
    case eEventNPCSetActiveOff:
        psy_instinct->GoalSet(NPC_GOAL_KJDEATH, GOAL_STAT_PROCESS);
        break;
    default:
        *handled = 0;
        return zNPCCommon::SysEvent(from, to, toEvent, toParam, toParamWidget, handled);
    }

    return 1;
}

void zNPCKingJelly::RenderExtra()
{
    zNPCKingJelly::render_debug();
}

void zNPCKingJelly::ParseINI()
{
    zNPCCommon::ParseINI();

    cfg_npc->snd_traxShare = g_sndTrax_KingJelly;
    NPCS_SndTablePrepare(g_sndTrax_KingJelly);
    cfg_npc->snd_trax = g_sndTrax_KingJelly;
    NPCS_SndTablePrepare(g_sndTrax_KingJelly);

    static tweak_callback cb_fade_obstructions = { (tweak_change_cb)on_change_fade_obstructions };
    static tweak_callback cb_ambient_ring = { (tweak_change_cb)on_change_ambient_ring };

    tweak.context = this;
    tweak.cb_fade_obstructions = &cb_fade_obstructions;
    tweak.cb_ambient_ring = &cb_ambient_ring;
    tweak.load(parmdata, pdatsize);
}

namespace
{
    void tweak_group::load(xModelAssetParam* ap, U32 apsize)
    {
        tweak_group::register_tweaks(TRUE, ap, apsize, NULL);
    }

    void tweak_group::register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*)
    {
        if (init)
        {
            max_life = 3;
            auto_tweak::load_param<S32, S32>(max_life, 1, 1, 10000000, ap, apsize, "max_life");
        }
        if (init)
        {
            min_dist = 4.0f;
            auto_tweak::load_param<F32, F32>(min_dist, 1.0f, 0.0f, 10.0f, ap, apsize, "min_dist");
        }
        if (init)
        {
            move_radius = 13.0f;
            auto_tweak::load_param<F32, F32>(move_radius, 1.0, 0.0f, 20.f, ap, apsize,
                                             "move_radius");
        }
        if (init)
        {
            vel_decay = 0.8f;
            auto_tweak::load_param<F32, F32>(vel_decay, 1.0f, 0.0f, 1.0f, ap, apsize, "vel_decay");
        }
        if (init)
        {
            repel_radius = 1.8f;
            auto_tweak::load_param<F32, F32>(repel_radius, 1.0f, 0.0f, 1000.0f, ap, apsize,
                                             "repel_radius");
        }
        if (init)
        {
            repel_radius_ground = 3.2f;
            auto_tweak::load_param<F32, F32>(repel_radius_ground, 1.0, 0.0f, 1000.f, ap, apsize,
                                             "repel_radius_ground");
        }
        if (init)
        {
            fade_obstructions = 0.4f;
            auto_tweak::load_param<F32, F32>(fade_obstructions, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "fade_obstructions");
        }
        if (init)
        {
            music_fade = 0.5f;
            auto_tweak::load_param<F32, F32>(music_fade, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "music_fade");
        }
        if (init)
        {
            music_fade_delay = 1.0f;
            auto_tweak::load_param<F32, F32>(music_fade_delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "music_fade_delay");
        }
        if (init)
        {
            blink.duration = 2.0f;
            auto_tweak::load_param<F32, F32>(blink.duration, 1.0f, 0.1, 100.f, ap, apsize,
                                             "blink.duration");
        }
        if (init)
        {
            blink.amount = 4;
            auto_tweak::load_param<S32, S32>(blink.amount, 1, 1, 100, ap, apsize, "blink.amount");
        }
        if (init)
        {
            blink.drop_off = 0.2f;
            auto_tweak::load_param<F32, F32>(blink.drop_off, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "blink.drop_off");
        }
        if (init)
        {
            blink.color.r = 2.0f;
            auto_tweak::load_param<F32, F32>(blink.color.r, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "blink.color.r");
        }
        if (init)
        {
            blink.color.g = 0.0f;
            auto_tweak::load_param<F32, F32>(blink.color.g, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "blink.color.g");
        }
        if (init)
        {
            blink.color.b = 0.0f;
            auto_tweak::load_param<F32, F32>(blink.color.b, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "blink.color.b");
        }
        if (init)
        {
            blink.color.a = 1.0f;
            auto_tweak::load_param<F32, F32>(blink.color.a, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "blink.color.a");
        }
        if (init)
        {
            interval.variance = 0.2;
            auto_tweak::load_param<F32, F32>(interval.variance, 1.0f, 0.0, 10.f, ap, apsize,
                                             "interval.variance");
        }
        if (init)
        {
            interval.attack[0] = 4.5f;
            auto_tweak::load_param<F32, F32>(interval.attack[0], 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "interval.attack[0]");
        }
        if (init)
        {
            interval.attack[1] = 3.5f;
            auto_tweak::load_param<F32, F32>(interval.attack[1], 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "interval.attack[1]");
        }
        if (init)
        {
            interval.attack[2] = 2.5f;
            auto_tweak::load_param<F32, F32>(interval.attack[2], 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "interval.attack[2]");
        }
        if (init)
        {
            interval.warm_up = 1.0f;
            auto_tweak::load_param<F32, F32>(interval.warm_up, 1.0f, 0.0f, 1000000000.0, ap, apsize,
                                             "interval.warm_up");
            ;
        }
        if (init)
        {
            interval.release = 0.5f;
            auto_tweak::load_param<F32, F32>(interval.release, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "interval.release");
        }
        if (init)
        {
            interval.cool_down = 0.25;
            auto_tweak::load_param<F32, F32>(interval.cool_down, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "interval.cool_down");
        }
        if (init)
        {
            spawn.cycles = 3;
            auto_tweak::load_param<S32, S32>(spawn.cycles, 1, 0, 100000, ap, apsize,
                                             "spawn.cycles");
        }
        if (init)
        {
            spawn.voffset = 3.0f;
            auto_tweak::load_param<F32, F32>(spawn.voffset, 1.0f, -100.0f, 100.f, ap, apsize,
                                             "spawn.voffset");
        }
        if (init)
        {
            spawn.hoffset = 0.5f;
            auto_tweak::load_param<F32, F32>(spawn.hoffset, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "spawn.hoffset");
        }
        if (init)
        {
            spawn.delay = 0.75f;
            auto_tweak::load_param<F32, F32>(spawn.delay, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "spawn.delay");
        }
        if (init)
        {
            spawn.fall_time = 2.5f;
            auto_tweak::load_param<F32, F32>(spawn.fall_time, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "spawn.fall_time");
        }
        if (init)
        {
            spawn.spew.speed = 4000.0f;
            auto_tweak::load_param<F32, F32>(spawn.spew.speed, 1.0f, 0.0f, 1000000000.0f, ap,
                                             apsize, "spawn.spew.speed");
        }
        if (init)
        {
            spawn.spew.drop_off = -6000.0f;
            auto_tweak::load_param<F32, F32>(spawn.spew.drop_off, 1.0f, -1000000000.0f, 0.0f, ap,
                                             apsize, "spawn.spew.drop_off");
        }
        if (init)
        {
            spawn.spew.delay = 0.75f;
            auto_tweak::load_param<F32, F32>(spawn.spew.delay, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "spawn.spew.delay");
        }
        if (init)
        {
            spawn.spew.voffset = -2.0f;
            auto_tweak::load_param<F32, F32>(spawn.spew.voffset, 1.0f, -100.0f, 100.0f, ap, apsize,
                                             "spawn.spew.voffset");
        }
        if (init)
        {
            wave_ring.min_radius = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.min_radius, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "wave_ring.min_radius");
        }
        if (init)
        {
            wave_ring.max_radius = 25.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.max_radius, 1.0f, 0.0f, 1000.0f, ap, apsize,
                                             "wave_ring.max_radius");
        }
        if (init)
        {
            wave_ring.height = 0.2f;
            auto_tweak::load_param<F32, F32>(wave_ring.height, 1.0f, -100.0f, 100.0f, ap, apsize,
                                             "wave_ring.height");
        }
        if (init)
        {
            wave_ring.fade_time = 2.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.fade_time, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "wave_ring.fade_time");
        }
        if (init)
        {
            wave_ring.max_vel = 20.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.max_vel, 1.0f, 0.0, 10000.0f, ap, apsize,
                                             "wave_ring.max_vel");
        }
        if (init)
        {
            wave_ring.accel = 70.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.accel, 1.0f, 0.0f, 1000.0f, ap, apsize,
                                             "wave_ring.accel");
        }
        if (init)
        {
            wave_ring.segment_length = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.segment_length, 1.0f, 0.0099999998f, 10.0f,
                                             ap, apsize, "wave_ring.segment_length");
        }
        if (init)
        {
            wave_ring.particle_height = -0.3f;
            auto_tweak::load_param<F32, F32>(wave_ring.particle_height, 1.0f, -10.0f, 10.0f, ap,
                                             apsize, "wave_ring.particle_height");
        }
        if (init)
        {
            wave_ring.particles = 5000.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.particles, 1.0f, 0.0f, 1000000.0f, ap,
                                             apsize, "wave_ring.particles");
        }
        if (init)
        {
            wave_ring.damage_height = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.damage_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "wave_ring.damage_height");
        }
        if (init)
        {
            wave_ring.damage_width = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.damage_width, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "wave_ring.damage_width");
        }
        if (init)
        {
            wave_ring.knock_back = 10.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.knock_back, 1.0f, 0.0f, 1000.0f, ap, apsize,
                                             "wave_ring.knock_back");
        }
        if (init)
        {
            wave_ring.unit[0].radius_offset = 0.25f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[0].radius_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[0].radius_offset");
        }
        if (init)
        {
            wave_ring.unit[0].height_offset = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[0].height_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[0].height_offset");
        }
        if (init)
        {
            wave_ring.unit[0].line = 0;
            auto_tweak::load_param<bool, S32>(wave_ring.unit[0].line, 0, 0, 0, ap, apsize,
                                           "wave_ring.unit[0].line");
        }
        if (init)
        {
            wave_ring.unit[0].thickness = 0.3f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[0].thickness, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[0].thickness");
        }
        if (init)
        {
            wave_ring.unit[0].color = xColorFromRGBA(255, 255, 0, 255);
            auto_tweak::load_param<iColor_tag, S32>(wave_ring.unit[0].color, 0, 0, 0, ap, apsize,
                                                    "wave_ring.unit[0].color");
        }
        if (init)
        {
            wave_ring.unit[0].rot_radius = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[0].rot_radius, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[0].rot_radius");
        }
        if (init)
        {
            wave_ring.unit[0].degrees = 720.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[0].degrees, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "wave_ring.unit[0].degrees");
        }
        if (init)
        {
            wave_ring.unit[1].radius_offset = 0.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[1].radius_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[1].radius_offset");
        }
        if (init)
        {
            wave_ring.unit[1].height_offset = 0.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[1].height_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[1].height_offset");
        }
        if (init)
        {
            wave_ring.unit[1].line = 0;
            auto_tweak::load_param<bool, S32>(wave_ring.unit[1].line, 0, 0, 0, ap, apsize,
                                           "wave_ring.unit[1].line");
        }
        if (init)
        {
            wave_ring.unit[1].thickness = 1.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[1].thickness, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[1].thickness");
        }
        if (init)
        {
            wave_ring.unit[1].color = xColorFromRGBA(255, 255, 255, 255);
            auto_tweak::load_param<iColor_tag, S32>(wave_ring.unit[1].color, 0, 0, 0, ap, apsize,
                                                    "wave_ring.unit[1].color");
        }
        if (init)
        {
            wave_ring.unit[1].rot_radius = 0.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[1].rot_radius, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[1].rot_radius");
        }
        if (init)
        {
            wave_ring.unit[1].degrees = 360.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[1].degrees, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "wave_ring.unit[1].degrees");
        }
        if (init)
        {
            wave_ring.unit[2].radius_offset = -0.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[2].radius_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[2].radius_offset");
        }
        if (init)
        {
            wave_ring.unit[2].height_offset = 0.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[2].height_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[2].height_offset");
        }
        if (init)
        {
            wave_ring.unit[2].line = 0;
            auto_tweak::load_param<bool, S32>(wave_ring.unit[2].line, 0, 0, 0, ap, apsize,
                                           "wave_ring.unit[2].line");
        }
        if (init)
        {
            wave_ring.unit[2].thickness = 1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[2].thickness, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[2].thickness");
        }
        if (init)
        {
            wave_ring.unit[2].color = xColorFromRGBA(155, 255, 255, 255);
            auto_tweak::load_param<iColor_tag, S32>(wave_ring.unit[2].color, 0, 0, 0, ap, apsize,
                                                    "wave_ring.unit[2].color");
        }
        if (init)
        {
            wave_ring.unit[2].rot_radius = 0.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[2].rot_radius, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[2].rot_radius");
        }
        if (init)
        {
            wave_ring.unit[2].degrees = 360.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[2].degrees, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "wave_ring.unit[2].degrees");
        }
        if (init)
        {
            wave_ring.unit[3].radius_offset = -1.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[3].radius_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[3].radius_offset");
        }
        if (init)
        {
            wave_ring.unit[3].height_offset = 0.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[3].height_offset, 1.0f, -10.0f, 10.0f,
                                             ap, apsize, "wave_ring.unit[3].height_offset");
        }
        if (init)
        {
            wave_ring.unit[3].line = 0;
            auto_tweak::load_param<bool, S32>(wave_ring.unit[3].line, 0, 0, 0, ap, apsize,
                                           "wave_ring.unit[3].line");
        }
        if (init)
        {
            wave_ring.unit[3].thickness = 0.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[3].thickness, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[3].thickness");
        }
        if (init)
        {
            wave_ring.unit[3].color = xColorFromRGBA(0, 255, 255, 255);
            auto_tweak::load_param<iColor_tag, S32>(wave_ring.unit[3].color, 0, 0, 0, ap, apsize,
                                                    "wave_ring.unit[3].color");
        }
        if (init)
        {
            wave_ring.unit[3].rot_radius = 0.5f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[3].rot_radius, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "wave_ring.unit[3].rot_radius");
        }
        if (init)
        {
            wave_ring.unit[3].degrees = 360.0f;
            auto_tweak::load_param<F32, F32>(wave_ring.unit[3].degrees, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "wave_ring.unit[3].degrees");
        }
        if (init)
        {
            ambient_ring.radius = 2.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "ambient_ring.radius");
        }
        if (init)
        {
            ambient_ring.min_height = 0.4f;
            auto_tweak::load_param<F32, F32>(ambient_ring.min_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "ambient_ring.min_height");
        }
        if (init)
        {
            ambient_ring.max_height = 3.5f;
            auto_tweak::load_param<F32, F32>(ambient_ring.max_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "ambient_ring.max_height");
        }
        if (init)
        {
            ambient_ring.speed = 2.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.speed, 1.0f, 0.0f, 10000.0f, ap, apsize,
                                             "ambient_ring.speed");
        }
        if (init)
        {
            ambient_ring.segment_length = 1.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.segment_length, 1.0f, 0.01f, 10.0f, ap,
                                             apsize, "ambient_ring.segment_length");
        }
        if (init)
        {
            ambient_ring.thickness = 0.2;
            auto_tweak::load_param<F32, F32>(ambient_ring.thickness, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "ambient_ring.thickness");
        }
        if (init)
        {
            ambient_ring.color = xColorFromRGBA(255, 100, 155, 255);
            auto_tweak::load_param<iColor_tag, S32>(ambient_ring.color, 0, 0, 0, ap, apsize,
                                                    "ambient_ring.color");
        }
        if (init)
        {
            ambient_ring.knock_back = 5.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.knock_back, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "ambient_ring.knock_back");
        }
        if (init)
        {
            ambient_ring.charge.radius = 4.5f;
            auto_tweak::load_param<F32, F32>(ambient_ring.charge.radius, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "ambient_ring.charge.radius");
        }
        if (init)
        {
            ambient_ring.charge.max_height = 8.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.charge.max_height, 1.0f, -100.0f, 100.0f,
                                             ap, apsize, "ambient_ring.charge.max_height");
        }
        if (init)
        {
            ambient_ring.charge.speed = 15.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.charge.speed, 1.0f, 0.0f, 10000.0f, ap,
                                             apsize, "ambient_ring.charge.speed");
        }
        if (init)
        {
            ambient_ring.charge.thickness = 5.0f;
            auto_tweak::load_param<F32, F32>(ambient_ring.charge.thickness, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "ambient_ring.charge.thickness");
        }
        if (init)
        {
            ambient_ring.charge.color = xColorFromRGBA(155, 100, 100, 0);
            auto_tweak::load_param<iColor_tag, S32>(ambient_ring.charge.color, 0, 0, 0, ap, apsize,
                                                    "ambient_ring.charge.color");
            ;
        }
        if (init)
        {
            tentacle.thickness = 0.2f;
            auto_tweak::load_param<F32, F32>(tentacle.thickness, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "tentacle.thickness");
        }
        if (init)
        {
            tentacle.rand_radius = 1.0f;
            auto_tweak::load_param<F32, F32>(tentacle.rand_radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "tentacle.rand_radius");
        }
        if (init)
        {
            tentacle.rot_radius = 1.0f;
            auto_tweak::load_param<F32, F32>(tentacle.rot_radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "tentacle.rot_radius");
        }
        if (init)
        {
            tentacle.move_degrees = 2440.0f;
            auto_tweak::load_param<F32, F32>(tentacle.move_degrees, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "tentacle.move_degrees");
        }
        if (init)
        {
            tentacle.color = xColorFromRGBA(255, 255, 196, 255);
            auto_tweak::load_param<iColor_tag, S32>(tentacle.color, 0, 0, 0, ap, apsize,
                                                    "tentacle.color");
        }
        if (init)
        {
            tentacle.delay = 1.0f;
            auto_tweak::load_param<F32, F32>(tentacle.delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "tentacle.delay");
        }
        if (init)
        {
            tentacle.time = 2.0f;
            auto_tweak::load_param<F32, F32>(tentacle.time, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "tentacle.time");
        }
        if (init)
        {
            tentacle.max = 5;
            auto_tweak::load_param<S32, S32>(tentacle.max, 1, 1, 7, ap, apsize, "tentacle.max");
        }
        if (init)
        {
            tentacle.particles = 0.0f;
            auto_tweak::load_param<F32, F32>(tentacle.particles, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "tentacle.particles");
        }
        if (init)
        {
            tentacle.knock_back = 5.0f;
            auto_tweak::load_param<F32, F32>(tentacle.knock_back, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "tentacle.knock_back");
        }
        if (init)
        {
            tentacle.damage_width = 0.3f;
            auto_tweak::load_param<F32, F32>(tentacle.damage_width, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "tentacle.damage_width");
        }
        if (init)
        {
            tentacle.charge.thickness = 0.4f;
            auto_tweak::load_param<F32, F32>(tentacle.charge.thickness, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "tentacle.charge.thickness");
        }
        if (init)
        {
            tentacle.charge.color = xColorFromRGBA(255, 255, 0, 255);
            auto_tweak::load_param<iColor_tag, S32>(tentacle.charge.color, 0, 0, 0, ap, apsize,
                                                    "tentacle.charge.color");
        }
        if (init)
        {
            tentacle.charge.move_degrees = 180.0f;
            auto_tweak::load_param<F32, F32>(tentacle.charge.move_degrees, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "tentacle.charge.move_degrees");
        }
        if (init)
        {
            thump.delay = 0.6f;
            auto_tweak::load_param<F32, F32>(thump.delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "thump.delay");
        }
        if (init)
        {
            thump.rings = 5;
            auto_tweak::load_param<S32, S32>(thump.rings, 1, 1, 10, ap, apsize, "thump.rings");
        }
        if (init)
        {
            thump.voffset = 0.0f;
            auto_tweak::load_param<F32, F32>(thump.voffset, 1.0f, -100.0f, 100.0f, ap, apsize,
                                             "thump.voffset");
        }
        if (init)
        {
            thump.particles = 200.0f;
            auto_tweak::load_param<F32, F32>(thump.particles, 1.0f, 0.0f, 10000.0f, ap, apsize,
                                             "thump.particles");
        }
        if (init)
        {
            thump.radius = 4.0f;
            auto_tweak::load_param<F32, F32>(thump.radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "thump.radius");
        }
        if (init)
        {
            thump.width = 2.0f;
            auto_tweak::load_param<F32, F32>(thump.width, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "thump.width");
        }
        if (init)
        {
            thump.vel = 10.0f;
            auto_tweak::load_param<F32, F32>(thump.vel, 1.0f, 0.0f, 10000.0f, ap, apsize,
                                             "thump.vel");
        }
        if (init)
        {
            thump.particle_drop_off = 0.5f;
            auto_tweak::load_param<F32, F32>(thump.particle_drop_off, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "thump.particle_drop_off");
        }
        if (init)
        {
            thump.vel_drop_off = 0.69999999f;
            auto_tweak::load_param<F32, F32>(thump.vel_drop_off, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "thump.vel_drop_off");
        }
        if (init)
        {
            sound[SOUND_AMBIENT_RING].volume = 0.3f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_AMBIENT_RING].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_AMBIENT_RING].volume");
        }
        // Retail registers no "delay" tweak for SOUND_AMBIENT_RING; sound[0].delay
        // is left at whatever the zero-initialised tweak block holds.
        if (init)
        {
            sound[SOUND_AMBIENT_RING].radius_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_AMBIENT_RING].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_AMBIENT_RING].radius_inner");
        }
        if (init)
        {
            sound[SOUND_AMBIENT_RING].radius_outer = 25.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_AMBIENT_RING].radius_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_AMBIENT_RING].radius_outer");
        }
        if (init)
        {
            sound[SOUND_AMBIENT_RING].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_AMBIENT_RING].priority, 1, 0, 1000, ap,
                                             apsize, "sound[SOUND_AMBIENT_RING].priority");
        }
        if (init)
        {
            sound[SOUND_BIRTH].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BIRTH].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_BIRTH].volume");
        }
        if (init)
        {
            sound[SOUND_BIRTH].delay = 0.2f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BIRTH].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_BIRTH].delay");
        }
        if (init)
        {
            sound[SOUND_BIRTH].radius_inner = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BIRTH].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BIRTH].radius_inner");
        }
        if (init)
        {
            sound[SOUND_BIRTH].radius_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BIRTH].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BIRTH].radius_outer");
        }
        if (init)
        {
            sound[SOUND_BIRTH].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_BIRTH].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_BIRTH].priority");
        }
        if (init)
        {
            sound[SOUND_CHARGE].volume = 0.75f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].volume");
        }
        if (init)
        {
            sound[SOUND_CHARGE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].delay");
        }
        if (init)
        {
            sound[SOUND_CHARGE].radius_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHARGE].radius_inner");
        }
        if (init)
        {
            sound[SOUND_CHARGE].radius_outer = 40.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].radius_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHARGE].radius_outer");
        }
        if (init)
        {
            sound[SOUND_CHARGE].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_CHARGE].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_CHARGE].priority");
        }
        if (init)
        {
            sound[SOUND_CHEER].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHEER].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_CHEER].volume");
        }
        if (init)
        {
            sound[SOUND_CHEER].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHEER].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_CHEER].delay");
        }
        if (init)
        {
            sound[SOUND_CHEER].radius_inner = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHEER].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHEER].radius_inner");
        }
        if (init)
        {
            sound[SOUND_CHEER].radius_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHEER].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHEER].radius_outer");
        }
        if (init)
        {
            sound[SOUND_CHEER].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_CHEER].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_CHEER].priority");
        }
        if (init)
        {
            sound[SOUND_GRUNT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_GRUNT].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_GRUNT].volume");
        }
        if (init)
        {
            sound[SOUND_GRUNT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_GRUNT].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_GRUNT].delay");
        }
        if (init)
        {
            sound[SOUND_GRUNT].radius_inner = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_GRUNT].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_GRUNT].radius_inner");
        }
        if (init)
        {
            sound[SOUND_GRUNT].radius_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_GRUNT].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_GRUNT].radius_outer");
        }
        if (init)
        {
            sound[SOUND_GRUNT].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_GRUNT].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_GRUNT].priority");
        }
        if (init)
        {
            sound[SOUND_LAND].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_LAND].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_LAND].volume");
        }
        if (init)
        {
            sound[SOUND_LAND].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_LAND].delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "sound[SOUND_LAND].delay");
        }
        if (init)
        {
            sound[SOUND_LAND].radius_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_LAND].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_LAND].radius_inner");
        }
        if (init)
        {
            sound[SOUND_LAND].radius_outer = 40.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_LAND].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_LAND].radius_outer");
        }
        if (init)
        {
            sound[SOUND_LAND].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_LAND].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_LAND].priority");
        }
        if (init)
        {
            sound[SOUND_MOVE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_MOVE].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_MOVE].volume");
        }
        if (init)
        {
            sound[SOUND_MOVE].radius_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_MOVE].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_MOVE].radius_inner");
        }
        if (init)
        {
            sound[SOUND_MOVE].radius_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_MOVE].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_MOVE].radius_outer");
        }
        if (init)
        {
            sound[SOUND_MOVE].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_MOVE].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_MOVE].priority");
        }
        if (init)
        {
            sound[SOUND_OSCILLATE].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_OSCILLATE].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_OSCILLATE].volume");
        }
        if (init)
        {
            sound[SOUND_OSCILLATE].radius_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_OSCILLATE].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_OSCILLATE].radius_inner");
        }
        if (init)
        {
            sound[SOUND_OSCILLATE].radius_outer = 25.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_OSCILLATE].radius_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_OSCILLATE].radius_outer");
        }
        if (init)
        {
            sound[SOUND_OSCILLATE].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_OSCILLATE].priority, 1, 0, 1000, ap,
                                             apsize, "sound[SOUND_OSCILLATE].priority");
        }
        if (init)
        {
            sound[SOUND_RISE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_RISE].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_RISE].volume");
        }
        if (init)
        {
            sound[SOUND_RISE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_RISE].delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "sound[SOUND_RISE].delay");
        }
        if (init)
        {
            sound[SOUND_RISE].radius_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_RISE].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_RISE].radius_inner");
        }
        if (init)
        {
            sound[SOUND_RISE].radius_outer = 40.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_RISE].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_RISE].radius_outer");
        }
        if (init)
        {
            sound[SOUND_RISE].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_RISE].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_RISE].priority");
        }
        if (init)
        {
            sound[SOUND_TAUNT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_TAUNT].volume");
        }
        if (init)
        {
            sound[SOUND_TAUNT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_TAUNT].delay");
        }
        if (init)
        {
            sound[SOUND_TAUNT].radius_inner = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].radius_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_TAUNT].radius_inner");
        }
        if (init)
        {
            sound[SOUND_TAUNT].radius_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].radius_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_TAUNT].radius_outer");
        }
        if (init)
        {
            sound[SOUND_TAUNT].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_TAUNT].priority, 1, 0, 1000, ap, apsize,
                                             "sound[SOUND_TAUNT].priority");
        }
        if (init)
        {
            sound[SOUND_WAVE_RING].volume = 0.75f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_WAVE_RING].volume, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_WAVE_RING].volume");
        }
        if (init)
        {
            sound[SOUND_WAVE_RING].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_WAVE_RING].delay, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "sound[SOUND_WAVE_RING].delay");
        }
        if (init)
        {
            sound[SOUND_WAVE_RING].radius_inner = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_WAVE_RING].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_WAVE_RING].radius_inner");
        }
        // Retail copy/paste slip: this block re-registers radius_inner (same
        // field, same tweak name) instead of radius_outer, so radius_inner ends
        // up defaulting to 50 and radius_outer is never registered at all.
        if (init)
        {
            sound[SOUND_WAVE_RING].radius_inner = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_WAVE_RING].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_WAVE_RING].radius_inner");
        }
        if (init)
        {
            sound[SOUND_WAVE_RING].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_WAVE_RING].priority, 1, 0, 1000, ap,
                                             apsize, "sound[SOUND_WAVE_RING].priority");
        }
    }
} // namespace

void zNPCKingJelly::ParseLinks()
{
    zNPCCommon::ParseLinks();

    xLinkAsset* l = link;
    xLinkAsset* end = l + linkCount;
    for (; l != end; l++)
    {
        if (l->dstEvent == eEventConnectToChild)
        {
            add_child(*zSceneFindObject(l->dstAssetID), l->param[0]);
        }
    }
}

U32 zNPCKingJelly::AnimPick(S32 rawgoal, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 hash = 0;
    S32 anim;

    switch (rawgoal)
    {
    case NPC_GOAL_KJIDLE:
        anim = ANIM_Idle01;
        break;
    case NPC_GOAL_KJBORED:
        anim = xUtil_choose<S32>(bored_anims, 2, NULL);
        break;
    case NPC_GOAL_KJSPAWNKIDS:
        anim = ANIM_SpawnKids01;
        break;
    case NPC_GOAL_KJTAUNT:
        anim = ANIM_Taunt01;
        break;
    case NPC_GOAL_KJSHOCKGROUND:
        anim = ANIM_AttackWindup01;
        break;
    case NPC_GOAL_KJDAMAGE:
        anim = ANIM_Damage01;
        break;
    case NPC_GOAL_KJDEATH:
        anim = -1;
        break;
    default:
        anim = ANIM_Idle01;
        break;
    }

    if (anim > -1)
    {
        hash = g_hash_subbanim[anim];
    }

    return hash;
}

void zNPCKingJelly::SelfSetup()
{
    xBehaveMgr* bmgr;
    xPsyche* psy;

    bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);
    psy = psy_instinct;
    psy->BrainBegin();
    psy->AddGoal(NPC_GOAL_KJIDLE, NULL);
    psy->AddGoal(NPC_GOAL_KJBORED, NULL);
    psy->AddGoal(NPC_GOAL_KJSPAWNKIDS, NULL);
    psy->AddGoal(NPC_GOAL_KJTAUNT, NULL);
    psy->AddGoal(NPC_GOAL_KJSHOCKGROUND, NULL);
    psy->AddGoal(NPC_GOAL_KJDAMAGE, NULL);
    psy->AddGoal(NPC_GOAL_KJDEATH, NULL);
    psy->AddGoal(NPC_GOAL_LIMBO, NULL);
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_KJIDLE);
}

void zNPCKingJelly::Damage(en_NPC_DAMAGE_TYPE damtype, xBase*, const xVec3*)
{
    if (!this->flag.fighting || this->flag.died)
    {
        return;
    }

    S32 state = this->psy_instinct->GIDOfActive();
    switch (damtype)
    {
    case DMGTYP_SIDE:
    case DMGTYP_BOULDER:
    case DMGTYP_BUBBOWL:
        if (state == 'NGM5' &&
            (this->shockstate == SS_RELEASE 
            || this->shockstate == SS_COOL_DOWN 
            || this->shockstate == SS_STOP))
        {
            set_life(this->life - 1);
        }
        break;
    
    case DMGTYP_CRUISEBUBBLE:
        if (!(state == 'NGM6') && !(state == 'NGM7') )
        {
            set_life(this->life - 1);
        }
        break;
    }
}

F32 zNPCKingJelly::get_variance() const
{
    return tweak.interval.variance * (2.0f * xurand() - 1.0f);
}

bool zNPCKingJelly::bored() const
{
    switch (round)
    {
    case 0:
        return (attack + 1) % 2 == 0;
    case 1:
        return (attack + 1) % 3 == 0;
    case 2:
        return (attack + 1) % 4 == 0;
    }

    return false;
}

S32 zNPCKingJelly::max_strikes() const
{
    return round + 1;
}

void zNPCKingJelly::update_camera(F32 dt)
{
    zCameraDisableTracking(CO_BOSS);
    if(!(zCameraIsTrackingDisabled() & ~0x8))
    {
        boss_cam.update(dt);
    }
}

void zNPCKingJelly::set_life(S32 life)
{
    S32 oldlife = this->life;
    
    this->life = range_limit<S32>(life, 0, tweak.max_life);
    S32 state = this->psy_instinct->GIDOfActive();
    if (!(state == 'NGM6') && !(state == 'NGM7') && !(this->life >= oldlife))
    {
        this->psy_instinct->GoalSet('NGM6', GOAL_STAT_PROCESS);
        start_blink();

        for (S32 i = this->life; i < oldlife; i++)
        {
            zEntEvent(this, this, eEventNPCHPDecremented);
        }

        if (this->life <= 0)
        {
            zEntEvent(this, this, eEventDeath);
        }
    } 
    else
    {
        update_round();
    }

}

void zNPCKingJelly::add_child(xBase& child, S32 wave)
{
    switch (child.baseType)
    {
    case eBaseTypeNPC:
        init_child(children[children_size], (zNPCCommon&)child, wave);
        children_size++;
        break;
    case eBaseTypeGroup:
    {
        U32 i = 0;
        U32 count = xGroupGetCount((xGroup*)&child);
        for (; i < count; i++)
        {
            xBase* item = xGroupGetItemPtr((xGroup*)&child, i);
            add_child(*item, wave);
        }
        break;
    }
    }
}

void zNPCKingJelly::init_child(zNPCKingJelly::child_data& child, zNPCCommon& npc, int wave)
{
    child.npc = &npc;
    child.wave = wave;
    child.active = 1;
    child.callback.eventFunc = npc.eventFunc;
    child.callback.update = npc.update;
    child.callback.bupdate = npc.bupdate;
    child.callback.move = npc.move;
    child.callback.render = npc.render;
    child.callback.transl = npc.transl;
}

void zNPCKingJelly::disable_child(zNPCKingJelly::child_data& child)
{
    if (child.active)
    {
        ((zNPCJelly*)child.npc)->JellyKill();
        child.active = false;
    }
}

void zNPCKingJelly::enable_child(zNPCKingJelly::child_data& child)
{
    if (child.active == false)
    {
        child.active = true;
    }
}

S32 zNPCKingJelly::count_children(S32 wave)
{
    S32 count = 0;

    for (U32 i = 0; i < children_size; i++)
    {
        if (children[i].wave == wave)
        {
            count++;
        }
    }

    return count;
}

void zNPCKingJelly::spawn_children(int wave, int count)
{
    U8 active[32];
    S32 total = 0;

    for (U32 i = 0; i < children_size; i++)
    {
        if (children[i].wave == wave && !children[i].active)
        {
            active[total] = i;
            total++;
        }
    }

    if (count > total)
    {
        count = total;
    }

    while (count > 0)
    {
        U32 i = active[(U32)rand() % total];
        child_data& child = children[i];

        if (child.active)
        {
            continue;
        }

        enable_child(child);
        move_to_spawn_position(*child.npc, 2.0f * PI * count / total);
        count--;
    }
}

void zNPCKingJelly::move_to_spawn_position(zNPCCommon& npc, F32 t)
{
    xVec3 loc = get_center();

    loc.y += tweak.spawn.voffset + (xurand() - 0.5f);
    loc.x += tweak.spawn.hoffset * icos(t);
    loc.z += tweak.spawn.hoffset * isin(t);

    npc.Reset();
    ((zNPCJelly&)npc).JellySpawn(&loc, tweak.spawn.fall_time);
}

void zNPCKingJelly::taunt()
{
    switch (psy_instinct->GIDOfActive())
    {
    case NPC_GOAL_KJTAUNT:
    case NPC_GOAL_KJDAMAGE:
    case NPC_GOAL_KJDEATH:
        return;
    }

    psy_instinct->GoalSet(NPC_GOAL_KJTAUNT, GOAL_STAT_PROCESS);
}

void zNPCKingJelly::repel_player()
{
    if (globals.player.cheat_mode)
    {
        return;
    }

    xVec3 center = get_center();
    xVec3& player_loc = (xVec3&)globals.player.ent.model->Mat->pos;
    xVec3& player_vel = globals.player.ent.frame->vel;

    xVec3 offset = player_loc - center;
    offset.y = 0.0f;

    F32 imag = offset.length2();

    F32 radius;
    if (bound.sph.center.y - bound.sph.r - get_bottom()->y < 2.0f)
    {
        radius = tweak.repel_radius_ground;
    }
    else
    {
        radius = tweak.repel_radius;
    }

    if ((imag >= -0.00001f && imag <= 0.00001f) || imag >= radius * radius)
    {
        return;
    }

    imag = xsqrt(imag);

    xVec3 dir = offset;
    dir *= 1.0f / imag;

    F32 vdot = player_vel.dot(dir);
    if (vdot < 0.0f)
    {
        player_vel -= dir * vdot;
    }

    player_loc += dir * (radius - imag);
    globals.player.ent.frame->mat.pos = player_loc;
}

namespace
{
    bool sphere_hits_line(const xSphere& o, const xVec3& v1, const xVec3& v2, F32 width)
    {
        xVec3 d1 = v2 - v1;
        xVec3 d2 = v1 - o.center;

        F32 r = o.r + width;
        F32 a = d1.length2();
        F32 b = 2.0f * d1.dot(d2);
        F32 q = b * b - 4.0f * a * (d2.length2() - r * r);

        if (q < 0.0f)
        {
            return false;
        }

        F32 d = xsqrt(q);
        F32 ia = 1.0f / (2.0f * a);
        F32 r1 = ia * (-b + d);
        F32 r2 = ia * (-b - d);

        return (r1 >= 0.0f && r1 <= 1.0f) || (r2 >= 0.0f && r2 <= 1.0f);
    }

    S32 sphere_hits_sphere_xz(const xSphere& a, const xSphere& b)
    {
        F32 dx = b.center.x - a.center.x;
        F32 dz = b.center.z - a.center.z;
        F32 sum = b.r + a.r;
        F32 diff = b.r - a.r;
        F32 dist = dx * dx + dz * dz;

        if (dist > sum * sum)
        {
            return 4;
        }
        if (dist < diff * diff)
        {
            return 2;
        }
        return 1;
    }
} // namespace

xVec3 zNPCKingJelly::get_away() const
{
    xVec3 dir;

    dir.x = globals.player.ent.bound.sph.center.x - bound.sph.center.x;
    dir.z = globals.player.ent.bound.sph.center.z - bound.sph.center.z;
    dir.y = 0.0f;

    F32 dist2 = dir.x * dir.x + dir.z * dir.z;

    if (dist2 < 0.001f)
    {
        dir.assign(0.0f, 1.0f, 0.0f);
    }
    else
    {
        F32 scale = 0.70710677f * (1.0f / xsqrt(dist2));
        dir.assign(dir.x * scale, 0.70710677f, dir.z * scale);
    }

    return dir;
}

bool zNPCKingJelly::apply_tentacle_damage()
{
    if (disable_tentacle_damage)
    {
        return false;
    }

    xSphere o;
    xVec3 v1;
    xVec3 v2;

    o.center = 0.0f;
    o.r = 1.0f;

    v1.assign(0.0f, 0.0f, 0.0f);
    v2.assign(1.0f, 1.0f, 1.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    v1.assign(-10.0f, 0.0f, 0.0f);
    v2.assign(10.0f, 0.0f, 0.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    v1.assign(-10.0f, 0.5f, 0.0f);
    v2.assign(10.0f, 0.5f, 0.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    v1.assign(-10.0f, 1.0f, 0.0f);
    v2.assign(10.0f, 1.0f, 0.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    v1.assign(-10.0f, 1.5f, 0.0f);
    v2.assign(10.0f, 1.5f, 0.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    v1.assign(10.0f, 0.0f, 0.0f);
    v2.assign(20.0f, 0.0f, 0.0f);
    sphere_hits_line(o, v1, v2, 0.0f);

    xEnt& player = globals.player.ent;

    if (!(sphere_hits_sphere_xz(bound.sph, player.bound.sph) & 3))
    {
        return false;
    }

    xVec3 joint[2];

    for (S32 i = 0; i < 7; i++)
    {
        joint[0] = xModelGetBoneLocation(*model, tentacle_bone[i][0]);

        for (S32 j = 1; j < 4; j++)
        {
            joint[j & 1] = xModelGetBoneLocation(*model, tentacle_bone[i][j]);

            if (sphere_hits_line(player.bound.sph, joint[(j - 1) & 1], joint[j & 1],
                                 tweak.tentacle.damage_width))
            {
                xVec3& player_vel = player.frame->vel;

                player_vel = get_away();
                player_vel *= tweak.tentacle.knock_back;
                zEntPlayer_Damage(this, 1);

                static struct
                {
                    S32 i;
                    S32 j;
                } last_hit_at = { -1, -1 };

                last_hit_at.i = i;
                last_hit_at.j = j;

                return true;
            }
        }
    }

    return false;
}

bool zNPCKingJelly::apply_wave_damage()
{
    lightning_ring& ring = wave_rings[0];

    if (!ring.active)
    {
        return false;
    }

    xVec3& loc = (xVec3&)model->Mat->pos;
    xEnt& player = globals.player.ent;
    xSphere& o = player.bound.sph;

    xSphere inner;
    xSphere outer;

    outer.center = ring.center;
    outer.r = ring.current.radius;

    inner.center = outer.center;
    inner.r = outer.r - tweak.wave_ring.damage_width;
    if (inner.r < 0.0f)
    {
        inner.r = 0.0f;
    }

    F32 lower = loc.y;
    F32 upper = loc.y + tweak.wave_ring.damage_height;

    if (!(sphere_hits_sphere_xz(o, outer) & 3))
    {
        return false;
    }

    if (!(sphere_hits_sphere_xz(o, inner) & 5))
    {
        return false;
    }

    if (o.center.y - o.r > upper)
    {
        return false;
    }

    if (o.center.y + o.r < lower)
    {
        return false;
    }

    xVec3& player_vel = player.frame->vel;

    player_vel = get_away();
    player_vel *= tweak.wave_ring.knock_back;
    zEntPlayer_Damage(this, 1);

    return true;
}

bool zNPCKingJelly::apply_ambient_damage()
{
    xEnt& player = globals.player.ent;

    const xVec3 center = get_center();
    xVec3 offset = (xVec3&)player.model->Mat->pos - center;

    F32 r = ambient_rings[0].current.radius;

    if (!ambient_rings[0].active)
    {
        return false;
    }

    if (offset.x * offset.x + offset.z * offset.z > r * r)
    {
        return false;
    }

    xVec3& player_vel = player.frame->vel;

    player_vel = get_away();
    player_vel *= tweak.ambient_ring.knock_back;
    zEntPlayer_Damage(this, 1);

    return true;
}

void zNPCKingJelly::check_player_damage()
{
    if (globals.player.cheat_mode)
    {
        return;
    }

    if (apply_wave_damage())
    {
        return;
    }
    if (apply_tentacle_damage())
    {
        return;
    }
    if (apply_ambient_damage())
    {
        return;
    }
}

void zNPCKingJelly::start_fight()
{
    if (!flag.fighting)
    {
        flag.fighting = true;
        show_attack_model();
        fade_curtain();
        play_sound(SOUND_AMBIENT_RING, (const xVec3*)&model->Mat->pos);
        play_sound(SOUND_OSCILLATE, (const xVec3*)&model->Mat->pos);
        zMusicSetVolume(tweak.music_fade, tweak.music_fade_delay);
        zCameraDisableTracking(CO_BOSS);
        boss_cam.start(globals.camera);
        boss_cam.set_targets((const xVec3&)globals.player.ent.model->Mat->pos, bound.sph.center,
                             bound.sph.r);
    }
}

void zNPCKingJelly::decompose()
{
    if (flag.died || !flag.fighting)
    {
        return;
    }

    flag.died = true;
    kill_sounds();
    destroy_ambient_rings();
    destroy_wave_rings();
    destroy_tentacle_lightning();

    for (U32 i = 0; i < children_size; i++)
    {
        disable_child(children[i]);
    }

    zMusicSetVolume(1.0f, tweak.music_fade_delay);
    reset_curtain();
}

void zNPCKingJelly::post_decompose()
{
    vanish();
    zCameraEnableTracking(CO_BOSS);
    boss_cam.stop();
}

void zNPCKingJelly::vanish()
{
    old.moreFlags = moreFlags;
    pflags = 0;
    moreFlags = 0;
    flags2.flg_colCheck = 0;
    flags2.flg_penCheck = 0;
    chkby = 0;
    penby = 0;
    xEntHide(this);
}

void zNPCKingJelly::reappear()
{
    moreFlags = old.moreFlags;
    this->RestoreColFlags();
    xEntShow(this);
}

void zNPCKingJelly::create_tentacle_lightning()
{
}

zLightning* zNPCKingJelly::new_tentacle_lightning(xVec3* points)
{
    _tagLightningAdd add;

    add.type = LYT_TYPE_ROTATING;
    add.total_points = 13;
    add.end_points = 0;
    add.thickness = tweak.tentacle.thickness;
    add.color = tweak.tentacle.color;
    add.rand_radius = tweak.tentacle.rand_radius;
    add.arc_height = 0.0f;
    add.rot_radius = tweak.tentacle.rot_radius;
    add.time = tweak.tentacle.time * (1.0f + 0.4f * (xurand() - 0.5f));
    add.flags = 0xaa;
    add.setup_degrees = 90.0f * xurand() + 20.0f;
    add.move_degrees = tweak.tentacle.move_degrees;

    if (xrand() & 1)
    {
        add.move_degrees *= -1.0f;
    }

    add.start = points;
    add.end = NULL;

    return zLightningAdd(&add);
}

void zNPCKingJelly::destroy_tentacle_lightning()
{
    for (S32 i = 0; i < 7; i++)
    {
        if (tentacle_lightning[i])
        {
            zLightningKill(tentacle_lightning[i]);
            tentacle_lightning[i] = NULL;
        }
    }
}

void zNPCKingJelly::update_tentacle_lightning(F32 dt)
{
    for (S32 i = 0; i < 7; i++)
    {
        zLightning*& zap = tentacle_lightning[i];

        if (zap != NULL)
        {
            if (!(zap->flags & 0x1) || !(zap->flags & 0x40))
            {
                zLightningKill(zap);
                zap = NULL;
            }
            else
            {
                refresh_tentacle_points(i);
                zLightningModifyEndpoints(zap, &tentacle_points[i][0], NULL);
            }
        }
    }

    if (tweak.tentacle.particles > 0.0f)
    {
        for (S32 i = 0; i < 7; i++)
        {
            if (tentacle_lightning[i] != NULL)
            {
                generate_zap_particles(*tentacle_lightning[i], tweak.tentacle.particles, dt);
            }
        }
    }

    if (disable_tentacle_damage)
    {
        return;
    }

    last_tentacle_shock += dt;

    if (last_tentacle_shock < tweak.tentacle.delay)
    {
        return;
    }

    last_tentacle_shock = tweak.tentacle.delay * (0.4f * (xurand() - 0.5f));

    S32 pick_max = xrand() % tweak.tentacle.max + 1;
    U32 pick_mask = 0;
    S32 picked = 0;

    while (picked < pick_max)
    {
        U32 i = xrand() % 7;

        if (!(pick_mask & (1 << i)))
        {
            pick_mask |= 1 << i;
            picked++;
        }
    }

    for (S32 i = 0; i < 7; i++)
    {
        if (pick_mask & (1 << i))
        {
            zLightning*& zap = tentacle_lightning[i];

            if (zap != NULL && (zap->flags & 0x1))
            {
                zLightningKill(zap);
            }
            else
            {
                refresh_tentacle_points(i);
            }

            zap = new_tentacle_lightning(&tentacle_points[i][0]);

            if (zap != NULL)
            {
                zap->flags |= 0x100;
            }
        }
    }
}

void zNPCKingJelly::generate_zap_particles(const zLightning& zap, F32 amount, F32 dt)
{
    F32 frac;

    if (zap.time_total <= 0.00001f || zap.time_total <= zap.time_left)
    {
        frac = 1.0f;
    }
    else
    {
        frac = zap.time_left / zap.time_total;
    }

    S32 points;
    S32 total = frac * (amount * dt) * xurand() + 0.5f;
    S32 emitted = 0;

    points = zap.legacy.total_points - 1;

    for (S32 j = 0; j < points; j++)
    {
        S32 particles = j * total / points - emitted;
        emitted += particles;

        xVec3 offset = zap.legacy.point[j + 1] - zap.legacy.point[j];

        for (S32 k = 0; k < particles; k++)
        {
            zap_emitter_settings.pos = zap.legacy.point[j] + offset * xurand();
            xParEmitterEmitCustom(zap_emitter, dt, &zap_emitter_settings);
        }
    }
}

void zNPCKingJelly::refresh_tentacle_points(S32 which)
{
    for (S32 j = 0; j < 4; j++)
    {
        tentacle_points[which][4 * j] = xModelGetBoneLocation(*model, tentacle_bone[which][j]);
    }

    for (S32 j = 0; j < 3; j++)
    {
        xVec3& start = tentacle_points[which][4 * j];
        xVec3& end = tentacle_points[which][4 * (j + 1)];
        F32 pos = 0.25f;

        for (S32 k = 1; k < 4; k++)
        {
            xVec3& v = tentacle_points[which][4 * j + k];

            xVec3Lerp(&v, &start, &end, pos);
            pos += 0.25f;
        }
    }
}

void zNPCKingJelly::refresh_tentacle_points()
{
    S32 tempvar = 0;
    do
    {
        refresh_tentacle_points(tempvar);
        tempvar = tempvar + 1;
    } while (tempvar < 7);
}

void zNPCKingJelly::update_rings(F32 dt)
{
    xVec3& subloc = (xVec3&)model->Mat[2].pos;
    xVec3& offset = cfg_npc->off_bound;
    F32 bound_radius = cfg_npc->dim_bound.x;
    xVec3 center = (xVec3&)model->Mat[0].pos;

    for (S32 i = 0; i < 3; i++)
    {
        lightning_ring& ring = ambient_rings[i];

        ring.center = center;

        if (!flag.charging)
        {
            ring.max_height = subloc.y + offset.y - bound_radius;
        }

        ring.update(dt);
    }

    for (S32 i = 0; i < 4; i++)
    {
        lightning_ring& ring = wave_rings[i];

        if (ring.active)
        {
            ring.update(dt);

            if (!ring.property.color.a)
            {
                ring.destroy();
            }
        }
    }

    if (wave_rings[0].active)
    {
        generate_ring_particles(wave_rings[0], dt);
    }
}

void zNPCKingJelly::create_ambient_rings()
{
    destroy_ambient_rings();

    for (S32 i = 0; i < 3; i++)
    {
        lightning_ring& ring = ambient_rings[i];

        ring.create();
        ring.property.line = 1;
        ring.property.thickness = tweak.ambient_ring.thickness;
        ring.property.color = tweak.ambient_ring.color;
        ring.property.rot_radius = 0.0f;
        ring.property.degrees = 0.0f;
        ring.center = (xVec3&)model->Mat->pos;
        ring.current.radius = tweak.ambient_ring.radius;
        ring.segment_length = tweak.ambient_ring.segment_length;
        ring.update_callback = updown_ring_update;
        ring.min_height = tweak.ambient_ring.min_height;
        ring.max_height = tweak.ambient_ring.max_height;
        ring.delay = 3.0f;
        ring.current.accel = tweak.ambient_ring.speed;
        ring.current.time = i;
    }
}

void zNPCKingJelly::destroy_ambient_rings()
{
    for (S32 i = 0; i < 3; i++)
    {
        ambient_rings[i].destroy();
    }
}

void zNPCKingJelly::create_wave_rings()
{
    destroy_wave_rings();

    for (S32 i = 0; i < 4; i++)
    {
        lightning_ring& ring = wave_rings[i];

        ring.create();
        ring.property.line = tweak.wave_ring.unit[i].line;
        ring.property.thickness = tweak.wave_ring.unit[i].thickness;
        ring.property.color = tweak.wave_ring.unit[i].color;
        ring.property.rot_radius = tweak.wave_ring.unit[i].rot_radius;
        ring.property.degrees = tweak.wave_ring.unit[i].degrees;
        ring.center = (xVec3&)model->Mat->pos;
        ring.update_callback = expand_ring_update;
        ring.min_radius = tweak.wave_ring.min_radius + tweak.wave_ring.unit[i].radius_offset;
        ring.max_radius = tweak.wave_ring.max_radius + tweak.wave_ring.unit[i].radius_offset;
        ring.current.radius = ring.min_radius;
        ring.segment_length = tweak.wave_ring.segment_length;
        ring.current.height = tweak.wave_ring.height + tweak.wave_ring.unit[i].height_offset;
        ring.current.accel = tweak.wave_ring.accel;
        ring.current.vel = 0.0f;
        ring.max_vel = tweak.wave_ring.max_vel;
        ring.delay = tweak.wave_ring.fade_time;
        ring.current.time = 0.0f;
    }
}

void zNPCKingJelly::destroy_wave_rings()
{
    for (S32 i = 0; i < 4; i++)
    {
        wave_rings[i].destroy();
    }
}

void zNPCKingJelly::generate_spawn_particles()
{
    spawn_particle_vel = tweak.spawn.spew.speed;
}

void zNPCKingJelly::update_spawn_particles(F32 dt)
{
    F32& amount = spawn_emitter_settings.rate.val[0];
    F32 accel = tweak.spawn.spew.drop_off;

    amount += dt * (spawn_particle_vel + 0.5f * accel * dt);

    if (amount <= 0.0f)
    {
        amount = 0.0f;
        return;
    }

    spawn_particle_vel += accel * dt;

    spawn_emitter_settings.pos = get_center();
    spawn_emitter_settings.pos.y += tweak.spawn.spew.voffset;

    xParEmitterEmitCustom(spawn_emitter, dt, &spawn_emitter_settings);
}

void zNPCKingJelly::generate_ring_particles(const lightning_ring& ring, F32 dt)
{
    F32 amount = (1.0f / 255.0f) * (tweak.wave_ring.particles * ring.property.color.a);
    S32 ring_size = set_ring_segments(ring.center, ring.current.radius, ring.segment_length);
    S32 total = amount * dt * xurand() + 0.5f;
    S32 emitted = 0;

    for (S32 j = 0; j < ring_size - 1; j++)
    {
        S32 particles = j * total / (ring_size - 1) - emitted;
        emitted += particles;

        xVec3 offset = ring_segments[j + 1] - ring_segments[j];

        for (S32 k = 0; k < particles; k++)
        {
            shock_ring_emitter_settings.pos = ring_segments[j] + offset * xurand();
            shock_ring_emitter_settings.pos.y += tweak.wave_ring.particle_height;
            xParEmitterEmitCustom(shock_ring_emitter, 1.0f / 60.0f, &shock_ring_emitter_settings);
        }
    }
}

namespace
{
    iColor_tag lerp(F32 t, iColor_tag a, iColor_tag b);
    U8 lerp(F32 t, U8 a, U8 b);
    F32 lerp(F32 t, F32 a, F32 b);
} // namespace

void zNPCKingJelly::generate_thump_particles()
{
    xParEmitterCustomSettings& s = thump_ring_emitter_settings;

    F32 iring = 1.0f / tweak.thump.rings;

    s.pos = *get_bottom();
    s.pos.y += tweak.thump.voffset;

    s.rate.val[0] = 59.999996f * tweak.thump.particles;
    s.vel.y = tweak.thump.vel;

    F32 drate = s.rate.val[0] * (-tweak.thump.particle_drop_off * iring);
    F32 dvel = s.vel.y * (-tweak.thump.vel_drop_off * iring);

    F32 dradius = tweak.thump.width * iring;
    s.radius = tweak.thump.radius;

    for (S32 i = 0; i < tweak.thump.rings; i++)
    {
        xParEmitterEmitCustom(thump_ring_emitter, 1.0f / 60.0f, &s);

        s.rate.val[0] += drate;
        s.vel.y += dvel;
        s.radius += dradius;
    }
}

void zNPCKingJelly::start_charge()
{
    flag.charging = true;

    play_sound(SOUND_CHARGE, (const xVec3*)&model->Mat->pos);
    refresh_tentacle_points();

    for (S32 i = 0; i < 7; i++)
    {
        zLightning*& zap = tentacle_lightning[i];

        if (zap != NULL && (zap->flags & 0x1))
        {
            zLightningKill(zap);
        }

        zap = new_tentacle_lightning(&tentacle_points[i][0]);

        if (zap != NULL)
        {
            zap->flags |= 0x110;
        }
    }

    update_charge(0.0f);
}

void zNPCKingJelly::update_charge(F32 frac)
{
    F32 thickness = lerp(frac, tweak.tentacle.thickness, tweak.tentacle.charge.thickness);
    iColor_tag color = lerp(frac, tweak.tentacle.color, tweak.tentacle.charge.color);

    for (S32 i = 0; i < 7; i++)
    {
        zLightning*& zap = tentacle_lightning[i];

        if (zap != NULL)
        {
            for (S32 j = 0; j < zap->legacy.total_points - 1; j++)
            {
                zap->legacy.thickness[j] = thickness;
            }

            zap->color = color;
            zap->legacy.rot.degrees =
                lerp(frac, zap->legacy.rot.degrees, tweak.tentacle.charge.move_degrees);
        }
    }

    F32 radius = lerp(frac >= 0.25f ? 1.0f : 4.0f * frac, tweak.ambient_ring.radius,
                      tweak.ambient_ring.charge.radius);
    F32 max_height =
        lerp(frac, tweak.ambient_ring.max_height, tweak.ambient_ring.charge.max_height);
    F32 speed = lerp(frac, tweak.ambient_ring.speed, tweak.ambient_ring.charge.speed);
    F32 thickness2 = lerp(frac, tweak.ambient_ring.thickness, tweak.ambient_ring.charge.thickness);
    iColor_tag color2 = lerp(frac, tweak.ambient_ring.color, tweak.ambient_ring.charge.color);

    for (S32 i = 0; i < 3; i++)
    {
        ambient_rings[i].current.accel = speed;
        ambient_rings[i].current.radius = radius;
        ambient_rings[i].max_height = max_height;
        ambient_rings[i].property.thickness = thickness2;
        ambient_rings[i].property.color = color2;
    }
}

namespace
{
    iColor_tag lerp(F32 t, iColor_tag a, iColor_tag b)
    {
        iColor_tag c;

        c.r = lerp(t, a.r, b.r);
        c.g = lerp(t, a.g, b.g);
        c.b = lerp(t, a.b, b.b);
        c.a = lerp(t, a.a, b.a);

        return c;
    }

    U8 lerp(F32 t, U8 a, U8 b)
    {
        return 0.5f + (t * ((F32)b - (F32)a) + (F32)a);
    }

    F32 lerp(F32 t, F32 a, F32 b)
    {
        return t * (b - a) + a;
    }
} // namespace

void zNPCKingJelly::end_charge()
{
    if (!flag.charging)
    {
        return;
    }

    flag.charging = false;

    for (S32 i = 0; i < 7; i++)
    {
        if (tentacle_lightning[i] != NULL)
        {
            tentacle_lightning[i]->flags &= ~0x10;
            tentacle_lightning[i]->time_left = tentacle_lightning[i]->time_total =
                tweak.interval.release;
        }
    }
}

void zNPCKingJelly::load_model()
{
    U32 i;
    xModelInstance* m = this->model;

    for (i = 0; m != NULL && i < 4; i++)
    {
        this->submodel[i] = m;
        m = m->Next;
    }

    this->submodel[0]->Data->boundingSphere.radius += 10.0f;
    this->submodel[1]->Data->boundingSphere.radius += 10.0f;
    this->submodel[3]->Data->boundingSphere.radius += 10.0f;
    this->submodel[2]->Data->boundingSphere.radius += 10.0f;

    this->submodel[3]->Flags |= 0x2;
    this->submodel[3]->Flags |= 0x1;
}

void zNPCKingJelly::load_curtain_model()
{
    char name[] = "SHOWER CURTAIN SIMP";

    this->curtain_ent = (zEnt*)zSceneFindObject(xStrHash(name));

    U32 i;
    xModelInstance* m = this->curtain_ent->model;

    for (i = 0; m != NULL && i < 5; i++)
    {
        this->curtain_model[i] = m;
        m = m->Next;
    }
}

void zNPCKingJelly::show_shower_model()
{
    submodel[0]->Flags |= 2;
    submodel[0]->Flags |= 1;
    submodel[1]->Flags |= 2;
    submodel[1]->Flags |= 1;
    submodel[2]->Flags &= 0xfffd;
    submodel[2]->Flags &= 0xfffe;
}

void zNPCKingJelly::show_attack_model()
{
    submodel[0]->Flags |= 2;
    submodel[0]->Flags &= 0xfffe;
    submodel[1]->Flags &= 0xfffd;
    submodel[1]->Flags &= 0xfffe;
    submodel[2]->Flags |= 2;
    submodel[2]->Flags |= 1;
}

void zNPCKingJelly::fade_curtain()
{
    curtain_model[2]->Alpha = curtain_model[4]->Alpha = tweak.fade_obstructions;
}

void zNPCKingJelly::reset_curtain()
{
    curtain_model[2]->Alpha = curtain_model[4]->Alpha = 1.0f;
}

namespace
{
    void set_model_color(xModelInstance* submodel, F32 r, F32 g, F32 b, F32 a)
    {
        while (submodel != NULL)
        {
            submodel->Flags |= 0x4000;
            submodel->RedMultiplier = r;
            submodel->GreenMultiplier = g;
            submodel->BlueMultiplier = b;
            submodel->Alpha = a;
            submodel = submodel->Next;
        }
    }

    void reset_model_color(xModelInstance* submodel)
    {
        while (submodel != NULL)
        {
            submodel->Flags &= 0xbfff;
            submodel->RedMultiplier = 1.0f;
            submodel->GreenMultiplier = 1.0f;
            submodel->BlueMultiplier = 1.0f;
            submodel->Alpha = 1.0f;
            submodel = submodel->Next;
        }
    }
} // namespace

void zNPCKingJelly::start_blink()
{
    blink.active = 1;
    blink.delay = 0.0f;
    blink.count = 0;
    model->Flags |= 0x4000;
}

void zNPCKingJelly::update_blink(F32 dt)
{
    if (!blink.active)
    {
        return;
    }

    F32 max_delay = tweak.blink.duration / tweak.blink.amount;

    if (blink.delay >= max_delay)
    {
        blink.delay = 0.0f;
        blink.count++;

        if (blink.count >= tweak.blink.amount)
        {
            blink.active = false;
            reset_model_color(model);
            return;
        }
    }

    blink.intensity = 1.0f - blink.delay / max_delay;
    blink.intensity = blink.intensity * (1.0f - blink.count * tweak.blink.drop_off);

    if (blink.intensity < 0.0f)
    {
        blink.intensity = 0.0f;
    }

    F32 ii = blink.intensity;
    F32 i = 1.0f - ii;

    set_model_color(model, ii * tweak.blink.color.r + i, ii * tweak.blink.color.g + i,
                    ii * tweak.blink.color.b + i, ii * tweak.blink.color.a + i);

    blink.delay += dt;
}

S32 zNPCGoalKJIdle::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    attack_delay = tweak.interval.attack[kj.round] + kj.get_variance();
    kj.flag.stop_moving = false;
    play_sound(SOUND_MOVE, (const xVec3*)&kj.model->Mat->pos);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJIdle::Exit(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    kill_sound(6);
    kj.flag.stop_moving = 1;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJIdle::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    rotate(dt);
    move(dt);

    attack_delay -= dt;

    if (attack_delay <= 0.0f)
    {
        xAnimState* anim = kj.AnimCurState();

        if (anim->ID != g_hash_subbanim[ANIM_Idle01] || dt > kj.AnimTimeRemain(NULL))
        {
            *trantype = GOAL_TRAN_SET;

            if (kj.bored())
            {
                return NPC_GOAL_KJBORED;
            }

            return NPC_GOAL_KJSHOCKGROUND;
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalKJIdle::rotate(float dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xVec3& loc = (xVec3&)kj.model->Mat->pos;
    xVec3& target = (xVec3&)globals.player.ent.model->Mat->pos;

    xVec3 dir = { 0.0f, 0.0f, 0.0f };

    dir.x = target.x - loc.x;
    dir.z = target.z - loc.z;

    F32 mag = dir.length2();

    if (mag >= -0.00001f && mag <= 0.00001f)
    {
        return;
    }

    mag = xsqrt(mag);

    xVec3 n = dir;
    n *= 1.0f / mag;

    kj.TurnToFace(dt, &n, -1.0f);
}

void zNPCGoalKJIdle::move(float dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    xMat4x3& imat = *(xMat4x3*)kj.model->Mat;
    xVec3& target = (xVec3&)globals.player.ent.model->Mat->pos;
    xVec3& vel = kj.frame->vel;

    xVec3 offset = { 0.0f, 0.0f, 0.0f };

    offset.x = target.x - imat.pos.x;
    offset.z = target.z - imat.pos.z;

    F32 dist = offset.length2();
    xVec3 dir;

    if (dist >= -0.00001f && dist <= 0.00001f)
    {
        dist = 0.0f;
        dir = xVec3::create(1.0f, 0.0f, 0.0f);
    }
    else
    {
        dist = xsqrt(dist);
        dir = offset * (1.0f / dist);
    }

    F32 maxvel = kj.cfg_npc->spd_moveMax;
    F32 a = kj.cfg_npc->fac_accelMax;

    dist -= tweak.min_dist;

    if (dist < 0.0f)
    {
        dist = -dist;
        a = -a;
    }

    xVec3 accel = dir * a;
    xVec3 newvel = vel + accel * dt;

    if (dist < 1.0f)
    {
        newvel * dist;
    }

    F32 frac = newvel.length2();

    if (frac > maxvel * maxvel)
    {
        vel = newvel * (maxvel / xsqrt(frac));
    }
    else
    {
        vel = newvel;
    }

    xVec3 displace = kj.asset->pos - imat.pos;

    F32 displace2 = displace.length2();
    F32 maxdist = tweak.move_radius;
    F32 edgedist = 0.9f * maxdist;
    F32 edgedist2 = edgedist * edgedist;

    if (displace2 > maxdist * maxdist)
    {
        xVec3 dir = displace * (1.0f / xsqrt(displace2));
        F32 d = vel.dot(dir);

        if (d < 0.0f)
        {
            vel -= dir * d;
        }
    }
    else if (displace2 > edgedist2)
    {
        F32 len = xsqrt(displace2);
        F32 frac = (len - edgedist) / (0.1f * maxdist);
        xVec3 dir = displace * (1.0f / len);
        F32 d = vel.dot(dir);

        if (d < 0.0f)
        {
            vel -= dir * (d * frac);
        }
    }
}

S32 zNPCGoalKJBored::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    //play_sound(int, const xVec3*);
    play_sound(SOUND_CHEER, (const xVec3*)&kj.model->Mat->pos);
    play_sound(SOUND_CHEER, (const xVec3*)&kj.model->Mat->pos);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJBored::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJBored::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xAnimState* anim = kj.AnimCurState();
    bool playing = false;
    for (S32 i = 0; i < 2; i++)
    {
        if (anim->ID == g_hash_subbanim[bored_anims[i]])
        {
            playing = true;
            break;
        }
    }

    if (!playing || dt > kj.AnimTimeRemain(NULL))
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_KJSHOCKGROUND;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalKJSpawnKids::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    cycle = 0;
    delay = 0.0f;
    spewed = 0;
    spawned = 0;
    spawn_count = 0;
    child_count = kj.count_children(kj.round);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJSpawnKids::Exit(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    if (spawn_count < child_count) //0x58 child_count
    {
        kj.generate_spawn_particles();
        kj.spawn_children(kj.round, child_count - spawn_count);
    }
    return zNPCGoalCommon::Exit(dt, updCtxt);
}

S32 zNPCGoalKJSpawnKids::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    delay += dt;

    if (!spawned && delay >= tweak.spawn.delay)
    {
        cycle++;

        S32 amount = cycle * child_count / tweak.spawn.cycles;

        if (amount > spawn_count)
        {
            play_sound(SOUND_BIRTH, (const xVec3*)&kj.model->Mat->pos);
            play_sound(SOUND_BIRTH, (const xVec3*)&kj.model->Mat->pos);
            kj.spawn_children(kj.round, amount - spawn_count);
            spawn_count = amount;
        }

        spawned = true;
    }

    if (!spewed && delay >= tweak.spawn.spew.delay)
    {
        kj.generate_spawn_particles();
        spewed = true;
    }

    if (spawned && spewed)
    {
        xAnimState* anim = kj.AnimCurState();

        if (anim->ID != g_hash_subbanim[ANIM_SpawnKids01] || dt > kj.AnimTimeRemain(NULL))
        {
            if (cycle >= tweak.spawn.cycles)
            {
                *trantype = GOAL_TRAN_SET;
                return NPC_GOAL_KJIDLE;
            }

            delay = 0.0f;
            spewed = false;
            spawned = false;
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalKJTaunt::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    //play_sound(int, const xVec3*);
    play_sound(SOUND_TAUNT, (const xVec3*)&kj.model->Mat->pos);
    play_sound(SOUND_TAUNT, (const xVec3*)&kj.model->Mat->pos);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJTaunt::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJTaunt::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xAnimState* anim = kj.AnimCurState();

    if (anim->ID != g_hash_subbanim[ANIM_Taunt01] || dt > kj.AnimTimeRemain(NULL))
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_KJIDLE;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

// void zNPCKingJelly::start_blink()
// {
//     blink.active = 1;
//     blink.delay = 0;
//     blink.count = 0;
//     // 0x24 model
//     // 0x44 render
// }

S32 zNPCGoalKJDamage::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xAnimState* state = kj.AnimCurState();

    if (state->ID != g_hash_subbanim[ANIM_Damage01] || dt > kj.AnimTimeRemain(NULL))
    {
        *trantype = GOAL_TRAN_SET;

        if (kj.life <= 0)
        {
            return 'NGM7';
        }

        return 'NGM3';
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalKJShockGround::Enter(F32 dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    kj.attack++;
    strikes = 0;
    kj.shockstate = zNPCKingJelly::SS_START;
    delay = tweak.thump.delay;
    play_sound(SOUND_LAND, (const xVec3*)&kj.model->Mat->pos);
    kj.disable_tentacle_damage = 1;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJShockGround::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    delay -= dt;

    switch (kj.shockstate)
    {
    case zNPCKingJelly::SS_START:
        kj.shockstate = (zNPCKingJelly::shockstate_enum)update_start(dt);
        break;
    case zNPCKingJelly::SS_WARM_UP:
        kj.shockstate = (zNPCKingJelly::shockstate_enum)update_warm_up(dt);
        break;
    case zNPCKingJelly::SS_RELEASE:
        kj.shockstate = (zNPCKingJelly::shockstate_enum)update_release(dt);
        break;
    case zNPCKingJelly::SS_COOL_DOWN:
        kj.shockstate = (zNPCKingJelly::shockstate_enum)update_cool_down(dt);
        break;
    case zNPCKingJelly::SS_STOP:
        kj.shockstate = (zNPCKingJelly::shockstate_enum)update_stop(dt);
        break;
    }

    if (kj.shockstate >= zNPCKingJelly::MAX_SS)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_KJIDLE;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalKJShockGround::Exit(F32 dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    if (kj.flag.charging != 0)
    {
        kj.end_charge();
    }
    kj.create_ambient_rings();
    kj.disable_tentacle_damage = 0;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJShockGround::update_start(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    if (delay <= 0.0f)
    {
        kj.generate_thump_particles();
        delay = 1000000000.0f;
    }

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID == g_hash_subbanim[ANIM_Attack01])
    {
        delay = tweak.interval.warm_up;
        kj.start_charge();
        return zNPCKingJelly::SS_WARM_UP;
    }

    return zNPCKingJelly::SS_START;
}

S32 zNPCGoalKJShockGround::update_warm_up(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    F32 t;
    if (tweak.interval.warm_up <= delay)
    {
        t = 1.0f;
    }
    else
    {
        t = 1.0f - delay / tweak.interval.warm_up;
    }
    kj.update_charge(t);

    if (delay > 0.0f)
    {
        return zNPCKingJelly::SS_WARM_UP;
    }

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID != g_hash_subbanim[ANIM_AttackLoop01] || dt > kj.AnimTimeRemain(NULL))
    {
        play_sound(SOUND_WAVE_RING, (const xVec3*)&kj.model->Mat->pos);
        delay = tweak.interval.release;
        kj.end_charge();
        kj.destroy_ambient_rings();
        kj.create_wave_rings();
        return zNPCKingJelly::SS_RELEASE;
    }

    return zNPCKingJelly::SS_WARM_UP;
}

S32 zNPCGoalKJShockGround::update_release(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    if (delay > 0.0f)
    {
        return zNPCKingJelly::SS_RELEASE;
    }

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID != g_hash_subbanim[ANIM_Attack01] || dt > kj.AnimTimeRemain(NULL))
    {
        kj.AnimStart(g_hash_subbanim[ANIM_AttackLoop01], 0);
        delay = tweak.interval.cool_down;
        return zNPCKingJelly::SS_COOL_DOWN;
    }

    return zNPCKingJelly::SS_RELEASE;
}

S32 zNPCGoalKJShockGround::update_cool_down(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    if (delay > 0.0f)
    {
        return zNPCKingJelly::SS_COOL_DOWN;
    }

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID != g_hash_subbanim[ANIM_AttackLoop01] || dt > kj.AnimTimeRemain(NULL))
    {
        strikes++;
        kj.create_ambient_rings();

        if (strikes >= kj.max_strikes())
        {
            play_sound(SOUND_RISE, (const xVec3*)&kj.model->Mat->pos);
            kj.AnimStart(g_hash_subbanim[ANIM_AttackEnd01], 0);
            return zNPCKingJelly::SS_STOP;
        }

        delay = tweak.interval.warm_up;
        kj.AnimStart(g_hash_subbanim[ANIM_Attack01], 0);
        kj.start_charge();
        return zNPCKingJelly::SS_WARM_UP;
    }

    return zNPCKingJelly::SS_COOL_DOWN;
}

S32 zNPCGoalKJShockGround::update_stop(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xAnimState* anim = kj.AnimCurState();

    if (anim->ID != g_hash_subbanim[ANIM_AttackEnd01] || dt > kj.AnimTimeRemain(NULL))
    {
        return zNPCKingJelly::MAX_SS;
    }

    return zNPCKingJelly::SS_STOP;
}

S32 zNPCGoalKJDamage::Enter(F32 dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    //play_sound(int, const xVec3*);
    play_sound(SOUND_GRUNT, (const xVec3*)&kj.model->Mat->pos);
    play_sound(SOUND_GRUNT, (const xVec3*)&kj.model->Mat->pos);
    kj.disable_tentacle_damage = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJDamage::Exit(F32 dt, void* updCtxt)
{
    // Needs to be a reference, casting as a pointer doesn't work.
    // Would never have gotten this if not for DWARF data.
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    kj.update_round();
    kj.disable_tentacle_damage = false;

    return xGoal::Exit(dt, updCtxt);
}

void zNPCKingJelly::update_round()
{
    if (life == 0)
    {
        round = 0;
        return;
    }

    round = 2 - ((life - 1) * 3) / tweak.max_life;
}

S32 zNPCGoalKJDeath::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    kj.decompose();
    kj.post_decompose();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJDeath::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJDeath::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void zNPCKingJelly::render_debug()
{
}

void zNPCKingJelly::on_change_ambient_ring(const tweak_info&)
{
}

void zNPCKingJelly::on_change_fade_obstructions(const tweak_info&)
{
}

xVec3 zNPCKingJelly::get_center() const
{
    return (xVec3&)model->Mat[0].pos + (xVec3&)model->Mat[2].pos + cfg_npc->off_bound;
}

xVec3* zNPCKingJelly::get_bottom() const
{
    return (xVec3*)&this->model->Mat->pos;
}

void lightning_ring::update(F32 dt)
{
    if (update_callback != NULL)
    {
        update_callback(*this, dt);
    }

    refresh();
}
