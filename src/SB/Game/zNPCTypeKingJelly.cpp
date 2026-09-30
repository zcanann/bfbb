#include "zNPCTypeKingJelly.h"

#include <types.h>
#include "xMathInlines.h"
#include "xColor.h"
#include "zNPCGoalCommon.h"
#include "zCamera.h"
#include "zMusic.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xutil.h"
#include "xGroup.h"
#include "xDebug.h"
#include "zNPCSndLists.h"
#include "string.h"

#define f1868 1.0f
#define f1869 0.0f
#define f2105 0.2f
#define f2106 0.1f

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
    
    // TODO: fix this up
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
            "KJ_Land1",
            NULL,
            NULL
        },
        {
            "KJ_Land2",
            NULL,
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

    static const S32 bored_anims[2] = { ANIM_Idle02, ANIM_Idle03 };

    static const U8 sound_flags[11] = { 0x1, 0x0, 0x0, 0x0, 0x0,
                                        0x0, 0x1, 0x1, 0x0, 0x0, 0x0};

    static xBinaryCamera boss_cam = {
        {
            { 8.0f, 4.0f, 3.0f },
            { 0.2f, 2.2f, -1.0f },
            { 1.0f, 0.2f, 1.5f },
            10.0f,
            10.0f,
            10.0f,
            10.0f,
            50.0f,
            0.0f,
        },
    };
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
        }
        else
        {
            sound_data_type& sound = sound_data[sound_index];
            if (sound.handle == 0 || !(sound_flags[sound_index] & 0x1))
            {
                sound.delayed = TRUE;
                sound.time = 0.0f;
            }
        }
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

    S32 set_ring_segments(const xVec3& center, F32 radius, F32 segment_length)
    {
        static F32 sin_lookup[9];
        static F32 cos_lookup[9];
        static U8 sclookup_inited = FALSE;

        if (!sclookup_inited)
        {
            sclookup_inited = TRUE;

            F32 angle = 0.0f;
            for (S32 i = 0; i < 9; i++)
            {
                sin_lookup[i] = isin(angle);
                cos_lookup[i] = icos(angle);

                const F32 angle_step = PI / 32.0f;
                angle += angle_step;
            }
        }

        S32 size = 0.5f + ((2.0f * radius * PI) / segment_length);
        size = (size + 7) & ~7;

        if (size > 64)
        {
            size = 64;
        }
        if (size <= 0)
        {
            return 0;
        }

        for (S32 i = 0; i < size; i++)
        {
            // TODO: what is here
        }

        return size;
    }

    void updown_ring_update(lightning_ring& ring, F32 dt)
    {
        ring.current.time += ring.current.accel * dt;
        ring.current.time = xfmod(ring.current.time, ring.delay);

        const F32 t = ring.current.time / ring.delay;
        const F32 range = ring.max_height - ring.min_height;
        const F32 wave = 1.0f + isin(PI * t);

        ring.current.height = ring.min_height + (0.5f * (range * wave));
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
    
    void kill_sound(S32 which)
    {
        sound_data_type& sound = sound_data[which];
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

    F32 lerp(F32 t, F32 a, F32 b)
    {
        return t * (b - a) + a;
    }

    U8 lerp(F32 t, U8 a, U8 b)
    {
        return 0.5f + (t * ((F32)b - (F32)a) + (F32)a);
    }

    iColor_tag lerp(F32 t, iColor_tag a, iColor_tag b)
    {
        iColor_tag c;
        c.r = lerp(t, a.r, b.r);
        c.g = lerp(t, a.g, b.g);
        c.b = lerp(t, a.b, b.b);
        c.a = lerp(t, a.a, b.a);
        return c;
    }

    void set_model_color(xModelInstance* model, F32 r, F32 g, F32 b, F32 a)
    {
        while (model != NULL)
        {
            model->Flags |= 0x4000;
            model->RedMultiplier = r;
            model->GreenMultiplier = g;
            model->BlueMultiplier = b;
            model->Alpha = a;
            model = model->Next;
        }
    }

    void reset_model_color(xModelInstance* model)
    {
        while (model != NULL)
        {
            model->Flags &= 0xBFFF;
            model->RedMultiplier = 1.0f;
            model->GreenMultiplier = 1.0f;
            model->BlueMultiplier = 1.0f;
            model->Alpha = 1.0f;
            model = model->Next;
        }
    }

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
            blink.color.a = 0.0f;
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
            auto_tweak::load_param<F32, F32>(wave_ring.particles, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "wave_ring.particles");
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
            auto_tweak::load_param<U8, U8>(wave_ring.unit[0].line, 0, 0, 0, ap, apsize,
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
            auto_tweak::load_param<U8, U8>(wave_ring.unit[1].line, 0, 0, 0, ap, apsize,
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
            auto_tweak::load_param<U8, U8>(wave_ring.unit[2].line, 0, 0, 0, ap, apsize,
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
            wave_ring.unit[2].color = xColorFromRGBA(255, 255, 255, 255);
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
            auto_tweak::load_param<U8, U8>(wave_ring.unit[3].line, 0, 0, 0, ap, apsize,
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
            auto_tweak::load_param<F32, F32>(ambient_ring.segment_length, 1.0f, 0.0f, 10.0f, ap,
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
        if (init)
        {
            sound[SOUND_AMBIENT_RING].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_AMBIENT_RING].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_AMBIENT_RING].delay");
        }
        if (init)
        {
            sound[SOUND_AMBIENT_RING].radius_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_AMBIENT_RING].radius_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_AMBIENT_RING].radius_inner");
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
        if (init)
        {
            sound[SOUND_WAVE_RING].radius_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_WAVE_RING].radius_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_WAVE_RING].radius_outer");
        }
        if (init)
        {
            sound[SOUND_WAVE_RING].priority = 0;
            auto_tweak::load_param<S32, S32>(sound[SOUND_WAVE_RING].priority, 1, 0, 1000, ap,
                                             apsize, "sound[SOUND_WAVE_RING].priority");
        }
    }

    S32 sphere_hits_sphere_xz(const xSphere& a, const xSphere& b)
    {
        F32 dx = b.center.x - a.center.x;
        F32 dz = b.center.z - a.center.z;
        F32 outer = b.r + a.r;
        F32 inner = b.r - a.r;
        F32 dist2 = dx * dx + dz * dz;

        if (dist2 > outer * outer)
        {
            return 4;
        }

        if (dist2 < inner * inner)
        {
            return 2;
        }

        return 1;
    }
} // namespace

void lightning_ring::update(F32 dt)
{
    if (update_callback != NULL)
    {
        update_callback(*this, dt);
    }
    refresh();
}

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

    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle01], 0x10, 0, f1868, NULL, NULL, f1869, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle02], 0x20, 0, f1868, NULL, NULL, f1869, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle03], 0x20, 0, f1868, NULL, NULL, f1869, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Taunt01], 0x20, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Attack01], 0x10, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackWindup01], 0x20, 0, f1868, NULL, NULL,
                       f1869, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackLoop01], 0x10, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackEnd01], 0x20, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Damage01], 0x20, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_SpawnKids01], 0x10, 0, f1868, NULL, NULL, f1869,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_subbanim, ourAnims, 1, f2105);

    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Attack01], 0, 0, 0x10, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Attack01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01],
                            g_strz_subbanim[ANIM_AttackLoop01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_AttackEnd01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle02], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle03], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Taunt01], g_strz_subbanim[ANIM_Damage01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01], g_strz_subbanim[ANIM_Damage01],
                            0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackEnd01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_SpawnKids01],
                            g_strz_subbanim[ANIM_Damage01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle02], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Idle03], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_Taunt01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_Taunt01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Attack01], g_strz_subbanim[ANIM_Taunt01], 0,
                            0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackEnd01], g_strz_subbanim[ANIM_Taunt01],
                            0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_SpawnKids01], g_strz_subbanim[ANIM_Taunt01],
                            0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_Damage01],
                            g_strz_subbanim[ANIM_SpawnKids01], 0, 0, 0, 0, 0, 0, 0, 0, f2106, 0);

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
    memset(&flag, 0, sizeof(flag));
    boss_cam.init();
}

const xVec3& zNPCKingJelly::get_bottom() const
{
    return *(const xVec3*)&this->model->Mat->pos;
}

xVec3 zNPCKingJelly::get_center() const
{
    return *(xVec3*)&model->Mat->pos + *(xVec3*)&model->Mat[2].pos + cfg_npc->off_bound;
}

F32 zNPCKingJelly::get_variance() const
{
    return tweak.interval.variance * (2.0f * xurand() - 1.0f);
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
    // u32 i
}

void zNPCKingJelly::Destroy()
{
    decompose();
    post_decompose();
    zNPCCommon::Destroy();
}

void zNPCKingJelly::BUpdate(xVec3* pos)
{
    xVec3& subloc = *(xVec3*)&model->Mat[2].pos;
    xVec3 loc = *pos + subloc;
    zNPCCommon::BUpdate(&loc);
}

S32 zNPCKingJelly::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                            xBase* toParamWidget, S32* handled)
{
    switch (toEvent)
    {
    case 0x1b5:
        start_fight();
        break;
    case 0x1b9:
        break;
    case 0x1d9:
        psy_instinct->GoalSet(NPC_GOAL_KJDEATH, 1);
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

    static tweak_callback cb_fade_obstructions = {
        (void (*)(tweak_info&))on_change_fade_obstructions
    };
    static tweak_callback cb_ambient_ring = { (void (*)(tweak_info&))on_change_ambient_ring };

    tweak.context = this;
    tweak.cb_fade_obstructions = &cb_fade_obstructions;
    tweak.cb_ambient_ring = &cb_ambient_ring;
    tweak.load(parmdata, pdatsize);
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

U32 zNPCKingJelly::AnimPick(S32 rawgoal, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 anim = 0;
    S32 index;

    switch (rawgoal)
    {
    case NPC_GOAL_KJIDLE:
        index = ANIM_Idle01;
        break;
    case NPC_GOAL_KJBORED:
        index = xUtil_choose<S32>(bored_anims, 2, NULL);
        break;
    case NPC_GOAL_KJSPAWNKIDS:
        index = ANIM_SpawnKids01;
        break;
    case NPC_GOAL_KJTAUNT:
        index = ANIM_Taunt01;
        break;
    case NPC_GOAL_KJSHOCKGROUND:
        index = ANIM_AttackWindup01;
        break;
    case NPC_GOAL_KJDAMAGE:
        index = ANIM_Damage01;
        break;
    case NPC_GOAL_KJDEATH:
        index = -1;
        break;
    default:
        index = ANIM_Idle01;
        break;
    }

    if (index > -1)
    {
        anim = g_hash_subbanim[index];
    }

    return anim;
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
        U32 size = xGroupGetCount((xGroup*)&child);
        for (; i < size; ++i)
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

void zNPCKingJelly::ParseLinks()
{
    zNPCCommon::ParseLinks();

    xLinkAsset* it = link;
    xLinkAsset* end = it + linkCount;
    for (; it != end; ++it)
    {
        if (it->dstEvent == 0x133)
        {
            add_child(*zSceneFindObject(it->dstAssetID), (S32)it->param[0]);
        }
    }
}

void zNPCKingJelly::start_fight()
{
    if (flag.fighting)
    {
        return;
    }

    flag.fighting = true;
    show_attack_model();
    fade_curtain();
    play_sound(0, (xVec3*)&model->Mat->pos);
    play_sound(7, (xVec3*)&model->Mat->pos);
    zMusicSetVolume(tweak.music_fade, tweak.music_fade_delay);
    zCameraDisableTracking(CO_BOSS);
    boss_cam.start(globals.camera);
    boss_cam.set_targets(*(xVec3*)&globals.player.ent.model->Mat->pos, bound.sph.center,
                         bound.sph.r);
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

void zNPCKingJelly::taunt()
{
    switch (psy_instinct->GIDOfActive())
    {
    case NPC_GOAL_KJTAUNT:
    case NPC_GOAL_KJDAMAGE:
    case NPC_GOAL_KJDEATH:
        break;
    default:
        psy_instinct->GoalSet(NPC_GOAL_KJTAUNT, 1);
        break;
    }
}

void zNPCKingJelly::check_player_damage()
{
    if (globals.player.cheat_mode != 0)
    {
        return;
    }

    if (apply_wave_damage() || apply_tentacle_damage() || apply_ambient_damage())
    {
        return;
    }
}

S32 zNPCKingJelly::count_children(S32 wave)
{
    S32 count = 0;
    for (U32 i = 0; i < children_size; ++i)
    {
        if (children[i].wave == wave)
        {
            ++count;
        }
    }
    return count;
}

void zNPCKingJelly::render_debug()
{
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

    for (U32 i = 0; i < children_size; ++i)
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

void zNPCKingJelly::refresh_tentacle_points()
{
    S32 tempvar = 0;
    do
    {
        refresh_tentacle_points(tempvar);
        tempvar = tempvar + 1;
    } while (tempvar < 7);
}

void zNPCKingJelly::destroy_ambient_rings()
{
    for (S32 i = 0; i < 3; i++)
    {
        ambient_rings[i].destroy();
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

void zNPCKingJelly::load_model()
{
    U32 i = 0;
    xModelInstance* m = model;
    while (m != NULL && i < 4)
    {
        submodel[i] = m;
        ++i;
        m = m->Next;
    }

    submodel[0]->Data->boundingSphere.radius += 10.0f;
    submodel[1]->Data->boundingSphere.radius += 10.0f;
    submodel[3]->Data->boundingSphere.radius += 10.0f;
    submodel[2]->Data->boundingSphere.radius += 10.0f;

    submodel[3]->Flags |= 0x2;
    submodel[3]->Flags |= 0x1;
}

void zNPCKingJelly::load_curtain_model()
{
    char name[20] = "SHOWER CURTAIN SIMP";
    curtain_ent = (zEnt*)zSceneFindObject(xStrHash(name));

    xModelInstance* m = curtain_ent->model;
    for (U32 i = 0; m != NULL && i < 5; ++i)
    {
        curtain_model[i] = m;
        m = m->Next;
    }
}

void zNPCKingJelly::show_attack_model()
{
    submodel[0]->Flags |= 0x2;
    submodel[0]->Flags &= ~0x1;
    submodel[1]->Flags &= 0xFFFD;
    submodel[1]->Flags &= ~0x1;
    submodel[2]->Flags |= 0x2;
    submodel[2]->Flags |= 0x1;
}

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

void zNPCKingJelly::show_shower_model()
{
    submodel[0]->Flags |= 0x2;
    submodel[0]->Flags |= 0x1;
    submodel[1]->Flags |= 0x2;
    submodel[1]->Flags |= 0x1;
    submodel[2]->Flags &= 0xFFFD;
    submodel[2]->Flags &= ~0x1;
}

void zNPCKingJelly::reset_curtain()
{
    curtain_model[2]->Alpha = curtain_model[4]->Alpha = 1.0f;
}

void zNPCKingJelly::fade_curtain()
{
    curtain_model[2]->Alpha = curtain_model[4]->Alpha = tweak.fade_obstructions;
}

S32 zNPCGoalKJIdle::Enter(F32 dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    attack_delay = tweak.interval.attack[kj.round] + kj.get_variance();
    kj.flag.stop_moving = false;
    play_sound(6, (xVec3*)&kj.model->Mat->pos);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJIdle::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
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

S32 zNPCGoalKJIdle::Exit(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    kill_sound(6);
    kj.flag.stop_moving = 1;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJBored::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    play_sound(3, (xVec3*)&kj.model->Mat->pos);
    play_sound(3, (xVec3*)&kj.model->Mat->pos);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJBored::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    xAnimState* anim = kj.AnimCurState();

    bool found = false;
    for (S32 i = 0; i < 2; i++)
    {
        if (anim->ID == g_hash_subbanim[bored_anims[i]])
        {
            found = true;
            break;
        }
    }

    if (!found || dt > kj.AnimTimeRemain(NULL))
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_KJSHOCKGROUND;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalKJBored::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKJSpawnKids::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    cycle = 0;
    delay = 0.0f;
    spewed = FALSE;
    spawned = FALSE;
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

S32 zNPCGoalKJTaunt::Enter(float dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    play_sound(9, (xVec3*)&kj.model->Mat->pos);
    play_sound(9, (xVec3*)&kj.model->Mat->pos);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJTaunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
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

S32 zNPCGoalKJTaunt::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

void zNPCKingJelly::start_blink()
{
    blink.active = TRUE;
    blink.delay = 0.0f;
    blink.count = 0;
    model->Flags |= 0x4000;
}

S32 zNPCGoalKJDamage::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    xAnimState* anim = kj.AnimCurState();

    if (anim->ID != g_hash_subbanim[ANIM_Damage01] || dt > kj.AnimTimeRemain(NULL))
    {
        *trantype = GOAL_TRAN_SET;
        if (kj.life <= 0)
        {
            return NPC_GOAL_KJDEATH;
        }
        else
        {
            return NPC_GOAL_KJSPAWNKIDS;
        }
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
    play_sound(5, (xVec3*)&kj.model->Mat->pos);
    kj.disable_tentacle_damage = TRUE;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKJShockGround::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    delay -= dt;

    switch (kj.shockstate)
    {
    case zNPCKingJelly::SS_START:
        kj.shockstate = update_start(dt);
        break;
    case zNPCKingJelly::SS_WARM_UP:
        kj.shockstate = update_warm_up(dt);
        break;
    case zNPCKingJelly::SS_RELEASE:
        kj.shockstate = update_release(dt);
        break;
    case zNPCKingJelly::SS_COOL_DOWN:
        kj.shockstate = update_cool_down(dt);
        break;
    case zNPCKingJelly::SS_STOP:
        kj.shockstate = update_stop(dt);
        break;
    }

    if (kj.shockstate >= zNPCKingJelly::MAX_SS)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_KJIDLE;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

zNPCKingJelly::shockstate_enum zNPCGoalKJShockGround::update_start(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    if (delay <= 0.0f)
    {
        kj.generate_thump_particles();
        delay = 1e9f;
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

zNPCKingJelly::shockstate_enum zNPCGoalKJShockGround::update_warm_up(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    kj.update_charge(tweak.interval.warm_up <= delay ? 1.0f
                                                     : 1.0f - delay / tweak.interval.warm_up);

    if (delay > 0.0f)
    {
        return zNPCKingJelly::SS_WARM_UP;
    }

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID != g_hash_subbanim[ANIM_AttackLoop01] || dt > kj.AnimTimeRemain(NULL))
    {
        play_sound(10, (xVec3*)&kj.model->Mat->pos);
        delay = tweak.interval.release;
        kj.end_charge();
        kj.destroy_ambient_rings();
        kj.create_wave_rings();
        return zNPCKingJelly::SS_RELEASE;
    }

    return zNPCKingJelly::SS_WARM_UP;
}

zNPCKingJelly::shockstate_enum zNPCGoalKJShockGround::update_release(F32 dt)
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

zNPCKingJelly::shockstate_enum zNPCGoalKJShockGround::update_cool_down(F32 dt)
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
            play_sound(8, (xVec3*)&kj.model->Mat->pos);
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

zNPCKingJelly::shockstate_enum zNPCGoalKJShockGround::update_stop(F32 dt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;

    xAnimState* anim = kj.AnimCurState();
    if (anim->ID != g_hash_subbanim[ANIM_AttackEnd01] ||
        dt > kj.AnimTimeRemain(NULL))
    {
        return zNPCKingJelly::MAX_SS;
    }

    return zNPCKingJelly::SS_STOP;
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

S32 zNPCGoalKJDamage::Enter(F32 dt, void* updCtxt)
{
    zNPCKingJelly& kj = *(zNPCKingJelly*)this->psyche->clt_owner;
    play_sound(4, (xVec3*)&kj.model->Mat->pos);
    play_sound(4, (xVec3*)&kj.model->Mat->pos);
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
    }
    else
    {
        round = 2 - (life - 1) * 3 / tweak.max_life;
    }
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
