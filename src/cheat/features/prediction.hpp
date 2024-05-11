#pragma once

#include "feature.hpp"
#include "names.hpp"
#if enable_new_lua
#include "../lua-v2/lua_def.hpp"
#include "../lua-v2/state.hpp"
#endif
#include "../sdk/game/spell_slot.hpp"



namespace features {
    class Prediction final : public IFeature {
    public:
        enum class EHitchance {
            low = 0,
            medium,
            high,
            very_high,
            immobile,
            invalid
        };

        enum class EMultihitLogic {
            tightest_angle,
            max_average_path_length,
            max_average_path_time
        };

        enum class EInvalidReason {
            unknown = 0,
            out_of_range,
            small_path,
            bad_slow,
            bad_haste,
            invalid_prediction,
            windwall_collision
        };

        enum class EHitchanceReason {
            unknown = 0,
            recent_path,
            similar_path_angle,
            fresh_path_angle,
            abnormal_path_length,
            reaction_time,
            post_spell,
            crowd_control,
            dash,
            idle,
            guaranteed_hit,
            predicted_stun,
            spellcast,
            bad_duration,
            slowed
        };

        enum class EAttackType {
            instant,
            missile
        };

        enum class ESpellType {
            none,
            linear,
            circle
        };

        struct PredictionResult {
            bool            valid{ };
            sdk::math::Vec3 position{ };
            sdk::math::Vec3 default_position{ };
            EHitchance      hitchance{ };

            EInvalidReason   error_reason{ };
            EHitchanceReason reason{ };
        };

        enum class MinionTargetingPriority {
            None = 0,
            NearestChampion,
            NearestMinion,
            ChampionAttackingAllyMinion,
            TurretAttackingAllyMinion,
            MinionAttackingAllyMinion,
            MinionAttackingAllyChampion,
            ChampionAttackingAllyChampion
        };

        enum class MinionState {

            Idle = 0,
            Moving,
            TargetingUnit
        };

        struct MinionAI {

            sdk::game::Object* unit{ };
            int16_t            index{};
            unsigned           network_id{};


            Object::EMinionType type{ };

            float state_change_time{ };
            float acquisition_radius{ 500.f };

            bool has_selected_target{ };
            MinionTargetingPriority target_priority{ };

            MinionState state{ };
            float last_targeting_time{ };
            int16_t     last_target_index{ };
            Vec3        targeting_position{};

            // moving
            float last_move_time{};

            float attack_register_time{ };
            float second_attack_register_time{ };
            float third_attack_register_time{ };

            //data

            //0.8

            float get_attack_cast_delay( ) {

                switch ( this->type )
                {
                case Object::EMinionType::melee:
                    return 0.393f;
                case Object::EMinionType::ranged:
                    return 0.469765f;
                case Object::EMinionType::siege:
                    return 0.3f;
                case Object::EMinionType::super:
                    return 0.f;
                default:
                    return 0.f;
                }
            }

            float get_attack_delay() {

                switch (this->type)
                {
                case Object::EMinionType::melee:
                    return 0.8f;
                case Object::EMinionType::ranged:
                    return 1.49925f;
                case Object::EMinionType::siege:
                    return 1.f;
                case Object::EMinionType::super:
                    return 0.f;
                default:
                    return 0.f;
                }
            }
        };

        struct AttackInstance {
            sdk::game::Object* source;
            sdk::game::Object* target;
            int16_t            source_index{ };
            int16_t            target_index{ };
            float              damage{ };
            float              start_time{ };
            float              land_time{ };
            float              end_time{ };
            float              server_cast_time{ };
            float              total_cast_time{ };
            float              windup_time{ };
            float              attack_time{ };
            float              missile_speed{ };

            bool  landed{ };
            float lowest_delta{ };

            float target_hp{ };
            bool  health_updated{ };

            uint32_t        missile_nid{ };
            int16_t         missile_index{ };
            bool            missile_found{ };
            bool            missile_tracked{ };
            sdk::math::Vec3 missile_last_position{ };
            float           missile_position_delta{ };

            sdk::game::Object::EMinionType type{ };
        };

        struct TurretShot {
            sdk::game::Object* source;
            sdk::game::Object* target;
            uint32_t           source_nid{ };
            uint32_t           target_nid{ };

            int16_t target_index{ };

            sdk::math::Vec3 target_position{ };

            float damage{ };
            float server_cast_time{ };
            float end_time{ };
            float land_time{ };
            float windup_time{ };

            bool landed{ };

            CHolder* missile;
            uint32_t missile_nid{ };
            int16_t  missile_index{ };
            float    missile_speed{ };
            bool     missile_found{ };
        };

        enum class EInputType {
            unknown,
            order_move,
            dash,
            spellcast,
            order_stop
        };

        struct InputEvent {
            EInputType type{ };

            float time{ };

            // move order data
            float move_length{ };
            float move_angle{ };

            sdk::math::Vec3 start_position{ };
            sdk::math::Vec3 end_position{ };
        };

        struct PredictionInstance {
            unsigned network_id{ };
            int16_t  index{ };

            std::vector< float >      paths{ };
            std::vector< InputEvent > events{ };

            float path_change_time{ };

            float last_path_duration{ };


            float idle_time{ };

            // tracking path angle
            float path_angle{ };
            float last_path_angle{ };

            int             identical_path_count{ };
            sdk::math::Vec3 path_direction{ };

            // paths after action
            int paths_after_cast{ };
            int paths_after_dash{ };

            // spellcast tracking
            bool            cast_detected{ };
            bool            was_autoattack{ };
            float           server_cast_time{ };
            float           last_cast_time{ };
            sdk::math::Vec3 last_cast_position{ };

            // animation lock logic
            float           lock_start_time{ };
            float           lock_end_time{ };
            float           animation_server_cast_time{ };
            sdk::math::Vec3 animation_position{ };

            [[nodiscard]] auto is_animation_locked( ) const -> bool{ return this->lock_end_time > *g_time; }

            bool is_standing_still{ };

            // dash things
            float dash_start_time{ };

            sdk::math::Vec3 last_path_end_position{ };
            sdk::math::Vec3 last_path_start_position{ };
            sdk::math::Vec3 previous_path_position{ };

            // buff tracking
            bool is_slowed{ };
            bool is_hasted{ };

            bool  is_stunned{ };
            int   type_of_stun{ };
            float stun_end_time{ };

            float last_movespeed_change_time{ };
            float last_move_speed{ };
            bool  has_new_speed{ };

            float base_movement_speed{ };

            // advanced tracking
            float last_action_time{ };

            float                path_length{ };
            float                last_path_length{ };
            std::vector< float > path_lengths{ };
            bool                 is_shorter_path{ };

            bool is_fresh_path{ };
            int  fresh_path_count{ };

            int                similiar_path_count{ };
            int                last_similiar_path_count{ };
            std::vector< int > similiar_paths{ };

            float path_duration{ };
            float path_distance{ };

            bool was_pathing{ };
            bool is_pathing{ };

            // events logic
            [[nodiscard]] auto is_path_confirmed( ) const -> bool{
                if ( this->events.size( ) < 3 ) return false;

                const auto previous_event = events[ events.size( ) - 2 ];
                const auto current_event  = events[ events.size( ) - 1 ];

                if ( previous_event.type != current_event.type || current_event.type != EInputType::order_move ||
                    current_event.time - previous_event.time > 3.f )
                    return false;

                const auto confirm_delay = current_event.time - previous_event.time;
                if ( confirm_delay > 0.3f ) return false;

                if ( previous_event.move_length > current_event.move_length ) return false;

                return identical_path_count > 0;
            }

            [[nodiscard]] auto is_path_unreliable( ) const -> bool{
                if ( this->events.size( ) < 3 ) return false;

                const auto previous_event = events[ events.size( ) - 2 ];
                const auto current_event  = events[ events.size( ) - 1 ];

                if ( previous_event.type != current_event.type || current_event.type != EInputType::order_move ||
                    current_event.time - previous_event.time > 3.f )
                    return false;

                return current_event.move_length < 275.f || current_event.move_length < previous_event.move_length ||
                    current_event.time - previous_event.time >= 0.7f;
            }

            [[nodiscard]] auto is_path_secure( ) const -> bool{
                if ( this->events.size( ) < 3 ) return false;

                const auto previous_event = events[ events.size( ) - 2 ];
                const auto current_event  = events[ events.size( ) - 1 ];

                if ( previous_event.type != current_event.type || current_event.type != EInputType::order_move ||
                    current_event.time - previous_event.time > 1.f )
                    return false;

                return current_event.time - previous_event.time < 0.5f;
            }
        };

        enum class EActionType {
            none = 0,
            new_path,
            spell,
        };

        enum class EChangeType {
            none = 0,
            fresh,
            equal,
            minor,
            average,
            major
        };

        enum class ESpellDamageType {
            physical = 0,
            magic,
            true_damage
        };

        struct ActionInstance {
            EActionType type{ };

            // new path
            sdk::math::Vec3 path_end{ };
            float           path_length{ };
            float           path_angle{ };
            float           duration_on_path{ };

            // spellcast
            float           server_cast_time{ };
            bool            is_autoattack{ };
            sdk::math::Vec3 direction{ };

            float time{ };
            bool  valid{ };
        };

        struct PatternCondition {
            EActionType action_type{ };
            EChangeType angle_difference{ };
        };

        struct ActionPattern {
            std::vector< PatternCondition > conditions{ };

            EChangeType predicted_change{ };
            bool        is_submitted{ };

            int pattern_size{ };

            int total_predictions{ };
            int matches{ };

            int   creation_action_count{ };
            float creation_time{ };
        };

        struct PatternInstance {
            int16_t     index{ };
            unsigned    network_id{ };
            std::string champion_name{ };

            ActionInstance                last_action{ };
            std::vector< ActionInstance > recent_actions{ };
            std::vector< ActionPattern >  patterns{ };

            int total_actions{ };

            auto remove_outdated_actions( ) -> void{
                const auto to_remove = std::ranges::remove_if(
                    recent_actions,
                    [&]( const ActionInstance& inst ) -> bool{ return *g_time - inst.time > 20.f || !inst.valid; }
                );

                if ( to_remove.empty( ) ) return;

                recent_actions.erase( to_remove.begin( ), to_remove.end( ) );
            }

            auto remove_action( const ActionInstance& action ){
                const auto to_remove = std::ranges::remove_if(
                    recent_actions,
                    [&]( const ActionInstance& inst ) -> bool{
                        return inst.type == action.type && inst.time == action.time;
                    }
                );

                if ( to_remove.empty( ) ) return;

                recent_actions.erase( to_remove.begin( ), to_remove.end( ) );
            }
        };

        struct VoteInstance {
            EChangeType type{ };
            int         votes{ };
        };

        struct SpecialAttackInstance {
            int16_t target_index{ };
            float   damage{ };
            float   land_time{ };

            bool is_autoattack{ };
            // conditional damage
            bool                  has_condition{ };
            sdk::game::ESpellSlot condition_slot{ };

            float travel_time{ };
            float creation_time{ };
        };

        struct IgnoredMissile {
            uint32_t network_id{ };
            float    ignore_start_time{ };
        };

        struct YasuoWall {
            bool is_active{ };

            int16_t start_index{ };
            int16_t end_index{ };
            float   end_time{ };
        };

        struct HeroAttack {
            int16_t source_index{ };
            int16_t target_index{ };

            float start_time{ };
            float server_cast_time{ };
            float land_time{ };
            float end_time{ };

            float damage{ };
            bool  is_teammate{ };

            EAttackType type{ };

            bool     missile_found{ };
            int16_t  missile_index{ };
            uint32_t missile_nid{ };

            float missile_speed{ };
        };

        struct HeroData {
            int16_t index{ };
            bool    is_ally{ };

            float last_attack_time{ };
            int   tries{ };
            bool  ready{ };

            bool     has_missile{ };
            unsigned last_missile_nid{ };
            float    missile_speed{ };
        };

        struct SpellAttribute {
            ESpellDamageType damage_type{ };
            EAttackType      spell_type{ };
            float            missile_speed{ };

            std::function< float(
                hash_t  spell_name,
                int16_t source_index,
                int16_t target_index,
                float   server_cast_time,
                float   land_time
            ) > get_damage{ };

            bool        delayed_cast{ };
            std::string name{ };
        };

        struct HeroSpell {
            int16_t source_index{ };
            int16_t target_index{ };

            float start_time{ };
            float server_cast_time{ };
            float land_time{ };

            float raw_server_cast_time{ };

            int slot{ };

            float damage{ };
            bool  is_teammate{ };

            EAttackType    type{ };
            SpellAttribute attributes{ };

            bool delayed_cast{ };

            std::string name{ };
            hash_t      name_hash{ };

            bool     missile_found{ };
            int16_t  missile_index{ };
            uint32_t missile_nid{ };

            float missile_speed{ };
        };

    public:
        auto run( ) -> void override;
        auto on_draw( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::prediction; }
#if enable_lua
        auto initialize_lua( sol::state* state ) -> void override;
#endif

#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_prediction"; }
#endif

        enum EPredictionFlags : int32_t {
            include_ping                    = 1 << 0,
            render_thread                   = 1 << 1,
            extend_range_with_hitbox        = 1 << 2,
            check_range_from_local_position = 1 << 4,
            extend_crowdcontrol             = 1 << 8
        };

        auto predict(
            int16_t         target_index,
            float           projectile_range,
            float           projectile_speed,
            float           projectile_width,
            float           delay,
            sdk::math::Vec3 source_position,
            bool            extend,
            int32_t         flags,
            ESpellType      spell_type
        ) -> PredictionResult;

        auto predict(
            int16_t         target_index,
            float           projectile_range,
            float           projectile_speed,
            float           projectile_width,
            float           delay,
            sdk::math::Vec3 source_position,
            bool            extend,
            int32_t         flags
        ) -> PredictionResult{
            return predict(
                target_index,
                projectile_range,
                projectile_speed,
                projectile_width,
                delay,
                source_position,
                extend,
                flags,
                ESpellType::none
            );
        }

        auto predict(
            int16_t         target_index,
            float           projectile_range,
            float           projectile_speed,
            float           projectile_width,
            float           delay,
            sdk::math::Vec3 source_position,
            bool            extend
        ) -> PredictionResult{
            return predict(
                target_index,
                projectile_range,
                projectile_speed,
                projectile_width,
                delay,
                source_position,
                extend,
                include_ping,
                ESpellType::none
            );
        }

        auto predict(
            int16_t         target_index,
            float           projectile_range,
            float           projectile_speed,
            float           projectile_width,
            float           delay,
            sdk::math::Vec3 source_position
        ) -> PredictionResult{
            return predict(
                target_index,
                projectile_range,
                projectile_speed,
                projectile_width,
                delay,
                source_position,
                false,
                include_ping,
                ESpellType::none
            );
        }

        auto predict(
            int16_t target_index,
            float   projectile_range,
            float   projectile_speed,
            float   projectile_width,
            float   delay
        ) -> PredictionResult{
            return predict(
                target_index,
                projectile_range,
                projectile_speed,
                projectile_width,
                delay,
                sdk::math::Vec3( ),
                false,
                include_ping,
                ESpellType::none
            );
        }

        [[nodiscard]] auto predict_default(
            int32_t target_index,
            float   time,
            bool    include_extra_tick
        ) const -> std::optional< sdk::math::Vec3 >;

        [[nodiscard]] auto predict_default(
            int32_t target_index,
            float   time
        ) const -> std::optional< sdk::math::Vec3 >{ return predict_default( target_index, time, true ); }

        auto predict_health(
            const sdk::game::Object* object,
            float                    time,
            bool                     multiple_attacks,
            bool                     ignore_turret,
            bool                     ignore_minions,
            bool                     override_delay
        ) -> float;

        auto predict_health(
            const sdk::game::Object* object,
            float                    time
        ) -> float{ return predict_health( object, time, false, false, false, false ); }

        auto predict_health(
            const sdk::game::Object* object,
            float                    time,
            bool                     multiple_attacks
        ) -> float{ return predict_health( object, time, multiple_attacks, false, false, false ); }

        auto predict_health(
            const sdk::game::Object* object,
            float                    time,
            bool                     multiple_attacks,
            bool                     ignore_turret
        ) -> float{ return predict_health( object, time, multiple_attacks, ignore_turret, false, false ); }

        auto predict_health(
            const sdk::game::Object* object,
            float                    time,
            bool                     multiple_attacks,
            bool                     ignore_turret,
            bool                     ignore_minions
        ) -> float{ return predict_health( object, time, multiple_attacks, ignore_turret, ignore_minions, false ); }


        auto predict_minion_health( int16_t index, float delay, bool predict_multiple_attacks ) -> float;

        auto predict_minion_health( int16_t index, float delay ) -> float{
            return predict_minion_health( index, delay, false );
        }

        [[nodiscard]] auto predict_possible_minion_health(
            int16_t index,
            float   delay,
            bool    predict_multiple_attacks = false
        ) const -> float;

        // advanced prediction logic
        static auto predict_dash(
            int16_t         target_index,
            float           range,
            float           speed,
            float           width,
            float           delay,
            sdk::math::Vec3 source_position
        ) -> PredictionResult;
        static auto predict_dash_from_spell(
            int16_t         target_index,
            float           range,
            float           speed,
            float           width,
            float           delay,
            sdk::math::Vec3 source_position
        ) -> PredictionResult;
        static auto predict_blink(
            int16_t         target_index,
            float           range,
            float           speed,
            float           width,
            float           delay,
            sdk::math::Vec3 source_position
        ) -> PredictionResult;

        // new prediction
        static auto predict_movement(
            int16_t index,
            float   duration,
            float   hitbox_size = 0.f
        ) -> std::optional< sdk::math::Vec3 >;
        static auto get_server_position( int16_t index ) -> sdk::math::Vec3;

        static auto get_champion_radius( hash_t name ) -> float;

#if enable_lua
        auto lua_predict(
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/target_index,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/projectile_range,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/projectile_speed,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/projectile_width,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/delay,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/source_position,
            const sol::object /*ReSharper disable once CppPassValueParameterByConstReference*/extend
        ) -> sol::object{
            lua_arg_check_ct( target_index, int16_t, "target_index" )
            lua_arg_check_ct( projectile_range, float, "projectile_range" )
            lua_arg_check_ct( projectile_speed, float, "projectile_speed" )
            lua_arg_check_ct( projectile_width, float, "projectile_width" )
            lua_arg_check_ct( delay, float, "delay" )
            if ( source_position.get_type( ) != sol::type::nil )
                lua_arg_check_ct( source_position, sdk::math::Vec3, "source_position" )
            if ( extend.get_type( ) != sol::type::nil )
                lua_arg_check_ct( extend, bool, "extend" )

            const auto result = predict(
                target_index.as< int16_t >( ),
                projectile_range.as< float >( ),
                projectile_speed.as< float >( ),
                projectile_width.as< float >( ),
                delay.as< float >( ),
                source_position.get_type( ) == sol::type::nil
                    ? sdk::math::Vec3( )
                    : source_position.as< sdk::math::Vec3 >( ),
                extend.get_type( ) == sol::type::nil ? false : extend.as< bool >( )
            );

            return sol::make_object(
                g_lua_state2,
                result
            );
        }

        [[nodiscard]] auto lua_predict_default(
            // ReSharper disable CppPassValueParameterByConstReference
            const sol::object target_index,
            const sol::object time
            // ReSharper restore CppPassValueParameterByConstReference
        ) const -> sol::object{
            lua_arg_check_ct( target_index, int32_t, "number" )
            lua_arg_check_ct( time, float, "number" )

            const auto r = predict_default( target_index.as< int32_t >( ), time.as< float >( ) );
            if ( !r ) return sol::nil;

            return sol::make_object( g_lua_state2, *r );
        }

        auto lua_predict_minion_health(
            // ReSharper disable CppPassValueParameterByConstReference
            const sol::object index,
            const sol::object delay,
            const sol::object predict_multiple_attacks
            // ReSharper restore CppPassValueParameterByConstReference
        ) -> sol::object{
            lua_arg_check_ct( index, int16_t, "number" )
            lua_arg_check_ct( delay, float, "number" )
            if ( predict_multiple_attacks.get_type( ) != sol::type::nil )
                lua_arg_check( predict_multiple_attacks, bool )

            return sol::make_object(
                g_lua_state2,
                predict_minion_health(
                    index.as< int16_t >( ),
                    delay.as< float >( ),
                    predict_multiple_attacks.get_type( ) == sol::type::nil
                        ? false
                        : predict_multiple_attacks.as< bool >( )
                )
            );
        }
#endif

        auto minion_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            unsigned               ignored_network_id,
            float                  damage_override
        ) -> bool;

        auto minion_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width
        ) -> bool{ return minion_in_line( start_pos, end_pos, projectile_width, 0, 0.f ); }

        auto minion_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            unsigned               ignored_network_id
        ) -> bool{ return minion_in_line( start_pos, end_pos, projectile_width, ignored_network_id, 0.f ); }

        [[nodiscard]] auto minion_in_line_predicted(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            float                  cast_time,
            float                  missile_speed
        ) const -> bool;

        static auto turret_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width
        ) -> bool;

        [[nodiscard]] auto champion_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            bool                   edge_range,
            float                  cast_time,
            float                  missile_speed,
            unsigned               ignored_network_id = 0
        ) const -> bool;

        [[nodiscard]] auto minion_in_circle(
            const sdk::math::Vec3& center,
            float                  radius,
            unsigned               ignored_network_id = 0
        ) const -> bool;

        [[nodiscard]] auto minion_in_circle_predicted(
            const sdk::math::Vec3& center,
            float                  radius,
            float                  traveltime,
            bool                   edge_range = false
        ) const -> bool;

        auto count_minions_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            unsigned               ignored_network_i
        ) -> int32_t;

        auto count_minions_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width
        ) -> int32_t{ return count_minions_in_line( start_pos, end_pos, projectile_width, 0 ); }

        [[nodiscard]] auto minions_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            unsigned               ignored_network_id = 0,
            float                  cast_delay         = 0.f,
            float                  travel_time        = 0.f
        ) const -> int;

        static auto get_minion_collision_position(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            bool                   edge_range,
            float                  cast_time,
            float                  missile_speed
        ) -> std::optional< sdk::math::Vec3 >;

        static auto get_champion_collision_position(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            bool                   edge_range,
            float                  cast_time,
            float                  missile_speed
        ) -> std::optional< sdk::math::Vec3 >;

        static auto get_skillshot_collision_position(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos,
            float                  projectile_width,
            bool                   edge_range,
            float                  cast_time,
            float                  missile_speed
        ) -> std::optional< sdk::math::Vec3 >;

        [[nodiscard]] auto windwall_in_line(
            const sdk::math::Vec3& start_pos,
            const sdk::math::Vec3& end_pos
        ) const -> bool;
        [[nodiscard]] auto is_windwall_active( ) const -> bool{ return m_windwall.is_active; }

        [[nodiscard]] auto get_attack_count( const int16_t index ) const -> int32_t;

        [[nodiscard]] auto is_minion_in_danger( int16_t index ) const -> bool;
        auto               add_special_attack(
            int16_t               index,
            float                 damage,
            float                 travel_time,
            bool                  cast_condition = false,
            sdk::game::ESpellSlot condition_slot = sdk::game::ESpellSlot::q,
            bool                  is_autoattack  = false
        ) -> void;
        static auto get_time_on_current_path( int16_t index ) -> float;

        auto is_object_turret_target( sdk::game::Object* object ) const -> bool;

        // get averages of tracked data
        auto get_average_path_time( unsigned network_id ) -> float;

        [[nodiscard]] auto get_incoming_attack_count( int16_t target_index ) const -> int;

        auto get_minion_tick_damage(int16_t index) -> float;

        // new damage pred stuff
        auto get_incoming_champion_damage( int16_t target_index, float time, float strict_calculation = true ) -> float;

        struct ConstantDamage {
            float damage{ };

            float cast_delay{ };
            float attack_delay{ };
            float first_tick_interval{ };

            bool needs_lasthit{ };

            auto compile_damage( float duration ) -> float {

                float multiplier =  (duration - this->first_tick_interval) / this->attack_delay;

                if (multiplier > std::round( multiplier )  )
                {
                    float cast_delay_modifier = multiplier - std::round(multiplier);
                    if (this->attack_delay * cast_delay_modifier > this->cast_delay) multiplier += 1.f;

                }

                return this->damage * multiplier;
            }
        };

        auto simulate_minion_damage(int16_t target_index, float duration) -> float;
        auto should_damage_minion(int16_t target_index, float attack_damage, float attack_cast_delay,
                                  float attack_delay) -> bool;

    private:
        // targetable spell tracking
        auto               update_champion_spells( ) -> void;
        auto               update_existing_champion_spells( ) -> void;
        [[nodiscard]] auto is_champion_spell_active( int16_t index, int slot, float server_cast_time ) const -> bool;
        auto               remove_champion_spell( int16_t index, int slot, float server_cast_time ) -> void;

        [[nodiscard]] auto get_targetable_spell_attributes( hash_t name ) const -> std::optional< SpellAttribute >;

        // drawing
#if __DEBUG
        auto draw_hppred_debug( ) const -> void;
#endif

        // prediction logging stuff
        auto update_prediction_data( ) -> void;
        auto get_data_index( unsigned network_id ) -> int;
        auto process_path( const int16_t index, const int data_index ) -> void;
        auto update_prediction( int16_t index ) -> void;
        auto remove_invalid_paths( ) -> void;

        // pattern detection
        auto detect_patterns( ) -> void;
        auto create_new_patterns( ) -> void;

        // debug drawings
        auto draw_prediction_debug() -> void;

        static auto get_angle_change_type( float angle ) -> EChangeType;

        [[nodiscard]] auto matches_existing_pattern( const ActionPattern& instance ) const -> bool;


        // minion attack tracking
        auto               update_minion_attacks( ) -> void;
        auto               remove_attack( int16_t index ) -> void;
        auto               update_existing_attacks( ) -> void;
        [[nodiscard]] auto is_attack_active( int16_t index ) const -> bool;
        auto               get_attack( int16_t index ) -> AttackInstance*;

        // turret attack tracking
        auto               remove_turret_shot( unsigned network_id ) -> void;
        [[nodiscard]] auto is_turret_attack_active( unsigned network_id ) const -> bool;

        // yasuo wall tracking
        auto update_windwall( ) -> void;

        // champion data gathering
        auto               update_hero_data( ) -> void;
        [[nodiscard]] auto is_hero_tracked( int16_t index ) const -> bool;
        [[nodiscard]] auto get_hero_missile_speed( int16_t index ) const -> float;

        // champion health prediction
        auto               update_hero_attacks( ) -> void;
        auto               update_existing_hero_attacks( ) -> void;
        [[nodiscard]] auto is_hero_attack_active( int16_t index, float server_cast_time ) const -> bool;
        auto               remove_hero_attack( int16_t index, float server_cast_time ) -> void;

        auto update_special_attacks( ) -> void;
        auto remove_invalid_special_attacks( ) -> void;
        auto remove_special_attack( int16_t target_index, float damage, float creation_time ) -> void;

        static auto is_blink_spell( hash_t name ) -> bool;
        static auto is_dash_spell( hash_t name ) -> bool;

        static auto get_dash_speed( hash_t name, float movespeed = 0.f ) -> float;
        static auto get_fixed_range( hash_t name ) -> std::optional< float >;
        static auto get_spell_max_range( hash_t name ) -> std::optional< float >;

        static auto is_channel_spell( hash_t name ) -> bool;
        static auto is_channel_blink( hash_t name ) -> bool;

        static auto fix_server_cast_time( hash_t name, float server_cast_time ) -> float;

        auto update_ignored_missiles( ) -> void;
        auto remove_ignored_missile( unsigned network_id ) -> void;

        // stat calculation
        static auto get_normal_move_speed( int16_t index ) -> float;
        static auto calculate_movement_speed( int16_t index, float time ) -> float;

        static auto get_animation_lock_duration( int16_t index ) -> float;

         // minion ai logic
        auto update_minions() -> void;
        auto draw_minions() -> void;
        bool is_minion_attacking( int16_t index, int16_t target_index ) {

            for (auto inst : m_minions) {

                if (inst.index != index || inst.state != MinionState::TargetingUnit ||
                    inst.last_target_index != target_index)
                    continue;

                return true;
            }

            return false;
        }

        auto is_minion_instance_active(unsigned network_id) -> bool;
        auto remove_minion_instance(unsigned network_id) -> void;

    private:
        float m_last_attacks_update_time{ };

        std::vector< PatternInstance > m_pattern_instances{ };

        bool m_pattern_initialized{ };

        float m_last_hero_attacks_update_time{ };
        float m_last_champion_spells_update_time{ };

        // wall prediction
        sdk::math::Polygon m_wall_polygon{ };
        float              m_last_wall_calculation_time{ };
        sdk::math::Vec3    m_closest_safe_point{ };
        sdk::math::Vec3    m_base_position{ };


        // hitbox simulation
        std::vector< sdk::math::Vec3 > m_dodge_points{ };
        sdk::math::Polygon             m_selected_hitbox{ };
        float                          m_last_simulation_time{ };
        sdk::math::Vec3                m_selected_point{ };
        sdk::math::Vec3                m_adjusted_pred{ };

    private:
        std::vector< PredictionInstance > m_prediction_data{ };
        std::deque< AttackInstance >      m_attacks{ };
        std::deque< IgnoredMissile >      m_ignored_missiles{ };

        std::deque< SpecialAttackInstance > m_special_attacks{ };

        std::vector<MinionAI> m_minions{ };

        std::deque< HeroAttack > m_hero_attacks{ };
        std::deque< HeroData >   m_hero_data{ };

        std::deque< HeroSpell > m_hero_spells{ };
        float                   m_new_path_time{ };

        std::deque< TurretShot > m_turret_attacks{ };
        YasuoWall                m_windwall{ };
    };
}
