#include "zEntPlayerBungeeState.h"

#include "iCollide.h"
#include "iScrFX.h"
#include "xScrFx.h"
#include "xCamera.h"
#include "xDebug.h"
#include "xFX.h"
#include "xLinkAsset.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "xMath3.h"
#include "xMarkerAsset.h"
#include "xModel.h"
#include "xPad.h"
#include "xShadow.h"
#include "xString.h"
#include "xVec3.h"

#include "xstransvc.h"
#include "zBase.h"
#include "zCamera.h"
#include "zEntCruiseBubble.h"
#include "zEntPlayer.h"
#include "zGameExtras.h"
#include "zGlobals.h"
#include "zCameraTweak.h"

#include "zLightning.h"
#include "zScene.h"
#include "zGrid.h"
#include "zMusic.h"
#include "zEntPickup.h"
#include "zNPCTypeCommon.h"
#include "zSurface.h"
#include "zTextBox.h"
#include "zEntButton.h"
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cstring>
#include <types.h>


// The declaration now lives in xMath3.h; the body has to stay here. Moving the
// inline into the header makes zDiscoFloor.cpp expand it, and that emits weak
// __apl__5xVec3Ff / __ami__5xVec3Ff copies into zDiscoFloor.o that retail's
// zDiscoFloor.o does not contain. Measured, not assumed.
inline void xBoxFromSphere(xBox& box, const xSphere& o)
{
    box.upper = box.lower = o.center;
    box.upper += o.r;
    box.lower -= o.r;
}

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
void __deadstripped_zEntPlayerBungeeState()
{
    const char _405[0x0C] = {};
    const char _406[0x0C] = {};
    const char _410[0x0C] = {};
    const char _441[0x0C] = {};

    const char _624[0x28] = {};
    const char _625[0x28] = {};
    const char _626[0x28] = {};
    const char _627[0x28] = {};
    const char _628[0x28] = {};
    const char _629[0x28] = {};
    const char _630[0x28] = {};
}

const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };
const basic_rect<F32> default_adjust = { 0.0f, 0.0f, 1.0f, 1.0f };

namespace bungee_state
{
    namespace
    {

        static struct
        {
            F32 bottom_anim_frac;
            F32 top_anim_frac;
            F32 bottom_anim_time;
            F32 top_anim_time;
            F32 hit_anim_time;
            F32 damage_rot;
            F32 death_time;
            F32 vel_blur;
            F32 fade_dist;
            F32 player_radius;
            F32 hook_fade_alpha;
            F32 hook_fade_time;
            struct
            {
                F32 edge_zone;
                F32 sway;
                F32 decay;
            } horizontal; // offset 0x30, size 0xC
            struct
            {
                F32 time;
                F32 anim_out_time;
                F32 min_dist;
                F32 max_dist;
            } dive; // offset 0x3C, size 0x10
            struct
            {
                F32 speed;
            } camera; // offset 0x4C, size 0x4
            struct
            {
                F32 spring;
                F32 decay;
            } turn; // offset 0x50, size 0x8
        } fixed;
        static struct
        {
            S32 flags;
            class state_type* state;
            class state_type* states[2];
            struct
            {
                xAnimTransition* start;
                xAnimTransition* rise;
                xAnimTransition* fall;
                xAnimTransition* stop;
                xAnimTransition* dive_start;
                xAnimTransition* dive_stop;
                xAnimTransition* top_start;
                xAnimTransition* top_stop;
                xAnimTransition* bottom_start;
                xAnimTransition* bottom_stop;
                xAnimTransition* hit;
            } anim_tran; // offset 0x10, size 0x2C
            class hook_type* hook;
            class hook_type* hook_cache[8];
            const drop_asset* drop_cache[32];
            xMarkerAsset* drop_marker_cache[32];
            S32 hook_cache_size;
            S32 drop_cache_size;
            xModelInstance* root_model;
            xModelInstance* ass_model;
            xModelInstance* pants_model;
            xVec3 hook_loc;
            xVec3 drop_loc;
            bool drop_set_view_angle;
            F32 drop_view_angle;
            F32 dismount_delay;
            S32 anim_state;
        } shared = { 1 }; // size: 0x19C, address: 0x4DF7E0

        static F32 old_pants_clip_radius;

        struct sound_group
        {
            S32 first;
            S32 last;
            bool streamed;
        };
        static const sound_group sound_groups[9] = {
            { ePlayerSnd_BungeeAttach, ePlayerSnd_BungeeAttach, false },
            { ePlayerStreamSnd_BungeeAttachComment, ePlayerStreamSnd_BungeeAttachComment, true },
            { ePlayerSnd_BungeeRelease, ePlayerSnd_BungeeRelease, false },
            { ePlayerSnd_BungeeDive1, ePlayerSnd_BungeeDive2, false },
            { ePlayerSnd_Invalid, -1, false },
            { ePlayerSnd_BeginBungee, ePlayerSnd_BeginBungee, false },
            { ePlayerSnd_OuchStart, ePlayerSnd_OuchEnd, false },
            { ePlayerSnd_OuchStart, ePlayerSnd_OuchEnd, false },
            { ePlayerStreamSnd_BungeeBeginDeath, ePlayerStreamSnd_BungeeEndDeath, true },
        };
        enum sound_enum
        {
            SOUND_INVALID = -1,
            BEGIN_SOUND = 0,
            SOUND_ATTACH = 0,
            SOUND_ATTACH_COMMENT = 1,
            SOUND_DETACH = 2,
            SOUND_DIVE = 3,
            SOUND_PEAK = 4,
            SOUND_WIND_LOOP = 5,
            SOUND_THUMP = 6,
            SOUND_DAMAGE = 7,
            SOUND_DEATH = 8,
            END_SOUND = 9,
            MAX_SOUND = 9,
        };

        static void play_sound(sound_enum which, F32 delay)
        {
            const sound_group& sg = sound_groups[which];
            if (sg.first > sg.last)
            {
                return;
            }
            if (sg.first == sg.last)
            {
                if (sg.streamed)
                {
                    zEntPlayer_SNDPlayStreamRandom((_tagePlayerStreamSnd)sg.first,
                                                   (_tagePlayerStreamSnd)sg.last, delay);
                }
                else
                {
                    zEntPlayer_SNDPlay((_tagePlayerSnd)sg.first, delay);
                }
            }
            else
            {
                if (sg.streamed)
                {
                    zEntPlayer_SNDPlayStreamRandom((_tagePlayerStreamSnd)sg.first,
                                                   (_tagePlayerStreamSnd)sg.last, delay);
                }
                else
                {
                    zEntPlayer_SNDPlayRandom((_tagePlayerSnd)sg.first, (_tagePlayerSnd)sg.last,
                                             delay);
                }
            }
        }
        static void stop_sound(S32 which)
        {
            const sound_group& sg = sound_groups[which];
            if (sg.first > sg.last)
            {
                return;
            }
            if (sg.streamed)
            {
                return;
            }
            for (S32 i = sg.first; i <= sg.last; ++i)
            {
                zEntPlayer_SNDStop((_tagePlayerSnd)i);
            }
        }
        static void set_volume(S32 which, F32 new_vol)
        {
            const sound_group& sg = sound_groups[which];
            if (sg.first > sg.last)
            {
                return;
            }
            if (sg.streamed)
            {
                return;
            }
            for (S32 i = sg.first; i <= sg.last; ++i)
            {
                zEntPlayer_SNDSetVol((_tagePlayerSnd)i, new_vol);
            }
        }
        static void init_models()
        {
            shared.root_model = globals.player.ent.model;
            shared.ass_model = globals.player.sb_models[3];
            shared.pants_model = globals.player.sb_models[4];
            xModelInstanceUpgradeBrotherShared(globals.player.sb_models[4],
                                               globals.player.sb_models[4]->Flags & 0xdfff | 8);
        }
        static void show_models()
        {
            shared.ass_model->Flags |= 0x3;
            shared.pants_model->Flags &= 0xfff7;
            shared.pants_model->Flags |= 0x22;
            old_pants_clip_radius = shared.pants_model->Data->boundingSphere.radius;
            shared.pants_model->Data->boundingSphere.radius = 500.0f;
        }
        static void hide_models()
        {
            shared.pants_model->Flags &= 0xffdc;
            shared.pants_model->Flags |= 0x8;
            shared.pants_model->Data->boundingSphere.radius = old_pants_clip_radius;

            if (zGameExtras_CheatFlags() & 0x10000000)
            {
                shared.ass_model->Flags |= 0x3;
            }
            else
            {
                shared.ass_model->Flags &= ~0x3;
            }
        }
        static void render_player(bool fade)
        {
            xShadowRender(&globals.player.ent, 100.0f);
            xEntRender(&globals.player.ent);

            if (fade)
            {
                xFXRenderProximityFade(*shared.pants_model, 1.0f, fixed.fade_dist);
            }
        }
        static void move_wedgie(const xVec3& stretch_loc)
        {
            if (shared.ass_model == NULL || shared.pants_model == NULL)
            {
                return;
            }
            shared.pants_model->Mat[2] = shared.root_model->Mat[2];

            xMat4x3 tm;
            xMat4x3 mworld;
            xMat4x3 mlocal;
            xMat4x3OrthoInv(&tm, (const xMat4x3*)shared.root_model->Mat);
            xMat3x3Identity(&mworld);
            mworld.pos = stretch_loc;

            static xVec3 tweak_cord_off = { 0 };
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                xDebugAddTweak("Bungee|Hook|temp xoff", &tweak_cord_off.x, -10000.0f, 10000.0f,
                               NULL, NULL, 0);
                xDebugAddTweak("Bungee|Hook|temp yoff", &tweak_cord_off.y, -10000.0f, 10000.0f,
                               NULL, NULL, 0);
                xDebugAddTweak("Bungee|Hook|temp zoff", &tweak_cord_off.z, -10000.0f, 10000.0f,
                               NULL, NULL, 0);
            }
            mworld.pos += tweak_cord_off;
            xMat4x3Mul(&mlocal, &mworld, &tm);
            shared.pants_model->Mat[34] = *(RwMatrix*)&mlocal;
        }

        static void update_hook_loc()
        {
            if (shared.hook == NULL || shared.hook->ent == NULL)
            {
                return;
            }

            xVec3* hook_pos = xEntGetPos(shared.hook->ent);
            shared.hook_loc = *hook_pos + shared.hook->asset->center;
        }

        static bool find_drop_off()
        {
            S32 idx = -1;
            F32 closest = SQR(shared.hook->asset->detach.dist);
            for (S32 i = 0; i < shared.drop_cache_size; ++i)
            {
                xVec3 d = shared.drop_marker_cache[i]->pos - shared.hook_loc;
                F32 len2 = d.length2();
                if (len2 >= closest)
                {
                    continue;
                }
                idx = i;
                closest = len2;
            }

            if (idx != -1)
            {
                shared.drop_loc = shared.drop_marker_cache[idx]->pos;
                shared.drop_set_view_angle = shared.drop_cache[idx]->set_view_angle;
                shared.drop_view_angle = shared.drop_cache[idx]->view_angle;
                return true;
            }
            return false;
        }
        static void trigger(U32 toEvent)
        {
            zEntEvent(shared.hook, shared.hook, toEvent);
        }

        enum state_enum
        {
            STATE_INVALID = -1,
            BEGIN_STATE = 0,
            STATE_ATTACHING = 0,
            STATE_HANGING = 1,
            END_STATE = 2,
            MAX_STATE = 2,
        };
        class state_type
        {
            // total size: 0x8
        public:
            state_type(state_enum type) : type(type)
            {
            }

            state_enum type;

            virtual void start()
            {
            }
            virtual void stop()
            {
            }
            virtual state_enum update(xScene& s, F32& dt) = 0;
            virtual void render()
            {
            }
        };
        class hanging_state_type : public state_type
        {
        public:
            class ent_info
            {
                // total size: 0x8
            public:
                class xEnt* ent; // offset 0x0, size 0x4
                signed int hits; // offset 0x4, size 0x4
            };
            class env_info
            {
                // total size: 0xC
            public:
                class xEnv* env; // offset 0x0, size 0x4
                unsigned char collide; // offset 0x4, size 0x1
                signed int hits; // offset 0x8, size 0x4
            };
            struct cb_cache_collisions
            {
                const xSphere& o;
                ent_info* ent_cache;
                S32& ent_cache_size;

                cb_cache_collisions(const xSphere& o, ent_info* ent_cache, S32& ent_cache_size);

                bool operator()(xEnt&, xGridBound&);
            };
            hanging_state_type() : state_type(STATE_HANGING)
            {
            }

            xVec3 loc;
            xVec3 vel;
            xVec3 last_loc;
            xVec3 last_hook_loc;
            xVec3 cam_loc;
            xVec3 cam_vel;
            xVec3 cam_dir;
            xVec3 cam_dir_vel;
            F32 dive_remaining;
            F32 rot;
            F32 rot_vel;
            xVec2 stick_loc;
            F32 stick_ang;
            F32 stick_mag;
            F32 stick_frac;
            xVec3 collide_accel;
            F32 roll_offset;
            bool detaching;
            xVec3 drop_off_vel;
            F32 max_yvel;
            bool dying;
            F32 control_lag_timer;
            F32 control_lag_max;
            bool has_dived;
            bool can_dive;
            U32 last_health;
            struct
            {
                F32 time;
                F32 end_time;
                xVec3 start_loc;
                xVec3 end_loc;
                xQuat start_dir;
                xQuat end_dir;
            } detach; // offset 0xC0, size 0x40
            xModelInstance* root_model;
            xModelInstance* ass_model;
            xModelInstance* pants_model;
            hook_asset h;
            struct
            {
                struct
                {
                    F32 rest_dist;
                    F32 emax;
                    F32 spring;
                    F32 alpha;
                    F32 omega;
                } vertical; // offset 0x0, size 0x14
                struct
                {
                    F32 vscale;
                    F32 hscale;
                    F32 roll_decay;
                } camera; // offset 0x14, size 0xC
            } eh; // offset 0x198, size 0x20
            ent_info ent_cache[256];
            S32 ent_cache_size;
            env_info env_cache;

            static void on_tweak_collision(const tweak_info& ti);
            void reset_props_collision();
            static void on_tweak_camera(const tweak_info& ti);
            void reset_props_camera();
            static void on_tweak_horizontal(const tweak_info& ti);
            void reset_props_horizontal();
            static void on_tweak_vertical(const tweak_info& ti);
            void reset_props_vertical();

            F32 spring_velocity(F32 x, F32 v, F32 e, F32 k, F32 g, F32 xc) const;
            F32 spring_potential_energy(F32 x, F32 k, F32 g, F32 xc) const;
            F32 spring_potential_energy(F32, F32) const;
            F32 kinetic_energy(F32 v) const;
            F32 find_spring_min(F32 min_dist, F32 max_dist, F32 gravity, F32 damp) const;

            void allow_dive(bool allowed);

            void update_vmovement(F32 dt);
            void calc_movement(F32& r3, F32& r4, F32& r5, F32 f1, F32 f2, F32 f3, F32 f4, F32 f5,
                               F32 f6);

            void update_heading(F32 dt);
            void update_animation(F32 dt);
            xSphere player_bound() const;
            void update_sound(F32 dt);
            void update_blur(F32 dt);
            xVec3 local_to_world(const xVec3& vl) const;
            S32 detach_update(xScene& scene, F32& dt);
            void update_detach_camera(F32 dt);
            void start_detaching();
            void calc_drop_off_velocity(xVec3& v, const xVec3& from, const xVec3& to, F32 g, F32 t);
            F32 spring_energy(F32 x, F32 v, F32 k, F32 g, F32 xc) const;

            void init_camera();
            xVec3 world_to_local(const xVec3& vw) const;
            void show_help();
            void hide_help();
            void check_damage(bool thump);
            void short_drop_sudden_stop();
            void multiply_dampening(F32 s);
            void update_collision(xScene& s, F32 dt);
            bool repath(const xScene& s);
            bool boundary_repath(const xScene& s);
            bool hit_boundary(xVec3& norm, xVec3& depen, const xVec3& v) const;
            S32 clip_nearest(F32& norm, F32& depen, F32 x, F32 min, F32 max) const;
            bool env_repath(const xScene& s);
            void trigger_collision(env_info& ei, F32 mag, const xCollis& coll);
            void ouchie(bool thump);
            bool ents_repath(const xScene& s);
            F32 trigger_collision(ent_info& ei, F32 mag, const xCollis& coll);
            bool collide(xCollis& coll, const xSphere& o, const xEnt& ent) const;
            void collide_start(xScene& s);
            void update_camera(F32 dt);
            void update_camera_direction(F32 dt);
            void rotate_camera();
            void interpolate_camera_loc(const xVec3& dest, F32 dt);
            bool update_free_look(F32 dt);
            void update_movement(F32 dt);
            void update_hmovement(F32 dt);
            void update_hmovement(F32& x, F32& v, F32 x0, F32 v0, F32 dt, F32 stick);

            virtual void start();
            virtual void stop();
            virtual state_enum update(xScene& s, F32& dt);
            virtual void render();
        };
        class attaching_state_type : public state_type
        {
        public:
            attaching_state_type() : state_type(STATE_ATTACHING)
            {
            }

            xVec3* loc;
            xVec3* vel;
            xVec3 last_hook_loc;
            xVec3 hook_vel;
            F32 time_left;
            F32 time;
            F32 end_time;
            xVec3 player_loc;
            xVec3 player_vel;

            virtual void start();
            virtual void stop();
            virtual state_enum update(xScene& s, F32& dt);
            virtual void render();
        };

        void hanging_state_type::on_tweak_collision(const tweak_info& ti)
        {
            reinterpret_cast<hanging_state_type*>(ti.context)->reset_props_collision();
        }
        void hanging_state_type::reset_props_collision()
        {
            if (shared.hook == NULL)
            {
                return;
            }
            h.collision = shared.hook->asset->collision;

            if (h.collision.damage_velocity == 0.0f)
            {
                h.collision.damage_velocity = FLOAT_MIN;
            }
            else
            {
                h.collision.damage_velocity *= max_yvel;
            }

            if (h.collision.hit_velocity == 0.0f)
            {
                h.collision.hit_velocity = FLOAT_MIN;
            }
            else
            {
                h.collision.hit_velocity *= max_yvel;
            }
        }
        void hanging_state_type::on_tweak_camera(const tweak_info& ti)
        {
            reinterpret_cast<hanging_state_type*>(ti.context)->reset_props_camera();
        }
        void hanging_state_type::reset_props_camera()
        {
            if (shared.hook == NULL)
            {
                return;
            }
            h.camera = shared.hook->asset->camera;

            h.camera.vel_scale /= max_yvel;
            h.camera.view_angle *= DEG2RAD(1.0f);
            h.camera.offset_dir *= DEG2RAD(1.0f);
            h.camera.offset_dir -= h.camera.view_angle;
            eh.camera.roll_decay = 1.0f - h.camera.roll_speed;
        }
        void hanging_state_type::on_tweak_horizontal(const tweak_info& ti)
        {
            reinterpret_cast<hanging_state_type*>(ti.context)->reset_props_horizontal();
        }
        void hanging_state_type::reset_props_horizontal()
        {
            if (shared.hook == NULL)
            {
                return;
            }
            h.horizontal = shared.hook->asset->horizontal;
        }
        void hanging_state_type::on_tweak_vertical(const tweak_info& ti)
        {
            reinterpret_cast<hanging_state_type*>(ti.context)->reset_props_vertical();
        }
        void hanging_state_type::reset_props_vertical()
        {
            if (shared.hook == NULL)
            {
                return;
            }
            h.vertical = shared.hook->asset->vertical;

            h.vertical.gravity *= -1.0f * h.vertical.frequency * h.vertical.frequency;
            h.vertical.dive *= h.vertical.gravity;
            h.vertical.damp = range_limit(h.vertical.damp, 0.0f, 1.0f);
            h.vertical.min_dist *= -1.0f;
            h.vertical.max_dist *= -1.0f;
            h.vertical.min_dist = find_spring_min(h.vertical.min_dist, h.vertical.max_dist,
                                                  h.vertical.gravity, h.vertical.damp);
            eh.vertical.rest_dist = 0.5f * (h.vertical.max_dist + h.vertical.min_dist);
            eh.vertical.spring = h.vertical.gravity / eh.vertical.rest_dist;
            eh.vertical.alpha = -h.vertical.damp * xsqrt(eh.vertical.spring);
            eh.vertical.omega = xsqrt(eh.vertical.spring - eh.vertical.alpha * eh.vertical.alpha);
            eh.vertical.emax = spring_energy(h.vertical.max_dist, 0.0f, eh.vertical.spring,
                                             h.vertical.gravity, eh.vertical.rest_dist);
            max_yvel =
                spring_velocity(eh.vertical.rest_dist, 1.0f, eh.vertical.emax, eh.vertical.spring,
                                h.vertical.gravity, eh.vertical.rest_dist);
            h.camera.vel_scale = shared.hook->asset->camera.vel_scale / max_yvel;
        }

        F32 hanging_state_type::spring_velocity(F32 x, F32 v, F32 e, F32 k, F32 g, F32 xc) const
        {
            F32 ret = e - spring_potential_energy(x, k, g, xc);
            if (ret <= 0.0f)
            {
                return 0.0f;
            }
            ret = xsqrt(2.0f * ret);

            if (v < 0.0f)
            {
                if (ret > 0.0f)
                {
                    return -ret;
                }
            }
            else
            {
                if (ret < 0.0f)
                {
                    return -ret;
                }
            }
            return ret;
        }
        F32 hanging_state_type::spring_potential_energy(F32 x, F32 k, F32 g, F32 xc) const
        {
            return -(g * -(0.5f * xc - x) - spring_potential_energy(x, k));
        }
        F32 hanging_state_type::spring_potential_energy(F32 x, F32 k) const
        {
            return 0.5f * k * x * x;
        }
        F32 hanging_state_type::spring_energy(F32 x, F32 v, F32 k, F32 g, F32 xc) const
        {
            return kinetic_energy(v) + spring_potential_energy(x, k, g, xc);
        }
        F32 hanging_state_type::kinetic_energy(F32 v) const
        {
            return 0.5f * v * v;
        }
        F32 hanging_state_type::find_spring_min(F32 min_dist, F32 max_dist, F32 gravity,
                                                F32 damp) const
        {
            F32 e = xexp(-PI * damp * xsqrt(-(damp * damp - 1.0f)));
            return (2.0f * min_dist + max_dist * (e - 1.0f)) / (1.0f + e);
        }
        U32 check_anim_start(xAnimTransition*, xAnimSingle*, void*)
        {
            return 0;
        }
        U32 check_anim_hit_to_dive(xAnimTransition*, xAnimSingle*, void*)
        {
            shared.anim_state &= ~0x40;
            return shared.anim_state & 0x2 && (shared.anim_state & 0x80) == 0;
        }
        U32 check_anim_hit_to_top(xAnimTransition*, xAnimSingle*, void*)
        {
            shared.anim_state &= ~0x40;
            return shared.anim_state & 0x8 && (shared.anim_state & 0x80) == 0;
        }
        U32 check_anim_hit_to_bottom(xAnimTransition*, xAnimSingle*, void*)
        {
            shared.anim_state &= ~0x40;
            return shared.anim_state & 0x20 && (shared.anim_state & 0x80) == 0;
        }
        U32 check_anim_hit_to_cycle(xAnimTransition*, xAnimSingle*, void*)
        {
            shared.anim_state &= ~0x40;
            return (shared.anim_state & 0x2a) == 0 && (shared.anim_state & 0x80) == 0;
        }
        U32 check_anim_hit_to_death(xAnimTransition*, xAnimSingle*, void*)
        {
            shared.anim_state &= ~0x40;
            if (shared.anim_state & 0x80)
            {
                play_sound(SOUND_DEATH, 1.0f);
                return 1;
            }
            return 0;
        }
        static S32 find_nearest_hook(const xVec3& loc)
        {
            S32 found = -1;
            F32 closest = 10000000000.0f;
            for (S32 i = 0; i < shared.hook_cache_size; ++i)
            {
                F32 attach_dist = shared.hook_cache[i]->asset->attach.dist;
                xVec3* p = xEntGetPos(shared.hook_cache[i]->ent);
                xVec3 dloc = *p + shared.hook_cache[i]->asset->center - loc;
                F32 len2 = dloc.length2();
                if (len2 <= SQR(attach_dist) && len2 < closest)
                {
                    found = i;
                    closest = len2;
                }
            }
            return found;
        }
        static void init_sounds()
        {
        }
        static xModelInstance* get_hook_model()
        {
            if (shared.hook == NULL || shared.hook->ent == NULL)
            {
                return NULL;
            }
            return shared.hook->ent->model;
        }
        static void fade_hook_reset()
        {
            if ((shared.flags & 0x60) == 0)
            {
                return;
            }

            shared.flags &= ~0x60;
            xModelInstance* hook = get_hook_model();
            if (hook != NULL)
            {
                hook->Alpha = 1.0f;
                hook->Flags &= 0xbfff;
                hook->PipeFlags &= ~0x30;
            }
        }
        static void fade_hook_out()
        {
            if (shared.flags & 0x40)
            {
                return;
            }

            shared.flags &= ~0x60;
            xModelInstance* hook = get_hook_model();
            if (hook != NULL)
            {
                hook->Flags |= 0x4000;
                hook->PipeFlags = hook->PipeFlags & ~0x30 | 0x30;
                shared.flags |= 0x40;
            }
        }
        static void fade_hook_in()
        {
            if (shared.flags & 0x20)
            {
                return;
            }

            shared.flags &= ~0x60;
            xModelInstance* hook = get_hook_model();
            if (hook != NULL)
            {
                hook->Flags |= 0x4000;
                hook->PipeFlags = hook->PipeFlags & ~0x30 | 0x30;
                shared.flags |= 0x20;
            }
        }
        static void fade_hook_update(float dt)
        {
            if ((shared.flags & 0x60) == 0)
            {
                return;
            }

            xModelInstance* hook = get_hook_model();
            if (hook == NULL)
            {
                return;
            }

            float vel = (1.0f - fixed.hook_fade_alpha) / fixed.hook_fade_time;
            if (shared.flags & 0x40)
            {
                hook->Alpha = -(vel * dt - hook->Alpha);
                if (hook->Alpha <= fixed.hook_fade_alpha)
                {
                    hook->Alpha = fixed.hook_fade_alpha;
                    shared.flags &= ~0x40;
                }
            }
            else
            {
                hook->Alpha = vel * dt + hook->Alpha;
                if (hook->Alpha >= 1.0f)
                {
                    hook->Alpha = 1.0f;
                    shared.flags &= ~0x20;
                    hook->Flags &= 0xbfff;
                    hook->PipeFlags &= ~0x30;
                }
            }
        }

        void start()
        {
            if ((shared.flags & 0x7) != 0x3)
            {
                return;
            }

            xEnt& player = globals.player.ent;
            const char* anim_name = player.model->Anim->Single->State->Name;
            bool found =
                shared.hook == NULL && globals.player.s->pcType == ePlayer_SB &&
                (strcmp(anim_name, "JumpStart01") || strcmp(anim_name, "JumpLift01") ||
                 strcmp(anim_name, "JumpApex01") || strcmp(anim_name, "DJumpStart01") ||
                 strcmp(anim_name, "DJumpLift01") || strcmp(anim_name, "Fall01") ||
                 strcmp(anim_name, "FallHigh01")) &&
                globals.player.cheat_mode == 0 && (globals.player.ControlOff & ~0x4000) == 0;

            shared.hook = NULL;
            if (found)
            {
                S32 i = find_nearest_hook(player.frame->mat.pos);
                if (i >= 0)
                {
                    shared.hook = shared.hook_cache[i];
                    shared.hook->ent = shared.hook_cache[i]->ent;
                }
            }

            if (shared.hook == NULL)
            {
                return;
            }

            zEntPlayerControlOn(CONTROL_OWNER_SPRINGBOARD);
            fade_hook_out();
            init_sounds();
            shared.flags |= 4;
            shared.state = shared.states[0];
            shared.state->start();
        }

        void cache_hook(hook_type& hook)
        {
            shared.hook_cache[shared.hook_cache_size] = &hook;
            shared.hook_cache_size++;
        }

        void cache_drop(const drop_asset& drop, U32 size)
        {
            xMarkerAsset* marker = (xMarkerAsset*)xSTFindAsset(drop.marker, &size);
            if (marker == NULL)
            {
                return;
            }

            if (size != sizeof(xMarkerAsset))
            {
                return;
            }

            shared.drop_cache[shared.drop_cache_size] = &drop;
            shared.drop_marker_cache[shared.drop_cache_size] = marker;
            shared.drop_cache_size++;
        }

        void init_cache()
        {
            shared.hook_cache_size = 0;
            zScene& s = *globals.sceneCur;
            hook_type* it = (hook_type*)s.baseList[eBaseTypeBungeeHook];
            hook_type* end = it + s.baseCount[eBaseTypeBungeeHook];
            for (; it != end; ++it)
            {
                cache_hook(*it);
            }

            shared.drop_cache_size = 0;
            S32 imax = xSTAssetCountByType('DYNA');
            U32 drop_type_id = xStrHash(drop_asset::type_name());
            for (S32 i = 0; i < imax; ++i)
            {
                U32 size = 0;
                xDynAsset* a = (xDynAsset*)xSTFindAssetByType('DYNA', i, &size);
                if (a == NULL)
                {
                    continue;
                }
                if (a->type == drop_type_id)
                {
                    cache_drop(*(drop_asset*)a, size);
                }
            }
        }

        void common_update(xScene& sc, F32 dt)
        {
            zEntPlayerCollTrigger(&globals.player.ent, &sc);
        }

        void hanging_state_type::calc_drop_off_velocity(xVec3& v, const xVec3& from,
                                                        const xVec3& to, F32 g, F32 t)
        {
            v = (to - from) * (1.0f / t);
            v.y = 0.5f * g * t + v.y;
        }

        void hanging_state_type::render()
        {
            render_player(TRUE);
        }

        void hanging_state_type::update_vmovement(F32 dt)
        {
            F32 dv;
            F32 range;
            F32 v;

            if (can_dive && globals.pad0->pressed & XPAD_BUTTON_X && control_lag_timer <= 0.0f &&
                !dying && dive_remaining <= 0.0f)
            {
                has_dived = true;
                allow_dive(false);
                dive_remaining = fixed.dive.time;
            }

            if (dive_remaining > 0.0f)
            {
                dive_remaining -= dt;

                if (dive_remaining < 0.0f)
                {
                    dv = h.vertical.dive * (dt + dive_remaining);
                    dive_remaining = 0.0f;
                }
                else
                {
                    dv = h.vertical.dive * dt;
                }

                if (spring_energy(loc.y, vel.y, eh.vertical.spring, h.vertical.gravity,
                                  eh.vertical.rest_dist) < eh.vertical.emax)
                {
                    vel.y += dv;

                    if (spring_energy(loc.y, vel.y, eh.vertical.spring, h.vertical.gravity,
                                      eh.vertical.rest_dist) > eh.vertical.emax)
                    {
                        vel.y = spring_velocity(loc.y, vel.y, eh.vertical.emax, eh.vertical.spring,
                                                h.vertical.gravity, eh.vertical.rest_dist);
                    }
                }
            }

            calc_movement(loc.y, v, range, loc.y, vel.y, dt, eh.vertical.rest_dist,
                          eh.vertical.alpha, eh.vertical.omega);

            if (v - vel.y < 0.0f && loc.y < h.vertical.min_dist)
            {
                if (spring_energy(loc.y, v, eh.vertical.spring, h.vertical.gravity,
                                  eh.vertical.rest_dist) > eh.vertical.emax)
                {
                    v = spring_velocity(loc.y, v, eh.vertical.emax, eh.vertical.spring,
                                        h.vertical.gravity, eh.vertical.rest_dist);
                }

                if (v > vel.y)
                {
                    v = vel.y;
                }
            }

            vel.y = v;

            if (vel.y > 0.1f * max_yvel)
            {
                has_dived = false;
            }
            else if (!has_dived)
            {
                F32 span = h.vertical.max_dist - h.vertical.min_dist;
                F32 ybottom = -h.vertical.min_dist - span * fixed.dive.min_dist;
                F32 ytop = -h.vertical.min_dist - span * fixed.dive.max_dist;

                if (loc.y <= ybottom && loc.y <= ytop)
                {
                    if (!can_dive)
                    {
                        allow_dive(true);
                    }
                }
                else
                {
                    if (can_dive)
                    {
                        allow_dive(false);
                    }
                }
            }
        }

        void hanging_state_type::calc_movement(F32& r4, F32& r5, F32& r6, F32 f1, F32 f2, F32 f3,
                                               F32 f4, F32 f5, F32 f6)
        {
            F32 dVar11;
            F32 dVar1;
            F32 dVar2;
            F32 dVar3;
            F32 dVar4;
            F32 dVar5;
            F32 dVar6;
            F32 dVar7;
            F32 dVar10;
            F32 dVar9;
            F32 dVar8;

            dVar7 = f6;
            dVar6 = f5;
            dVar5 = f4;
            dVar4 = f3;
            dVar3 = f2;
            dVar2 = f1;
            dVar11 = dVar7 * dVar4;
            dVar1 = isin(dVar11);
            dVar11 = icos(dVar11);
            dVar9 = dVar2 - dVar5;
            dVar8 = -(dVar9 * dVar6 - dVar3) / dVar7;
            dVar10 = dVar8 * dVar6 - dVar9 * dVar7;
            dVar2 = xexp(dVar6 * dVar4);

            r4 = dVar2 * (dVar9 * dVar11 + (dVar8 * dVar1)) + dVar5;
            r5 = dVar2 * (dVar3 * dVar11 + (dVar10 * dVar1));
            r6 = dVar2 * (dVar11 * (dVar3 * dVar6 + (dVar10 * dVar7)) +
                          (dVar1 * (dVar10 * dVar6 - (dVar3 * dVar7))));
        }

        void hanging_state_type::update_heading(F32 dt)
        {
            F32 angle = xrmod((PI / 2) - stick_ang - rot);
            rot_vel += stick_frac * (angle - PI) * fixed.turn.spring * stick_mag * dt;

            rot += rot_vel * dt;
            rot_vel *= fixed.turn.decay;
        }

        S32 hanging_state_type::detach_update(xScene& scene, F32& dt)
        {
            xSphere sphere;

            last_hook_loc = shared.hook_loc;
            update_hook_loc();

            stick_loc.x = stick_loc.y = 0.0f; // Chained assignment required for match
            last_loc = loc;

            xVec3 deltaVel = loc.normal() * -h.detach.accel;
            vel += deltaVel * dt;
            loc += deltaVel * dt * dt + vel * dt;

            update_animation(dt);
            update_detach_camera(dt);

            globals.player.ent.bound.sph = player_bound();

            update_sound(dt);
            update_blur(dt);

            if (loc.y >= 0.0f || loc.length2() <= 0.1f)
            {
                play_sound(SOUND_DETACH, 0.0f);
                return -1;
            }

            return 1;
        }

        void hanging_state_type::update_detach_camera(F32 dt)
        {
            xMat4x3 cammat;
            xQuat dir;

            detach.time += dt;

            F32 detachTimeRatio;
            if (detach.time < detach.end_time)
            {
                detachTimeRatio = detach.time / detach.end_time;
            }
            else
            {
                detachTimeRatio = 1.0f;
            }

            F32 curve = xSCurve(detachTimeRatio);
            cammat.pos = detach.start_loc + (detach.end_loc - detach.start_loc) * curve;
            cammat.pos += local_to_world(loc);

            xQuatSlerp(&dir, &detach.start_dir, &detach.end_dir, curve);
            xQuatToMat(&dir, &cammat);

            xCameraMove(&globals.camera, cammat.pos);
            xCameraRotate(&globals.camera, cammat, 0.0f, 0.0f, 0.0f);
        }

        void hanging_state_type::start_detaching()
        {
            xMat4x3 mat;

            detaching = true;
            calc_drop_off_velocity(drop_off_vel, shared.hook_loc, shared.drop_loc,
                                   globals.player.g.Gravity,
                                   shared.hook->asset->detach.free_fall_time);

            xMat4x3& cam = globals.camera.mat;
            xVec3 world_loc = local_to_world(loc);
            mat.right = cam.right;
            mat.up.assign(0.0f, 1.0f, 0.0f);
            mat.at = mat.right.cross(mat.up);

            F32 hgoal = globals.camera.hgoal;
            F32 dgoal = globals.camera.dgoal;
            detach.start_loc = cam.pos - world_loc;
            detach.end_loc = mat.up * hgoal + mat.at * -dgoal;

            xVec3 eulerVec;
            xMat3x3GetEuler(&mat, &eulerVec);
            eulerVec.y = zCameraTweakGlobal_GetPitch();
            eulerVec.z = 0.0f;
            xMat3x3Euler(&mat, &eulerVec);

            xQuatFromMat(&detach.start_dir, &globals.camera.mat);
            xQuatFromMat(&detach.end_dir, &mat);

            detach.time = 0.0f;
            detach.end_time = xsqrt(__fabs(loc.length() / h.detach.accel));
            if (detach.end_time >= -1e-5f && detach.end_time <= 1e-5f)
            {
                detach.end_time = 0.01f;
            }

            globals.camera.tm_dec = 0.0f;
            globals.camera.tm_acc = 0.0f;
            globals.camera.tmr = 0.0f;
            globals.camera.ltm_dec = 0.0f;
            globals.camera.ltm_acc = 0.0f;
            globals.camera.ltmr = 0.0f;
            globals.camera.pgoal = PI + eulerVec.x;
            globals.camera.pcur = PI + eulerVec.x;
        }

        bool hanging_state_type::cb_cache_collisions::operator()(xEnt& ent, xGridBound& bound)
        {
            xCollis coll;

            if (!(ent.chkby & 0x10) || !(ent.penby & 0x10))
            {
                return true;
            }

            coll.flags = 0x0;
            xSphereHitsBound(&o, &ent.bound, &coll);

            if (!(coll.flags & 0x1))
            {
                return true;
            }

            if (ent.collLev == 0x5)
            {
                xSphereHitsModel(&o, ent.model, &coll);
                if (!(coll.flags & 0x1))
                {
                    return true;
                }
            }

            ent_info& cache_item = ent_cache[ent_cache_size];
            cache_item.ent = &ent;
            cache_item.hits = 0;
            ent_cache_size++;

            return true;
        }
    } // namespace

    void load(class xBase& data, class xDynAsset& asset, size_t)
    {
        xBaseInit(&data, &asset);
        hook_type& hook = (hook_type&)data;
        hook.asset = (hook_asset*)&asset;
        if (hook.linkCount != 0)
        {
            hook.link = (xLinkAsset*)(hook.asset + 1);
        }
        hook.ent = (xEnt*)zSceneFindObject(hook.asset->entity);
    }

    void load_settings(xIniFile& ini)
    {
        fixed.bottom_anim_frac = xIniGetFloat(&ini, "SB.state.bungee.bottom_anim_frac", 0.05f);
        if (fixed.bottom_anim_frac < 0.0f)
        {
            fixed.bottom_anim_frac = 0.0f;
        }
        if (fixed.bottom_anim_frac > 0.5f)
        {
            fixed.bottom_anim_frac = 0.5f;
        }
        xDebugAddTweak("Bungee|Globals|Anim Bottom Fraction", &fixed.bottom_anim_frac, 0.0f, 0.5f,
                       NULL, NULL, 0);

        fixed.top_anim_frac = xIniGetFloat(&ini, "SB.state.bungee.top_anim_frac", 0.1f);
        if (fixed.top_anim_frac < 0.0f)
        {
            fixed.top_anim_frac = 0.0f;
        }
        if (fixed.top_anim_frac > 0.5f)
        {
            fixed.top_anim_frac = 0.5f;
        }
        xDebugAddTweak("Bungee|Globals|Anim Top Fraction", &fixed.top_anim_frac, 0.0f, 0.5f, NULL,
                       NULL, NULL);

        fixed.bottom_anim_time = xIniGetFloat(&ini, "SB.state.bungee.bottom_anim_time", 0.1f);
        if (fixed.bottom_anim_time < 0.0f)
        {
            fixed.bottom_anim_time = 0.0f;
        }
        if (fixed.bottom_anim_time > 2.0f)
        {
            fixed.bottom_anim_time = 2.0f;
        }
        xDebugAddTweak("Bungee|Globals|Anim Trans Bottom-Time", &fixed.bottom_anim_time, 0.0f, 2.0f,
                       NULL, NULL, NULL);

        fixed.top_anim_time = xIniGetFloat(&ini, "SB.state.bungee.top_anim_time", 0.1f);
        if (fixed.top_anim_time < 0.0f)
        {
            fixed.top_anim_time = 0.0f;
        }
        if (fixed.top_anim_time > 2.0f)
        {
            fixed.top_anim_time = 2.0f;
        }
        xDebugAddTweak("Bungee|Globals|Anim Trans Top-Time", &fixed.top_anim_time, 0.0f, 2.0f, NULL,
                       NULL, NULL);

        fixed.hit_anim_time = xIniGetFloat(&ini, "SB.state.bungee.hit_anim_time", 0.1f);
        if (fixed.hit_anim_time < 0.0f)
        {
            fixed.hit_anim_time = 0.0f;
        }
        if (fixed.hit_anim_time > 2.0f)
        {
            fixed.hit_anim_time = 2.0f;
        }
        xDebugAddTweak("Bungee|Globals|Anim Trans Hit-Time", &fixed.hit_anim_time, 0.0f, 2.0f, NULL,
                       NULL, NULL);

        fixed.damage_rot = xIniGetFloat(&ini, "SB.state.bungee.damage_rot", 10.0f);
        if (fixed.damage_rot < 10.0f)
        {
            fixed.damage_rot = 10.0f;
        }
        if (fixed.damage_rot > 1000.0f)
        {
            fixed.damage_rot = 1000.0f;
        }
        xDebugAddTweak("Bungee|Globals|Damage Rotation", &fixed.damage_rot, 10.0f, 1000.0f, NULL,
                       NULL, NULL);

        fixed.death_time = xIniGetFloat(&ini, "SB.state.bungee.death_time", 1.5f);
        if (fixed.death_time < 0.0f)
        {
            fixed.death_time = 0.0f;
        }
        if (fixed.death_time > 20.0f)
        {
            fixed.death_time = 20.0f;
        }
        xDebugAddTweak("Bungee|Globals|Death Fade-Out Time", &fixed.death_time, 0.0f, 20.0f, NULL,
                       NULL, NULL);

        fixed.vel_blur = xIniGetFloat(&ini, "SB.state.bungee.vel_blur", 0.0f);
        if (fixed.vel_blur < 0.0f)
        {
            fixed.vel_blur = 0.0f;
        }
        if (fixed.vel_blur > 1.0f)
        {
            fixed.vel_blur = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Velocity Blur", &fixed.vel_blur, 0.0f, 1.0f, NULL, NULL,
                       NULL);

        fixed.fade_dist = xIniGetFloat(&ini, "SB.state.bungee.fade_dist", 2.0f);
        if (fixed.fade_dist < 1.0f)
        {
            fixed.fade_dist = 1.0f;
        }
        if (fixed.fade_dist > 10000.0f)
        {
            fixed.fade_dist = 10000.0f;
        }
        xDebugAddTweak("Bungee|Globals|Cord Fade Distance", &fixed.fade_dist, 1.0f, 10000.0f, NULL,
                       NULL, NULL);

        fixed.player_radius = xIniGetFloat(&ini, "SB.state.bungee.player_radius", 1.0f);
        if (fixed.player_radius < 0.0f)
        {
            fixed.player_radius = 0.0f;
        }
        if (fixed.player_radius > 10.0f)
        {
            fixed.player_radius = 10.0f;
        }
        xDebugAddTweak("Bungee|Globals|Player Radius", &fixed.player_radius, 0.0f, 10.0f, NULL,
                       NULL, NULL);

        fixed.hook_fade_alpha = xIniGetFloat(&ini, "SB.state.bungee.hook_fade_alpha", 0.3f);
        if (fixed.hook_fade_alpha < 0.0f)
        {
            fixed.hook_fade_alpha = 0.0f;
        }
        if (fixed.hook_fade_alpha > 1.0f)
        {
            fixed.hook_fade_alpha = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Hook Fade Alpha", &fixed.hook_fade_alpha, 0.0f, 1.0f, NULL,
                       NULL, NULL);

        fixed.hook_fade_time = xIniGetFloat(&ini, "SB.state.bungee.hook_fade_time", 1.0f);
        if (fixed.hook_fade_time < 0.01f)
        {
            fixed.hook_fade_time = 0.01f;
        }
        if (fixed.hook_fade_time > 10.0f)
        {
            fixed.hook_fade_time = 10.0f;
        }
        xDebugAddTweak("Bungee|Globals|Hook Fade Time", &fixed.hook_fade_time, 0.01f, 10.0f, NULL,
                       NULL, NULL);

        fixed.horizontal.edge_zone =
            xIniGetFloat(&ini, "SB.state.bungee.horizontal.edge_zone", 0.0f);
        if (fixed.horizontal.edge_zone < 0.0f)
        {
            fixed.horizontal.edge_zone = 0.0f;
        }
        if (fixed.horizontal.edge_zone > 0.25f)
        {
            fixed.horizontal.edge_zone = 0.25f;
        }
        xDebugAddTweak("Bungee|Globals|Horz Edge Zone", &fixed.horizontal.edge_zone, 0.0f, 0.25f,
                       NULL, NULL, NULL);

        fixed.horizontal.sway = xIniGetFloat(&ini, "SB.state.bungee.horizontal.sway", 3.0f);
        if (fixed.horizontal.sway < 0.0f)
        {
            fixed.horizontal.sway = 0.0f;
        }
        if (fixed.horizontal.sway > 10000.0f)
        {
            fixed.horizontal.sway = 10000.0f;
        }
        xDebugAddTweak("Bungee|Globals|Horz Sway Force", &fixed.horizontal.sway, 0.0f, 10000.0f,
                       NULL, NULL, NULL);

        fixed.horizontal.decay = xIniGetFloat(&ini, "SB.state.bungee.horizontal.decay", 0.91f);
        if (fixed.horizontal.decay < 0.0f)
        {
            fixed.horizontal.decay = 0.0f;
        }
        if (fixed.horizontal.decay > 1.0f)
        {
            fixed.horizontal.decay = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Horz Velocity Decay", &fixed.horizontal.decay, 0.0f, 1.0f,
                       NULL, NULL, NULL);

        fixed.dive.time = xIniGetFloat(&ini, "SB.state.bungee.dive.time", 0.5f);
        if (fixed.dive.time < 0.01f)
        {
            fixed.dive.time = 0.01f;
        }
        if (fixed.dive.time > 5.0f)
        {
            fixed.dive.time = 5.0f;
        }
        xDebugAddTweak("Bungee|Globals|Dive Time", &fixed.dive.time, 0.01f, 5.0f, NULL, NULL, NULL);

        fixed.dive.anim_out_time = xIniGetFloat(&ini, "SB.state.bungee.dive.anim_out_time", 0.5f);
        if (fixed.dive.anim_out_time < 0.0f)
        {
            fixed.dive.anim_out_time = 0.0f;
        }
        if (fixed.dive.anim_out_time > 5.0f)
        {
            fixed.dive.anim_out_time = 5.0f;
        }
        xDebugAddTweak("Bungee|Globals|Dive Anim Out-Time", &fixed.dive.anim_out_time, 0.0f, 5.0f,
                       NULL, NULL, NULL);

        fixed.dive.min_dist = xIniGetFloat(&ini, "SB.state.bungee.dive.min_dist", 0.0f);
        if (fixed.dive.min_dist < 0.0f)
        {
            fixed.dive.min_dist = 0.0f;
        }
        if (fixed.dive.min_dist > 1.0f)
        {
            fixed.dive.min_dist = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Dive Min Distance", &fixed.dive.min_dist, 0.0f, 1.0f, NULL,
                       NULL, NULL);

        fixed.dive.max_dist = xIniGetFloat(&ini, "SB.state.bungee.dive.max_dist", 0.6f);
        if (fixed.dive.max_dist < 0.51f)
        {
            fixed.dive.max_dist = 0.51f;
        }
        if (fixed.dive.max_dist > 1.0f)
        {
            fixed.dive.max_dist = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Dive Max Distance", &fixed.dive.max_dist, 0.51f, 1.0f, NULL,
                       NULL, NULL);

        fixed.camera.speed = xIniGetFloat(&ini, "SB.state.bungee.camera.speed", 10.0f);
        if (fixed.camera.speed < 0.0f)
        {
            fixed.camera.speed = 0.0f;
        }
        if (fixed.camera.speed > 1000000000.0f)
        {
            fixed.camera.speed = 1000000000.0f;
        }
        xDebugAddTweak("Bungee|Globals|Camera Speed", &fixed.camera.speed, 0.0f, 1000000000.0f,
                       NULL, NULL, NULL);

        fixed.turn.spring = xIniGetFloat(&ini, "SB.state.bungee.turn.spring", 25.0f);
        if (fixed.turn.spring < 0.0f)
        {
            fixed.turn.spring = 0.0f;
        }
        if (fixed.turn.spring > 100000.0f)
        {
            fixed.turn.spring = 100000.0f;
        }
        xDebugAddTweak("Bungee|Globals|Turn Spring", &fixed.turn.spring, 0.0f, 100000.0f, NULL,
                       NULL, NULL);

        fixed.turn.decay = xIniGetFloat(&ini, "SB.state.bungee.turn.decay", 0.95f);
        if (fixed.turn.decay < 0.0f)
        {
            fixed.turn.decay = 0.0f;
        }
        if (fixed.turn.decay > 1.0f)
        {
            fixed.turn.decay = 1.0f;
        }
        xDebugAddTweak("Bungee|Globals|Turn Decay", &fixed.turn.decay, 0.0f, 1.0f, NULL, NULL,
                       NULL);
    }

    void init()
    {
        if ((shared.flags & 0x1) != 0x1)
        {
            return;
        }

        shared.flags = 3;
        shared.state = NULL;
        shared.hook = NULL;

        static attaching_state_type attaching_state;
        shared.states[0] = &attaching_state;
        static hanging_state_type hanging_state;
        shared.states[1] = &hanging_state;

        init_cache();
    }

    void destroy()
    {
        stop();
        iCameraSetBlurriness(0.0f);
        zCameraEnableTracking(CO_BUNGEE);
        xCameraDoCollisions(1, CO_BUNGEE);
        shared.flags = 0x3;
    }

    void reset()
    {
        if ((shared.flags & 0x3) == 3)
        {
            stop();
            fade_hook_reset();
            iCameraSetBlurriness(0.0);
            zCameraEnableTracking(CO_BUNGEE);
            xCameraDoCollisions(1, CO_BUNGEE);
            shared.flags = 0x3;
        }
    }

    bool active() {
        return shared.state;
    }

    bool landed() {
        bool ret = false;
        if (shared.state == 0 && shared.dismount_delay <= 0.0f) {
            ret = true;
        }
        return ret;
    }

    bool update(xScene* sc, F32 dt)
    {
        if ((shared.flags & 0x3) != 0x3)
        {
            return false;
        }

        fade_hook_update(dt);

        F32 last_delay = shared.dismount_delay;
        shared.dismount_delay -= dt;
        if (shared.dismount_delay > 0.0f)
        {
            return false;
        }
        if (last_delay > 0.0f)
        {
            xCameraDoCollisions(1, CO_BUNGEE);
        }

        zEntPlayerControlOn(CONTROL_OWNER_BUNGEE);
        start();

        if (shared.state == NULL)
        {
            return false;
        }

        while (1)
        {
            state_enum next = shared.state->update(*sc, dt);
            if (next == shared.state->type)
            {
                break;
            }
            if (next == STATE_INVALID)
            {
                stop();
                break;
            }
            shared.state->stop();
            shared.state = shared.states[next];
            shared.state->start();
        }

        common_update(*sc, dt);
        return shared.state != NULL;
    }

    bool render()
    {
        if ((shared.flags & 0x7) != 0x7)
        {
            return false;
        }
        shared.state->render();
        return true;
    }

    void stop()
    {
        if ((shared.flags & 0x7) != 0x7)
        {
            return;
        }
        fade_hook_in();
        if (shared.state != NULL)
        {
            shared.state->stop();
        }
        shared.state = NULL;
        shared.flags &= ~0x4;
    }
} // namespace bungee_state

void bungee_state::insert_animations(xAnimTable& table)
{
    xAnimTableNewState(&table, "bungee_bottom_0", 0x10, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_top_0", 0x10, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_dive_0", 0, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_hit_0", 0x20, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_mount_0", 0x20, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_cycle_0", 0x10, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);
    xAnimTableNewState(&table, "bungee_death_0", 0, 0, 1.0f, 0, 0, 0.0f, 0, 0, xAnimDefaultBeforeEnter, 0, 0);

    static const char* start_from = "JumpStart01 JumpLift01 JumpApex01 DJumpStart01 DJumpLift01 Fall01 FallHigh01";
    static const char* start_to = "bungee_cycle_0";

    shared.anim_tran.start = xAnimTableNewTransition(&table, start_from, start_to, check_anim_start, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.5f, 0);
    shared.anim_tran.dive_start = xAnimTableNewTransition(&table, "bungee_cycle_0", "bungee_dive_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.dive.time, 0);
    shared.anim_tran.dive_stop = xAnimTableNewTransition(&table, "bungee_dive_0", "bungee_cycle_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.dive.anim_out_time, 0);
    shared.anim_tran.top_start = xAnimTableNewTransition(&table, "bungee_cycle_0", "bungee_top_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.top_anim_time, 0);
    shared.anim_tran.top_stop = xAnimTableNewTransition(&table, "bungee_top_0", "bungee_cycle_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.top_anim_time, 0);
    shared.anim_tran.bottom_start = xAnimTableNewTransition(&table, "bungee_cycle_0", "bungee_bottom_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.bottom_anim_time, 0);
    shared.anim_tran.bottom_stop = xAnimTableNewTransition(&table, "bungee_bottom_0", "bungee_cycle_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.bottom_anim_time, 0);
    shared.anim_tran.hit = xAnimTableNewTransition(&table, "bungee_cycle_0 bungee_dive_0 bungee_top_0 bungee_bottom_0", "bungee_hit_0", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    xAnimTableNewTransition(&table, "bungee_hit_0", "bungee_dive_0", check_anim_hit_to_dive, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    xAnimTableNewTransition(&table, "bungee_hit_0", "bungee_top_0", check_anim_hit_to_top, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    xAnimTableNewTransition(&table, "bungee_hit_0", "bungee_bottom_0", check_anim_hit_to_bottom, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    xAnimTableNewTransition(&table, "bungee_hit_0", "bungee_cycle_0", check_anim_hit_to_cycle, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    xAnimTableNewTransition(&table, "bungee_hit_0", "bungee_death_0", check_anim_hit_to_death, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, fixed.hit_anim_time, 0);
    shared.anim_tran.stop = xAnimTableNewTransition(&table, "bungee_cycle_0 bungee_dive_0 bungee_top_0 bungee_bottom_0", "Fall01", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.5f, 0);
}

namespace bungee_state
{
    namespace
    {

        void attaching_state_type::start()
        {
            xEntFrame& frame = *globals.player.ent.frame;
            loc = &frame.mat.pos;
            vel = &frame.vel;

            init_models();

            xVec3& root_loc = ((xMat4x3*)shared.root_model->Mat)->pos;
            *loc = root_loc;

            update_hook_loc();

            last_hook_loc = shared.hook_loc;
            hook_vel = 0.0f;
            time = 0.0f;
            end_time = shared.hook->asset->attach.travel_time;
            player_loc = *loc;

            player_vel = (frame.mat.pos - frame.oldmat.pos) / globals.update_dt;

            xVec3 offset = shared.hook_loc - *loc;
            F32 dist = offset.length();
            if (dist / end_time > 10.0f)
            {
                end_time = 0.1f * dist;
            }

            xAnimPlayStartTransition(globals.player.ent.model->Anim, shared.anim_tran.start);
            trigger(eEventMount);
        }

        void attaching_state_type::stop()
        {
            *loc = shared.hook_loc;
            *vel = 0.0f;
        }

        state_enum attaching_state_type::update(xScene&, F32& dt)
        {
            update_hook_loc();

            time += dt;
            if (time >= end_time)
            {
                dt = time - end_time;
                return STATE_HANGING;
            }

            if (time < end_time)
            {
                F32 g = -globals.player.g.Gravity;

                player_loc.x += player_vel.x * dt;
                player_loc.y += player_vel.y * dt + dt * (0.5f * g * dt);
                player_loc.z += player_vel.z * dt;
                player_vel.y += g * dt;

                F32 s = xSCurve(time / end_time);
                *loc = player_loc + (shared.hook_loc - player_loc) * s;
                *vel = 0.0f;
            }
            else
            {
                dt = time - end_time;
            }

            RwMatrix& mm = *shared.root_model->Mat;
            ((xMat4x3&)mm).pos = *loc;

            xModelUpdate(globals.player.ent.model, dt);
            xModelEval(globals.player.ent.model);

            if (time < end_time)
            {
                return STATE_ATTACHING;
            }

            dt = time - end_time;
            return STATE_HANGING;
        }

        void attaching_state_type::render()
        {
            render_player(FALSE);
        }

        // NOTE: defined here, ahead of hanging_state_type::start(), because the target
        // object creates this function's anonymous 12-byte { 0, -1, 0 } .rodata template
        // (@1502) *before* start()'s four static tweak_callback objects (@1766-@1769).
        // .rodata is laid out in object-id (parse) order, so the definition has to be
        // parsed first or every later .rodata offset shifts by 12 and start() cannot
        // match. The target's .text order puts this function after collide_start().
        void hanging_state_type::update_camera(F32 dt)
        {
            if (update_free_look(dt))
            {
                return;
            }

            xMat3x3 m;
            xVec3 dir;
            xVec3 offset;
            xVec3 up;
            xVec3 goal;
            xVec3 down = { 0.0f, -1.0f, 0.0f };

            if (loc.y >= 0.0f)
            {
                dir = down;
            }
            else
            {
                F32 dist = loc.length();
                if (dist < h.camera.rest_dist)
                {
                    if (dist < 0.01f)
                    {
                        dir = down;
                    }
                    else
                    {
                        F32 frac = dist / h.camera.rest_dist;
                        dir = down * (1.0f - frac) + loc * (frac / dist);
                        dir.normalize();
                    }
                }
                else
                {
                    dir = loc * (1.0f / dist);
                }
            }

            F32 vscale = (vel.y >= 0.0f) ? 0.0f : -vel.y * h.camera.vel_scale;

            offset = dir * -(h.camera.rest_dist + vscale);

            up.x = 0.0f;
            up.y = dir.z;
            up.z = -dir.y;
            up.normalize();

            xMat3x3Rot(&m, &dir, h.camera.offset_dir);
            xMat3x3RMulVec(&up, &m, &up);

            offset += up * h.camera.offset;

            goal = loc + offset;

            if ((goal - cam_loc).length2() > 0.0001f)
            {
                xVec3 v = cam_dir;

                interpolate_camera_loc(goal, dt);

                const xVec3 dv = cam_dir - v;
                if (((dv.x < 0.0f) ? 1 : 0) != ((vel.x < 0.0f) ? 1 : 0))
                {
                    cam_dir.x = v.x;
                }
                if (((dv.y < 0.0f) ? 1 : 0) != ((vel.y < 0.0f) ? 1 : 0))
                {
                    cam_dir.y = v.y;
                }
                if (((dv.z < 0.0f) ? 1 : 0) != ((vel.z < 0.0f) ? 1 : 0))
                {
                    cam_dir.z = v.z;
                }
            }

            xCameraMove(&globals.camera, local_to_world(cam_loc));
            update_camera_direction(dt);
        }

        void hanging_state_type::start()
        {
            static const tweak_callback vertical_cb = { (void (*)(tweak_info&))on_tweak_vertical };
            static const tweak_callback horizontal_cb = { (void (*)(tweak_info&))
                                                              on_tweak_horizontal };
            static const tweak_callback camera_cb = { (void (*)(tweak_info&))on_tweak_camera };
            static const tweak_callback collision_cb = { (void (*)(tweak_info&))
                                                             on_tweak_collision };

            show_help();
            has_dived = false;
            allow_dive(false);

            last_health = globals.player.Health;
            dying = false;
            control_lag_timer = 0.0f;
            control_lag_max = 1.0f;
            globals.player.DamageTimer = 0.0f;
            detaching = false;

            show_models();
            update_hook_loc();

            shared.flags = (shared.flags | 0x8) & ~0x10;
            zCameraDisableTracking(CO_BUNGEE);
            xCameraDoCollisions(0, CO_BUNGEE);
            globals.player.ControlOffTimer = 0.0f;
            shared.anim_state = 0;

            h = *shared.hook->asset;

            update_hook_loc();
            last_hook_loc = shared.hook_loc;

            reset_props_vertical();
            h.detach.accel *= -h.vertical.gravity;
            reset_props_camera();

            loc = world_to_local(globals.player.ent.frame->mat.pos);
            last_loc = loc;
            vel = globals.player.ent.frame->vel;
            dive_remaining = 0.0f;

            init_camera();

            xVec3 eu;
            xMat3x3GetEuler((const xMat3x3*)shared.root_model->Mat, &eu);
            rot = eu.x;
            rot_vel = 0.0f;

            reset_props_collision();

            globals.player.ent.bound.sph.r = fixed.player_radius;
            xAnimPlayStartTransition(globals.player.ent.model->Anim, shared.anim_tran.start);

            play_sound(SOUND_ATTACH, 0.0f);
            play_sound(SOUND_ATTACH_COMMENT, 0.0f);
            play_sound(SOUND_WIND_LOOP, 0.0f);
            zMusicSetVolume(0.5f, 1.0f);

            xDebugAddTweak("Bungee|Hook|Vertical Frequency",
                           &shared.hook->asset->vertical.frequency, 0.1f, 100.0f, &vertical_cb,
                           this, 0);
            xDebugAddTweak("Bungee|Hook|Vertical Dive Strength", &shared.hook->asset->vertical.dive,
                           0.0f, 10000.0f, &vertical_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Vertical Gravity", &shared.hook->asset->vertical.gravity,
                           0.1f, 1000.0f, &vertical_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Vertical Min Distance",
                           &shared.hook->asset->vertical.min_dist, 0.5f, 100.0f, &vertical_cb, this,
                           0);
            xDebugAddTweak("Bungee|Hook|Vertical Max Distance",
                           &shared.hook->asset->vertical.max_dist, 0.5f, 500.0f, &vertical_cb, this,
                           0);
            xDebugAddTweak("Bungee|Hook|Vertical Dampening", &shared.hook->asset->vertical.damp,
                           0.0f, 1.0f, &vertical_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Horizontal Max Distance",
                           &shared.hook->asset->horizontal.max_dist, 0.0f, 100.0f, &horizontal_cb,
                           this, 0);
            xDebugAddTweak("Bungee|Hook|Camera Rest Distance",
                           &shared.hook->asset->camera.rest_dist, 1.0f, 200.0f, &camera_cb, this,
                           0);
            xDebugAddTweak("Bungee|Hook|Camera View Angle", &shared.hook->asset->camera.view_angle,
                           0.0f, 360.0f, &camera_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Camera Offset", &shared.hook->asset->camera.offset, 0.0f,
                           100.0f, &camera_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Camera Offset Direction",
                           &shared.hook->asset->camera.offset_dir, 0.0f, 360.0f, &camera_cb, this,
                           0);
            xDebugAddTweak("Bungee|Hook|Camera Turn Speed",
                           &shared.hook->asset->camera.turn_speed, 0.0f, 1.0f, &camera_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Camera Velocity Scale",
                           &shared.hook->asset->camera.vel_scale, 0.0f, 1000.0f, &camera_cb, this,
                           0);
            xDebugAddTweak("Bungee|Hook|Camera Roll Speed", &shared.hook->asset->camera.roll_speed,
                           0.0f, 1.0f, &camera_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Collide Hit Loss", &shared.hook->asset->collision.hit_loss,
                           0.0f, 1.0f, &collision_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Collide Damage Velocity",
                           &shared.hook->asset->collision.damage_velocity, 0.0f, 1.0f,
                           &collision_cb, this, 0);
            xDebugAddTweak("Bungee|Hook|Collide Hit Velocity",
                           &shared.hook->asset->collision.hit_velocity, 0.0f, 1.0f, &collision_cb,
                           this, 0);
        }

        void hanging_state_type::init_camera()
        {
            const xMat4x3& cm = globals.camera.mat;

            cam_loc = world_to_local(cm.pos);

            xVec3 eu;
            xMat3x3GetEuler(&cm, &eu);

            F32 cy = icos(eu.y);
            cam_dir.x = cy * isin(eu.x);
            cam_dir.y = -isin(eu.y);
            cam_dir.z = cy * icos(eu.x);

            roll_offset = -(eu.x - h.camera.view_angle);
            roll_offset = xrmod(PI + roll_offset);
            if (roll_offset < 0.0f)
            {
                roll_offset += 2.0f * PI;
            }
            roll_offset = roll_offset - PI;

            cam_vel = 0.0f;
            cam_dir_vel = 0.0f;
        }

        xVec3 hanging_state_type::world_to_local(const xVec3& vw) const
        {
            return vw - shared.hook_loc;
        }

        void hanging_state_type::allow_dive(bool allowed)
        {
            can_dive = allowed;

            ztextbox* tb = (ztextbox*)zSceneFindObject(xStrHash("TEXTBOX_BUNGEE_HELP"));
            if (tb == NULL || tb->baseType != eBaseTypeTextBox)
            {
                return;
            }

            if (can_dive)
            {
                tb->set_text(xStrHash("text_bungee_help"));
            }
            else
            {
                tb->set_text(xStrHash("text_bungee_help_nodive"));
            }
        }

        void hanging_state_type::show_help()
        {
            xBase* base = zSceneFindObject(xStrHash("TEXTBOX_BUNGEE_HELP"));
            if (base == NULL || base->baseType != eBaseTypeTextBox)
            {
                return;
            }

            ((ztextbox*)base)->activate();
        }

        void hanging_state_type::stop()
        {
            iCameraSetBlurriness(0.0f);
            hide_help();

            shared.flags &= ~0x8;
            shared.dismount_delay = shared.hook->asset->detach.free_fall_time;

            zEntPlayerControlOff(CONTROL_OWNER_BUNGEE);
            hide_models();

            xMat4x3& mm = *(xMat4x3*)shared.root_model->Mat;
            globals.player.ent.frame->mat.pos = mm.pos = shared.hook_loc;
            globals.player.ent.frame->vel = drop_off_vel;
            globals.player.ent.bound.sph.r = default_player_radius;

            xAnimPlayStartTransition(globals.player.ent.model->Anim, shared.anim_tran.stop);

            stop_sound(SOUND_WIND_LOOP);
            zMusicSetVolume(1.0f, 1.0f);
            zCameraEnableTracking(CO_BUNGEE);
            xDebugRemoveTweak("Bungee|Hook");
            trigger(eEventDismount);

            xEnt* ent = shared.hook->ent;
            if (ent != NULL)
            {
                ent->chkby &= 0xef;
                ent->penby &= 0xef;
            }
        }

        void hanging_state_type::hide_help()
        {
            xBase* base = zSceneFindObject(xStrHash("TEXTBOX_BUNGEE_HELP"));
            if (base == NULL || base->baseType != eBaseTypeTextBox)
            {
                return;
            }

            ((ztextbox*)base)->deactivate();
        }

        state_enum hanging_state_type::update(xScene& s, F32& dt)
        {
            if (globals.pad0->pressed & XPAD_BUTTON_TRIANGLE && !dying && !detaching &&
                find_drop_off())
            {
                start_detaching();
            }

            if (detaching)
            {
                return (state_enum)detach_update(s, dt);
            }

            globals.player.DamageTimer -= dt;
            control_lag_timer -= dt;

            last_hook_loc = shared.hook_loc;
            update_hook_loc();

            stick_frac = 1.0f;
            if (control_lag_timer > 0.0f)
            {
                if (control_lag_max <= 0.0f)
                {
                    stick_frac = 0.1f;
                }
                else
                {
                    stick_frac = (control_lag_max - control_lag_timer) / control_lag_max;
                }
            }

            if (dying)
            {
                stick_loc = 0.0f;
                stick_mag = stick_ang = 0.0f;
            }
            else
            {
                _tagxPad::analog_data& analog = globals.pad0->analog[0];

                stick_mag = stick_frac * analog.mag;
                stick_ang = analog.ang - h.camera.view_angle;
                stick_loc.y = stick_mag * isin(stick_ang);
                stick_loc.x = stick_mag * icos(stick_ang);
            }

            last_loc = loc;

            update_movement(dt);
            update_animation(dt);
            update_camera(dt);

            xQuickCullForBound(&globals.player.ent.bound.qcd, &globals.player.ent.bound);
            update_collision(s, dt);
            check_damage(false);

            globals.player.ent.bound.sph = player_bound();

            update_sound(dt);
            update_blur(dt);

            zEntPickup_CheckAllPickupsAgainstPlayer(&s, dt);

            if (dying && globals.player.DamageTimer <= 0.0f)
            {
                zCameraEnableTracking(CO_BUNGEE);
                return STATE_INVALID;
            }

            return STATE_HANGING;
        }

        void hanging_state_type::update_blur(F32)
        {
            F32 blur = xabs(vel.y) / max_yvel;
            if (blur > 1.0f)
            {
                blur = 1.0f;
            }
            iCameraSetBlurriness(blur * (fixed.vel_blur * blur));
        }

        void hanging_state_type::update_sound(F32)
        {
            F32 vol = xabs(vel.y) / max_yvel;
            if (vol > 1.0f)
            {
                vol = 1.0f;
            }
            set_volume(SOUND_WIND_LOOP, vol);
        }

        xSphere hanging_state_type::player_bound() const
        {
            xSphere o;

            o.r = globals.player.ent.bound.sph.r;
            o.center = local_to_world(loc);

            return o;
        }

        xVec3 hanging_state_type::local_to_world(const xVec3& vl) const
        {
            return vl + shared.hook_loc;
        }

        void hanging_state_type::check_damage(bool thump)
        {
            if (dying)
            {
                return;
            }

            U32 old_last_health = last_health;
            last_health = globals.player.Health;
            if (globals.player.Health >= old_last_health)
            {
                return;
            }

            control_lag_timer = control_lag_max = globals.player.DamageTimer = 2.0f;
            rot_vel += 2.0f * PI * fixed.damage_rot * (xurand() - 0.5f);

            shared.anim_state |= 0x40;
            xAnimPlayStartTransition(globals.player.ent.model->Anim, shared.anim_tran.hit);

            if (thump)
            {
                play_sound(SOUND_THUMP, 0.0f);
            }
            else
            {
                play_sound(SOUND_DAMAGE, 0.0f);
            }

            if (globals.player.Health == 0)
            {
                short_drop_sudden_stop();
            }
        }

        void hanging_state_type::short_drop_sudden_stop()
        {
            dying = true;
            globals.player.DamageTimer = 20.0f;
            zEntEventAll(NULL, 0, eEventPlayerDeath, NULL);
            control_lag_timer = globals.player.DamageTimer;
            control_lag_max = 0.0f;
            dive_remaining = 0.0f;
            multiply_dampening(5.0f);
            shared.anim_state |= 0x80;
        }

        void hanging_state_type::multiply_dampening(F32 s)
        {
            h.vertical.damp *= s;
            if (h.vertical.damp > 1.0f)
            {
                h.vertical.damp = 1.0f;
            }

            eh.vertical.alpha = -h.vertical.damp * xsqrt(eh.vertical.spring);
            eh.vertical.omega = xsqrt(eh.vertical.spring - eh.vertical.alpha * eh.vertical.alpha);
            eh.vertical.emax = spring_energy(h.vertical.max_dist, 0.0f, eh.vertical.spring,
                                             h.vertical.gravity, eh.vertical.rest_dist);
        }

        void hanging_state_type::update_collision(xScene& s, F32)
        {
            collide_start(s);

            bool again = true;
            for (S32 i = 0; again && i < 8; i++)
            {
                again = repath(s);
            }
        }

        bool hanging_state_type::repath(const xScene& s)
        {
            bool hit = false;
            hit = ents_repath(s) || hit;
            hit = env_repath(s) || hit;
            hit = boundary_repath(s) || hit;
            return hit;
        }

        bool hanging_state_type::boundary_repath(const xScene&)
        {
            xVec3 norm;
            xVec3 depen;

            if (!hit_boundary(norm, depen, loc))
            {
                return false;
            }

            loc += depen;
            vel -= norm * norm.dot(vel);

            return true;
        }

        bool hanging_state_type::hit_boundary(xVec3& norm, xVec3& depen, const xVec3& v) const
        {
            S32 hits =
                clip_nearest(norm.x, depen.x, v.x, -h.horizontal.max_dist, h.horizontal.max_dist);
            hits += clip_nearest(norm.y, depen.y, v.y, h.vertical.max_dist, h.vertical.min_dist);
            hits +=
                clip_nearest(norm.z, depen.z, v.z, -h.horizontal.max_dist, h.horizontal.max_dist);

            switch (hits)
            {
            case 1:
                return true;
            case 2:
                norm *= 0.70710677f;
                return true;
            case 3:
                norm *= 0.57735026f;
                return true;
            }

            return false;
        }

        S32 hanging_state_type::clip_nearest(F32& norm, F32& depen, F32 x, F32 min, F32 max) const
        {
            S32 hits = 0;

            if (x > max)
            {
                norm = -1.0f;
                depen = max - x;
                hits = 1;
            }
            else if (x < min)
            {
                norm = 1.0f;
                depen = min - x;
                hits = 1;
            }
            else
            {
                norm = 0.0f;
                depen = 0.0f;
            }

            return hits;
        }

        bool hanging_state_type::env_repath(const xScene& s)
        {
            if (!env_cache.collide)
            {
                return false;
            }

            xCollis coll;
            memset(&coll, 0, sizeof(coll));
            coll.flags = 0xa00;

            xSphere o = player_bound();
            if (iSphereHitsEnv(&o, s.env, &coll) == 0)
            {
                return false;
            }

            loc += coll.depen;

            F32 mag = -coll.norm.dot(vel);
            vel += coll.norm * mag;

            trigger_collision(env_cache, mag, coll);

            return true;
        }

        void hanging_state_type::trigger_collision(env_info& ei, F32 mag, const xCollis& coll)
        {
            ei.hits++;

            if (dying)
            {
                return;
            }

            xSurface* surf = zSurfaceGetSurface(&coll);
            if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
            {
                ouchie(true);
            }

            if (mag >= h.collision.damage_velocity)
            {
                ouchie(true);
            }
        }

        void hanging_state_type::ouchie(bool thump)
        {
            if (globals.player.DamageTimer > 0.0f || dying || !globals.player.g.TakeDamage)
            {
                return;
            }

            if (globals.player.Health != 0)
            {
                globals.player.Health--;
            }
            check_damage(thump);
        }

        bool hanging_state_type::ents_repath(const xScene&)
        {
            if (ent_cache_size <= 0)
            {
                return false;
            }

            xCollis coll;
            coll.flags = 0xa00;

            bool hit = false;
            ent_info* it = ent_cache;
            ent_info* end = it + ent_cache_size;
            for (; it != end; ++it)
            {
                xSphere o = player_bound();
                if (collide(coll, o, *it->ent))
                {
                    F32 mag = trigger_collision(*it, -vel.y * xabs(coll.norm.y), coll);
                    if (mag != 0.0f)
                    {
                        hit = true;
                        loc += coll.depen * mag;
                        vel -= coll.norm * (mag * coll.norm.dot(vel));
                    }
                }
            }

            return hit;
        }

        F32 hanging_state_type::trigger_collision(ent_info& ei, F32 mag, const xCollis&)
        {
            ei.hits++;

            if (dying)
            {
                return 1.0f;
            }

            xSurface* surf = ei.ent->model->Surf;
            if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
            {
                ouchie(true);
            }

            bool can_splat = true;
            if (mag >= h.collision.hit_velocity)
            {
                if (ei.ent->baseType == eBaseTypeNPC)
                {
                    zNPCCommon& npc = *(zNPCCommon*)ei.ent;
                    if (ei.hits == 1)
                    {
                        npc.Damage(DMGTYP_BUNGEED, &globals.player.ent, &vel);
                        if (npc.IsHealthy())
                        {
                            return 1.0f;
                        }
                        return h.collision.hit_loss;
                    }
                    if (npc.IsHealthy())
                    {
                        return 1.0f;
                    }
                    return 0.0f;
                }
                if (ei.ent->baseType == eBaseTypeButton)
                {
                    zEntButton_Press((_zEntButton*)ei.ent, 0x20);
                    can_splat = false;
                }
            }

            if (mag >= h.collision.damage_velocity && can_splat)
            {
                ouchie(true);
            }

            return 1.0f;
        }

        bool hanging_state_type::collide(xCollis& coll, const xSphere& o, const xEnt& ent) const
        {
            if (!(ent.chkby & 0x10) || !(ent.penby & 0x10))
            {
                return false;
            }

            xSphereHitsBound(&o, &ent.bound, &coll);
            if (!(coll.flags & 0x1))
            {
                return false;
            }

            if (ent.collLev == 0x5)
            {
                xSphereHitsModel(&o, ent.model, &coll);
                if (!(coll.flags & 0x1))
                {
                    return false;
                }
            }

            return true;
        }

        void hanging_state_type::collide_start(xScene& s)
        {
            xSphere o;
            xBound bound;

            o.r = globals.player.ent.bound.sph.r + 0.5f * (last_loc - loc).length();
            o.center = (last_loc + loc) * 0.5f;
            o.center = local_to_world(o.center);

            bound.type = XBOUND_TYPE_BOX;
            xBoxFromSphere(bound.box.box, o);
            xVec3Add(&bound.box.center, &bound.box.box.lower, &bound.box.box.upper);
            xVec3SMul(&bound.box.center, &bound.box.center, 0.5f);
            xQuickCullForBox(&bound.qcd, &bound.box.box);

            ent_cache_size = 0;
            cb_cache_collisions cb(o, ent_cache, ent_cache_size);
            xGridCheckBound(colls_grid, bound, bound.qcd, cb);
            xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
            xGridCheckBound(npcs_grid, bound, bound.qcd, cb);

            xCollis coll;
            coll.flags = 0;
            env_cache.env = s.env;
            env_cache.collide = iSphereHitsEnv(&o, s.env, &coll) != 0;
            env_cache.hits = 0;
        }

        hanging_state_type::cb_cache_collisions::cb_cache_collisions(const xSphere& o,
                                                                     ent_info* ent_cache,
                                                                     S32& ent_cache_size)
            : o(o), ent_cache(ent_cache), ent_cache_size(ent_cache_size)
        {
        }

        void hanging_state_type::update_camera_direction(F32)
        {
            xVec3 start = cam_dir;
            xVec3 dir = (loc - cam_loc).normal();

            cam_dir = start + (dir - start) * h.camera.turn_speed;
            cam_dir.normalize();

            rotate_camera();
        }

        void hanging_state_type::rotate_camera()
        {
            F32 yaw;
            F32 pitch;

            if (cam_dir.y <= -1.0f)
            {
                yaw = 0.0f;
                pitch = 1.5707964f;
            }
            else
            {
                yaw = xatan2(cam_dir.x, cam_dir.z);
                pitch = -xasin(cam_dir.y);
            }

            F32 roll = roll_offset + (yaw - h.camera.view_angle);
            roll_offset *= eh.camera.roll_decay;

            xCameraLookYPR(&globals.camera, 0, yaw, pitch, roll, 0.0f, 0.0f, 0.0f);
        }

        void hanging_state_type::interpolate_camera_loc(const xVec3& dest, F32 dt)
        {
            xMat3x3 m;
            xMat3x3 im;
            xVec3 curr;
            xVec3 goal;

            F32 ang = h.camera.view_angle - h.camera.offset_dir;

            m.right = xVec3::create(icos(ang), 0.0f, isin(ang));
            m.up = xVec3::create(0.0f, 1.0f, 0.0f);
            m.at = xVec3::create(-m.right.z, 0.0f, m.right.x);
            m.right.normalize();
            m.at.normalize();

            xMat3x3RMulVec(&curr, &m, &cam_loc);
            xMat3x3RMulVec(&goal, &m, &dest);

            F32 s = 1.0f - xexp(-fixed.camera.speed * dt);
            curr.x += s * (goal.x - curr.x);
            curr.y += s * (goal.y - curr.y);
            curr.z += s * (goal.z - curr.z);

            xMat3x3Transpose(&im, &m);
            xMat3x3RMulVec(&cam_loc, &im, &curr);
        }

        bool hanging_state_type::update_free_look(F32)
        {
            return false;
        }

        void hanging_state_type::update_animation(F32 dt)
        {
            xAnimPlay* ap = globals.player.ent.model->Anim;

            if (shared.anim_state & 0x80 && ap->Single->CurrentSpeed == 0.0f && !xScrFxIsFading())
            {
                globals.player.DamageTimer = fixed.death_time;

                iColor_tag black = { 0, 0, 0, 0xff };
                iColor_tag clear = { 0, 0, 0, 0 };

                xScrFxFade(&clear, &black, fixed.death_time, NULL, 1);
            }

            RwMatrix& mm = *globals.player.ent.model->Mat;
            ((xMat4x3&)mm).pos = local_to_world(loc);
            xMat3x3Euler((xMat3x3*)&mm, rot, 0.0f, 0.0f);

            F32 dist_frac =
                (loc.y - h.vertical.min_dist) / (h.vertical.max_dist - h.vertical.min_dist);

            if (dive_remaining > 0.0f)
            {
                shared.anim_state = shared.anim_state & ~0x3c | 0x1;
            }
            else if (dist_frac <= fixed.top_anim_frac)
            {
                shared.anim_state = shared.anim_state & ~0x33 | 0x4;
            }
            else if (dist_frac >= 1.0f - fixed.bottom_anim_frac)
            {
                shared.anim_state = shared.anim_state & ~0xf | 0x10;
            }

            switch (shared.anim_state)
            {
            case 0x1:
                play_sound(SOUND_DIVE, 0.0f);
                play_sound(SOUND_ATTACH, 0.0f);
                xAnimPlayStartTransition(ap, shared.anim_tran.dive_start);
                shared.anim_state |= 0x2;
                break;
            case 0x2:
                xAnimPlayStartTransition(ap, shared.anim_tran.dive_stop);
                shared.anim_state &= ~0x2;
                break;
            case 0x4:
                play_sound(SOUND_PEAK, 0.0f);
                xAnimPlayStartTransition(ap, shared.anim_tran.top_start);
                shared.anim_state |= 0x8;
                break;
            case 0x8:
                xAnimPlayStartTransition(ap, shared.anim_tran.top_stop);
                shared.anim_state &= ~0x8;
                break;
            case 0x10:
                xAnimPlayStartTransition(ap, shared.anim_tran.bottom_start);
                shared.anim_state |= 0x20;
                break;
            case 0x20:
                xAnimPlayStartTransition(ap, shared.anim_tran.bottom_stop);
                shared.anim_state &= ~0x20;
                break;
            }

            shared.anim_state &= ~0x15;
            if (shared.anim_state == 0)
            {
                F32 vel_frac =
                    xabs(vel.y / spring_velocity(eh.vertical.rest_dist, 1.0f, eh.vertical.emax,
                                                 eh.vertical.spring, h.vertical.gravity,
                                                 eh.vertical.rest_dist));

                xAnimSingle& xas = *globals.player.ent.model->Anim->Single;
                xas.BilinearLerp[0] = 3.0f * dist_frac;
                xas.BilinearLerp[1] = 2.0f * vel_frac;
            }

            move_wedgie(shared.hook_loc);

            xModelUpdate(globals.player.ent.model, dt);
            xModelEval(globals.player.ent.model);
        }

        void hanging_state_type::update_movement(F32 dt)
        {
            update_heading(dt);
            update_vmovement(dt);
            update_hmovement(dt);
        }

        void hanging_state_type::update_hmovement(F32 dt)
        {
            update_hmovement(loc.x, vel.x, loc.x, vel.x, dt, -stick_loc.x);
            update_hmovement(loc.z, vel.z, loc.z, vel.z, dt, -stick_loc.y);
        }

        void hanging_state_type::update_hmovement(F32& x, F32& v, F32 x0, F32 v0, F32 dt, F32 stick)
        {
            F32 goal = (1.0f + fixed.horizontal.edge_zone) * (h.horizontal.max_dist * stick);
            if (goal > h.horizontal.max_dist)
            {
                goal = h.horizontal.max_dist;
            }
            else if (goal < -h.horizontal.max_dist)
            {
                goal = -h.horizontal.max_dist;
            }

            F32 accel = stick_frac * fixed.horizontal.sway * (goal - x0);

            v = accel * dt + v0;
            x = x0 + (dt * (0.5f * accel * dt) + v * dt);
            v *= fixed.horizontal.decay;
        }

    } // namespace
} // namespace bungee_state
