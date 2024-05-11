#pragma once

#include "feature.hpp"
#include "names.hpp"

#include "orbwalker.hpp"
#include "prediction.hpp"
#include "../sdk/game/spell_slot.hpp"


namespace features {
    class SpellDetector final : public IFeature {
    public:
        enum class ESpellType {
            none,
            line,
            circle,
            rectangle,
            arc,
            cone
        };

        enum class ESpecialSpell {
            none = 0,
            ashe_r,
            jinx_r
        };

        enum class ECheckType {
            none = 0,
            jarvan_iv_q,
            orianna_self_r,
            orianna_ally_r
        };

        enum class EParticleSpell {
            none = 0,
            zac_e,
            ekko_w,
            irelia_e,
            lux_r,
            jhin_w
        };

        struct IgnoredMissile {
            unsigned network_id{ };

            float start_time{ };
        };

        struct IgnoredSpell {
            int16_t         source_index{ };
            float           server_cast_time{ };
            sdk::math::Vec3 end_position{ };
        };

        struct IgnoredObject {
            unsigned network_id{ };

            float start_time{ };
        };

        struct SpellConfigValues {
            bool enabled{ };
            int  danger_level{ };
            int  max_health_to_ignore{ };

            bool valid{ };
        };

        struct ObjectInstance {
            ObjectInstance( ) = default;

            ObjectInstance( const float radius, const int32_t danger, const ESpellType type, const bool cc )
                : radius( radius ),
                danger( danger ),
                type( type ),
                cc( cc ){
            }

            int16_t  index{ };
            unsigned network_id{ };

            ESpellType      type{ };
            sdk::math::Vec3 position;

            int   danger{ };
            float radius{ };
            bool  cc{ };

            float start_time{ };
            float end_time{ };
        };

        struct MissileInstance {
            ESpellType  type{ };
            int16_t     index{ };
            unsigned    network_id{ };
            std::string name{ };

            sdk::math::Vec3 start_position{ };
            sdk::math::Vec3 end_position{ };
            sdk::math::Vec3 position{ };

            float start_time{ };
            float end_time{ };

            // static data
            float speed{ };
            float range{ };
            float radius{ };
            float windup_time{ };
            int   danger{ };
            bool  cc{ };
            bool  collision{ };

            bool has_edge_radius{ true };

            float missile_spawn_time{ };
            float distance_to_end{ };
            bool  is_initialized{ };

            // special
            bool ignore_missile{ };
            bool manual_update{ };

            float allow_dodge{ true };
            float ignore_health_threshold{ 100.f };

            ESpecialSpell special_type{ };

            [[nodiscard]] auto get_dynamic_line_endpos( ) const -> sdk::math::Vec3{
                if ( this->speed == 0.f ) return this->end_position;

                const auto extend_distance = this->speed * ( 0.75f + this->radius * 2.f / g_local->movement_speed );

                const auto end_position = extend_distance > this->position.dist_to( this->end_position )
                                              ? this->end_position
                                              : this->position.extend( this->end_position, extend_distance );

                if ( !this->collision ) return end_position;

                const auto collision = g_features->prediction->get_skillshot_collision_position(
                    this->position,
                    end_position,
                    this->radius,
                    this->has_edge_radius,
                    0.f,
                    this->speed
                );

                return collision.has_value( ) ? collision.value( ) : end_position;
            }

            auto time_till_impact( const Vec3 position ) const -> float{
                switch ( this->type ) {
                case ESpellType::line:
                {
                    if ( this->speed > 0.f ) {
                        const auto current_position = this->position;

                        if ( const auto projection = position.project_on( current_position, this->end_position );
                            projection.is_on_segment ) {
                            return fmax(
                                0.f,
                                ( projection.line_point.dist_to( current_position ) - this->radius - (
                                    this->has_edge_radius ? g_features->orbwalker->get_bounding_radius( ) : 0.f ) ) /
                                this->speed
                            );
                        }

                        return 0.f;
                    }

                    return this->end_time - *g_time;
                }
                case ESpellType::circle:
                case ESpellType::cone:
                    return this->end_time - *g_time;
                default:
                    return 0.f;
                }
            }

            auto should_dodge( ) const -> bool{
                const auto health_percent = g_local->health / g_local->max_health;

                return allow_dodge && health_percent <= ignore_health_threshold / 100.f && g_config->evade.dodge_fow->
                    get< bool >( );
            }

            auto is_dangerous( ) const -> bool{
                return this->danger >= g_config->evade.humanizer_dangerous_threshold->get< int >( ) ||
                    g_config->evade.humanizer_crowdcontrol_dangerous->get< bool >( ) && this->cc;
            }
        };

        struct SpellInstance {
            SpellInstance( ) = default;

            SpellInstance(
                const float      speed,
                const float      range,
                const float      radius,
                const float      windup_time,
                const int32_t    danger,
                const ESpellType type,
                const bool       cc,
                const bool       collision
            )
                : type( type ),
                speed( speed ),
                range( range ),
                radius( radius ),
                windup_time( windup_time ),
                danger( danger ),
                collision( collision ),
                cc( cc ){
            }

            std::shared_ptr< Object > source{ };
            int32_t                   source_index{ };

            ESpellType           type{ ESpellType::none };
            sdk::game::SpellSlot slot{ };

            std::shared_ptr< sdk::game::Object > missile_obj{ };
            unsigned                             missile_nid{ };
            std::string                          missile_name{ };

            sdk::math::Vec3 start_pos{ };
            sdk::math::Vec3 current_pos{ };
            sdk::math::Vec3 end_pos{ };
            sdk::math::Vec3 raw_end_pos{ };

            float speed{ };
            float base_speed{ };
            float range{ };
            float radius{ };
            float windup_time{ };
            float total_cast_time{ };
            int   danger{ };

            bool has_edge_radius{ true };


            bool                    is_particle_spell{ };
            int16_t                 particle_index{ };
            unsigned                particle_nid{ };
            std::vector< unsigned > related_particles{ };

            EParticleSpell particle_type{ };

            // cone specific
            float angle{ };

            bool  allow_dodge{ true };
            float ignore_health_threshold{ 100.f };

            // sticking to caster
            bool            stick_during_cast{ };
            bool            stick_full_duration{ };
            bool            is_direction_locked{ };
            sdk::math::Vec3 direction{ };

            // relative hitbox
            sdk::math::Vec3 base_position{ };
            float           base_distance_to_start{ };
            float           base_distance_to_end{ };
            bool            is_hitbox_relative{ };

            bool            update_direction{ };
            sdk::math::Vec3 raw_start_pos{ };

            // windup checking
            bool       should_run_check{ };
            ECheckType check_type{ };
            int16_t    check_target_index{ };

            ESpecialSpell special_type{ };

            float start_time{ };
            float server_cast_time{ };
            float end_time{ };

            std::string spell_name{ };

            bool missile_found{ };

            int32_t         mis_index{ };
            sdk::math::Vec3 mis_start_position{ };
            sdk::math::Vec3 mis_end_position{ };

            sdk::math::Vec3 mis_last_position{ };

            bool collision{ };
            bool cc{ };

            // NEW: Pre-calculated hitbox data
            sdk::math::Polygon             hitbox_area{ };
            sdk::math::Polygon             tether_area{ };

            std::vector< sdk::math::Vec3 > dodge_points{ };
            std::vector<sdk::math::Vec3> tether_points{ };

            float area_update_time{ };

            auto update_hitbox( ) -> void{
                const auto tether_radius = g_config->evade.tether_distance->get< int >( ) + g_config->evade.
                    extra_distance->get
                    < int >( );

                switch ( this->type ) {
                case ESpellType::line: {
                    break;
                }
                case ESpellType::circle:
                {
                    this->hitbox_area = {
                        g_render->get_3d_circle_points(
                            this->end_pos,
                            this->radius + ( this->has_edge_radius
                                                 ? g_features->orbwalker->get_bounding_radius( )
                                                 : 0.f ) +
                            g_config->evade.extra_distance->get< int >( ),
                            32
                        )
                    };

                    this->tether_area = {
                        g_render->get_3d_circle_points(
                            this->end_pos,
                            tether_radius + this->radius +
                            ( this->has_edge_radius ? g_features->orbwalker->get_bounding_radius( ) : 0.f ),
                            32
                        )
                    };
                }
                default:
                    return;
                }

                this->area_update_time = *g_time;
            }

            auto get_future_hitbox( const bool is_tether, const float time = 0.f ) -> sdk::math::Polygon{
                const auto tether_radius =
                    g_config->evade.tether_distance->get< int >( ) + g_config->evade.extra_distance->get< int >( );

                switch ( this->type ) {
                case ESpellType::line:
                {
                    if ( is_tether ) {
                        return sdk::math::Rectangle{
                            get_current_position( time ),
                            this->end_pos,
                            this->radius
                        }.to_polygon(
                            tether_radius +
                            static_cast< int >( this->has_edge_radius
                                                    ? g_features->orbwalker->get_bounding_radius( )
                                                    : 0.f )
                        );
                    }

                    return sdk::math::Rectangle{ get_current_position( time ), this->end_pos, this->radius }.to_polygon(
                        static_cast< int >( this->has_edge_radius
                                                ? g_features->orbwalker->get_bounding_radius( )
                                                : 0.f )
                    );
                }
                default:
                    break;
                }

                return is_tether ? this->tether_area : this->hitbox_area;
            }

            auto get_current_position( const float extra_time = 0.f ) const -> sdk::math::Vec3{
                const auto current_time = *g_time + extra_time;

                if ( this->server_cast_time < current_time && this->type == ESpellType::line ) {
                    const auto time_traveled     = current_time - this->server_cast_time;
                    auto       distance_traveled = time_traveled * this->base_speed;

                    switch ( this->special_type ) {
                    case ESpecialSpell::ashe_r:
                    {
                        const auto seconds_traveled =
                            std::min( static_cast< int >( std::floor( current_time - this->server_cast_time ) ), 3 );
                        const auto speed_increase = seconds_traveled < 3 ? 200.f * seconds_traveled : 500.f;

                        const auto extra_distance_traveled = ( time_traveled - seconds_traveled ) * ( this->base_speed +
                            speed_increase );

                        switch ( seconds_traveled ) {
                        case 0:
                            break;
                        case 1:
                            distance_traveled = 1600.f + extra_distance_traveled;
                            break;
                        case 2:
                            distance_traveled = 1600.f + 1800.f + extra_distance_traveled;
                            break;
                        default:
                            distance_traveled = 1600.f + 1800.f + 2000.f + extra_distance_traveled;
                            break;
                        }

                        break;
                    }
                    case ESpecialSpell::jinx_r:
                    {
                        if ( distance_traveled > 1300.f )
                            distance_traveled = 1300.f + ( time_traveled - 0.765f ) *
                                2200.f;
                        break;
                    }
                    default:
                        break;
                    }

                    if ( rt_hash( this->spell_name.c_str() ) == ct_hash( "JavelinToss" ) ||
                        rt_hash( this->spell_name.c_str() ) == ct_hash( "KaisaW" ) ) {
                        distance_traveled -= distance_traveled > 100.f ? 100.f : distance_traveled;
                    }

                    const auto current_pos = this->start_pos.extend( this->end_pos, distance_traveled );

                    return current_pos;
                }

                return this->current_pos;
            }

            auto time_till_impact( const sdk::math::Vec3 position ) const -> float{
                switch ( this->type ) {
                case ESpellType::line:
                {
                    if ( this->speed > 0.f ) {
                        const auto current_position = get_current_position( );
                        const auto end_position     = this->end_pos;


                        if ( const auto projection = position.project_on( current_position, end_position ); projection.
                            is_on_segment ) {
                            const auto distance_reduction = std::max(
                                this->radius + 5.f + ( this->has_edge_radius
                                                     ? g_features->orbwalker->get_bounding_radius( )
                                                     : 0.f ),
                                0.f
                            );

                            return fmax(
                                0.f,
                                ( projection.line_point.dist_to( current_position ) - distance_reduction ) /
                                this->speed + fmax( this->server_cast_time - *g_time, 0.f )
                            );
                        }

                        return 0.f;
                    }

                    return this->end_time - *g_time;
                }
                case ESpellType::circle:
                case ESpellType::cone:
                    return this->end_time - *g_time;
                default:
                    return 0.f;
                }
            }

            auto get_dynamic_line_endpos(
                const float extra_time      = 0.f,
                const float additional_time = 0.f
            ) const -> sdk::math::Vec3{
                if ( this->speed == 0.f ) return this->end_pos;

                const auto evade_time = this->radius * 2.f / g_local->movement_speed + 0.75f + additional_time;

                if ( this->server_cast_time > *g_time ) {
                    if ( this->server_cast_time - *g_time >= evade_time ) return this->start_pos;

                    const auto travel_time     = evade_time + extra_time - ( this->server_cast_time - *g_time );
                    const auto extend_distance = this->speed * travel_time;

                    const auto end_position = extend_distance > this->start_pos.dist_to( this->end_pos ) ||
                                              extend_distance < 0.f
                                                  ? this->end_pos
                                                  : this->current_pos.extend( this->end_pos, extend_distance );

                    if ( !this->collision ) return end_position;

                    const auto collision = g_features->prediction->get_skillshot_collision_position(
                        this->current_pos,
                        end_position,
                        this->radius,
                        this->has_edge_radius,
                        std::max( this->server_cast_time - *g_time, 0.f ),
                        this->speed
                    );

                    return collision.has_value( ) ? collision.value( ) : end_position;
                }

                const auto extend_distance = this->speed * evade_time;

                if ( rt_hash( this->spell_name.c_str() ) == ct_hash( "JavelinToss" )
                    //|| rt_hash( this->spell_name.c_str() ) == ct_hash( "KaisaW" )
                ) {
                    const auto distance_traveled = ( *g_time - this->server_cast_time + extra_time ) * this->speed;

                    return extend_distance > this->start_pos.extend( this->end_pos, distance_traveled ).dist_to(
                               this->end_pos
                           )
                               ? this->end_pos
                               : this->start_pos.extend( this->end_pos, distance_traveled ).extend(
                                   this->end_pos,
                                   extend_distance
                               );
                }

                const auto end_position = extend_distance > this->current_pos.dist_to( this->end_pos )
                                              ? this->end_pos
                                              : this->current_pos.extend( this->end_pos, extend_distance );

                if ( !this->collision ) return end_position;

                const auto collision = g_features->prediction->get_skillshot_collision_position(
                    this->current_pos,
                    end_position,
                    this->radius,
                    this->has_edge_radius,
                    0.f,
                    this->speed
                );

                return collision.has_value( ) ? collision.value( ) : end_position;
            }

            auto should_dodge( ) const -> bool{
                const auto health_percent = g_local->health / g_local->max_health;

                return allow_dodge && health_percent <= ignore_health_threshold / 100.f;
            }

            auto is_dangerous( ) const -> bool{
                return this->danger >= g_config->evade.humanizer_dangerous_threshold->get< int >( ) ||
                    g_config->evade.humanizer_crowdcontrol_dangerous->get< bool >( ) && this->cc;
            }

            bool manual_end_time{ };
        };

        struct VeigarCageInstance {
            sdk::math::Vec3 position{ };

            float start_time{ };
            float end_time{ };

            auto is_active( ) const -> bool{ return this->end_time > *g_time; }
        };

    public:
        SpellDetector( ){
            initialize_spell_instances( );
            initialize_object_instances( );
        }

        auto run( ) -> void override;
        auto on_draw( ) -> void override;

        auto get_name( ) noexcept -> hash_t override{ return names::spell_detector; }
#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_spell_detector"; }
#endif

        auto is_spell_active(
            int32_t                index,
            float                  server_cast_time,
            const sdk::math::Vec3& raw_end_pos = { },
            bool                   is_particle = false
        ) -> bool;
        auto is_missile_active( unsigned nid ) -> bool;
        auto is_object_active( unsigned nid ) const -> bool;

        // optimization
        auto  remove_old_ignored_missiles( ) -> void;
        auto  remove_old_ignored_spells( ) -> void;
        auto  remove_old_ignored_objects( ) -> void;
        float m_last_removal_time{ };

        auto is_missile_from_spell( unsigned nid ) -> bool;

        auto get_active_spells( ) -> std::vector< SpellInstance >{
            if ( g_threading->is_render_thread( ) ) {
                std::unique_lock lock( m_render_mutex );
                return m_active_spells_render;
            }

            return m_active_spells;
        }

        auto get_active_missiles( ) -> std::vector< MissileInstance >{
            if ( g_threading->is_render_thread( ) ) {
                std::unique_lock lock( m_render_mutex );
                return m_active_missiles_render;
            }
            return m_active_missiles;
        }

        auto get_active_objects( ) -> std::vector< ObjectInstance >{
            if ( g_threading->is_render_thread( ) ) {
                std::unique_lock lock( m_render_mutex );
                return m_active_objects_render;
            }
            return m_active_objects;
        }

        auto get_veigar_cage( ) -> std::optional< VeigarCageInstance >{
            if ( !m_veigar_cage.is_active( ) ) return std::nullopt;

            return std::make_optional( m_veigar_cage );
        }

    private:
        auto enable_linear_debug_spell( ) -> void;
        auto enable_circle_debug_spell( ) -> void;

        auto update_spells( ) -> void;

        auto is_spell_ignored(
            int16_t                index,
            float                  server_cast_time,
            const sdk::math::Vec3& raw_end_pos
        ) const -> bool;
        auto is_ignored_missile( unsigned nid ) -> bool;
        auto is_object_ignored( unsigned network_id ) const -> bool;
        auto is_ignored_particle( unsigned network_id ) const -> bool;

        auto remove_spell( int32_t source_index, const sdk::math::Vec3& raw_end_pos ) -> void;
        auto remove_missile( unsigned nid ) -> void;
        auto remove_object( unsigned nid ) -> void;

        auto get_data( hash_t spell_name ) -> SpellInstance;
        auto get_object_data( hash_t object_name ) -> ObjectInstance;

        auto initialize_spell_instances( ) -> void;
        auto initialize_object_instances( ) -> void;

        // spell data modification
        static auto get_dynamic_danger_level( const std::string& spell_name ) -> std::optional< int >;
        static auto get_manual_endtime( const std::string& spell_name ) -> std::optional< float >;
        static auto get_manual_cast_time( const std::string& spell_name ) -> std::optional< float >;
        static auto should_fix_range( const std::string& spell_name ) -> bool;
        static auto has_edge_range( hash_t spell_name ) -> bool;

        // particle spell detection
        auto get_particle_spell( std::string particle_name, sdk::math::Vec3 particle_position ) -> SpellInstance;

        // dash spell
        auto get_dash_spell( int16_t index ) -> SpellInstance;

        // evade config shit
        static auto get_spell_config_value( hash_t name_hash ) -> SpellConfigValues;

        // spell simulator
        auto  simulate_spells( ) -> void;
        float m_last_simulation_time{ };

        std::mutex                     m_render_mutex;
        std::vector< SpellInstance >   m_active_spells{ };
        std::vector< MissileInstance > m_active_missiles{ };
        std::vector< SpellInstance >   m_active_spells_render{ };
        std::vector< MissileInstance > m_active_missiles_render{ };

        std::vector< ObjectInstance > m_active_objects{ };
        std::vector< ObjectInstance > m_active_objects_render{ };


        std::unordered_map< hash_t, SpellInstance >  m_spell_instance;
        std::unordered_map< hash_t, ObjectInstance > m_object_instance;


        std::vector< IgnoredMissile > m_ignored_missiles{ };
        std::vector< IgnoredObject >  m_ignored_objects{ };
        std::vector< IgnoredSpell >   m_ignored_spells{ };
        std::vector< unsigned >       m_ignored_particles{ };

        VeigarCageInstance m_veigar_cage{ };

        float m_last_debug_time{ };
        float m_last_debug_time2{ };

        bool is_debug{ false };


        bool m_detect_ally_spells{ false };
        bool m_detect_ally_missiles{ false };
        bool m_detect_ally_particles{ false };


        float m_malphite_r_time{ };
        bool  m_found_malphite{ };
        bool  m_found_particle_champ{ };

        float m_first_run_time{ };
    };
}
