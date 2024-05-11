#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class aatrox_module final : public IModule {
    public:
        virtual ~aatrox_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "aatrox_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Aatrox" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "aatrox" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->aatrox.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->aatrox.q_harass );
            q_settings->checkbox( _( "simulate smaller hitbox (?)" ), g_config->aatrox.q_accurate_hitbox )->set_tooltip(
                _( "Will use smaller Q hitboxes when predicting enemy, arguably higher hitchance" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->aatrox.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable (?)" ), g_config->aatrox.e_enabled )->set_tooltip(
                "Will be used to adjust Q position"
            );
            //w_settings->checkbox(_("enable"), g_config->aatrox.w_enabled);
            //w_settings->select(_("hitchance"), g_config->aatrox.w_hitchance, { _("Low"), _("Medium"), _("High"), _("Very high"), _("Immobile") });

            drawings->checkbox( _( "draw q range" ), g_config->aatrox.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->aatrox.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->aatrox.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->aatrox.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->aatrox.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->aatrox.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->aatrox.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->aatrox.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->aatrox.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->aatrox.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( m_cast_detected ) {
                const auto polygon = get_hitbox( m_cast_hitbox, m_cast_direction );

                //g_render->circle_3d(get_focus_point(m_cast_hitbox, m_cast_direction), color(100, 255, 125, 255), 20.f, 2, -1, 2.f);

                for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                    const auto start_point = polygon.points[ i ];
                    const auto end_point   = i == polygon.points.size( ) - 1
                                                 ? polygon.points[ 0 ]
                                                 : polygon.points[ i + 1 ];

                    g_render->line_3d( start_point, end_point, Color( 255, 255, 255 ), 2.f );
                }
            }

            return;

            auto cursor          = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto cursor_extended = g_local->position.extend( cursor, 800.f );

            auto sweetspot_start = g_local->position.extend( cursor_extended, 190.f );

            auto rect    = sdk::math::Circle( sweetspot_start, 150.f );
            auto polygon = rect.to_polygon( );

            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                g_render->line_3d( start_point, end_point, Color( 255, 255, 255 ), 2.f );
            }

            if ( g_config->aatrox.q_accurate_hitbox->get< bool >( ) ) {
                //sweetspot_start = g_local->position.extend(cursor_extended, 500.f);
                //rect = sdk::math::rectangle(sweetspot_start, sweetspot_start.extend(cursor_extended, 110.f), 75.f);

                polygon = rect.to_polygon( -60 );

                for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                    const auto start_point = polygon.points[ i ];
                    const auto end_point   = i == polygon.points.size( ) - 1
                                                 ? polygon.points[ 0 ]
                                                 : polygon.points[ i + 1 ];

                    g_render->line_3d( start_point, end_point, Color( 50, 255, 50 ), 2.f );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            cast_tracker( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->aatrox.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->aatrox.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.5f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( rt_hash( m_slot_q->get_name().c_str() ) == ct_hash( "AatroxQ" ) ) {
                const auto pred = g_features->prediction->predict( target->index, 800.f, 0.f, 0.f, 0.6f );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->aatrox.q_hitchance->get<
                    int >( ) )
                    return false;

                auto extended_pred   = g_local->position.extend( pred.position, 800.f );
                auto sweetspot_start = g_local->position.extend( pred.position, 475.f );
                auto hitbox_modifier = g_config->aatrox.q_accurate_hitbox->get< bool >( ) ? -30 : 0;


                auto rect = sdk::math::Rectangle(
                    sweetspot_start,
                    sweetspot_start.extend( extended_pred, 160.f ),
                    105.f
                );
                auto polygon = rect.to_polygon( hitbox_modifier );

                if ( !polygon.is_inside( pred.position ) ) return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->on_cast( );

                    m_cast_active       = true;
                    m_cast_direction    = pred.position;
                    m_cast_hitbox       = 1;
                    m_cast_target_index = target->index;
                    return true;
                }
            } else if ( rt_hash( m_slot_q->get_name().c_str() ) == ct_hash( "AatroxQ2" ) ) {
                const auto pred = g_features->prediction->predict( target->index, 800.f, 0.f, 0.f, 0.6f );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->aatrox.q_hitchance->get<
                    int >( ) )
                    return false;

                auto extended_pred   = g_local->position.extend( pred.position, 800.f );
                auto sweetspot_start = g_local->position.extend( pred.position, 350.f );
                auto hitbox_modifier = g_config->aatrox.q_accurate_hitbox->get< bool >( ) ? -30 : 0;

                auto rect = sdk::math::Rectangle(
                    sweetspot_start,
                    sweetspot_start.extend( extended_pred, 150.f ),
                    275.f
                );
                auto polygon = rect.to_polygon( hitbox_modifier );

                if ( !polygon.is_inside( pred.position ) ) return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->on_cast( );

                    m_cast_active       = true;
                    m_cast_direction    = pred.position;
                    m_cast_hitbox       = 2;
                    m_cast_target_index = target->index;
                    return true;
                }
            } else if ( rt_hash( m_slot_q->get_name().c_str() ) == ct_hash( "AatroxQ3" ) ) {
                const auto pred = g_features->prediction->predict( target->index, 500.f, 0.f, 0.f, 0.6f );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->aatrox.q_hitchance->get<
                    int >( ) )
                    return false;

                auto sweetspot_start = g_local->position.extend( pred.position, 190.f );
                auto hitbox_modifier = g_config->aatrox.q_accurate_hitbox->get< bool >( ) ? -30 : 0;

                auto circle  = sdk::math::Circle( sweetspot_start, 150.f );
                auto polygon = circle.to_polygon( hitbox_modifier );

                if ( !polygon.is_inside( pred.position ) ) return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->on_cast( );

                    m_cast_active       = true;
                    m_cast_direction    = pred.position;
                    m_cast_hitbox       = 3;
                    m_cast_target_index = target->index;
                    return true;
                }
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{ return false; }

        auto spell_r( ) -> bool override{ return false; }

        auto adjust_e( const Vec3& position ) -> void{
            if ( !g_config->aatrox.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( ) )
                return;

            if ( cast_spell( ESpellSlot::e, position ) ) m_last_e_time = *g_time;
        }

        auto cast_tracker( ) -> void{
            if ( !m_cast_active ) return;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 0 ) {
                if ( m_cast_detected ) reset_cast( );
                return;
            }

            m_cast_detected = true;

            if ( !m_fixed_direction ) {
                m_cast_direction  = ( m_cast_direction - g_local->position ).normalize( );
                m_fixed_direction = true;
            }

            const auto& target = g_entity_list.get_by_index( m_cast_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                reset_cast( );
                return;
            }

            const auto duration_left = sci->server_cast_time - *g_time;
            auto       hitbox        = get_hitbox( m_cast_hitbox, m_cast_direction );

            const auto pred = g_features->prediction->predict_default( target->index, duration_left );
            if ( !pred ) return;

            if ( hitbox.is_inside( *pred ) ) return;

            const auto focus_point   = get_focus_point( m_cast_hitbox, m_cast_direction );
            const auto distance      = focus_point.dist_to( *pred );
            const auto direction     = ( *pred - focus_point ).normalize( );
            const auto cast_position = g_local->position.extend( g_local->position + direction, distance );

            const auto dash_time = g_local->position.dist_to( cast_position ) / 805.f;

            if ( dash_time > duration_left + 0.075f ) return;

            adjust_e( cast_position );
        }

        static auto get_hitbox( const int index, const Vec3& direction ) -> sdk::math::Polygon{
            const auto dir = g_local->position + direction;

            switch ( index ) {
            case 1:
            {
                const auto extended_pred   = g_local->position.extend( dir, 800.f );
                const auto sweetspot_start = g_local->position.extend( dir, 475.f );


                const auto rect = sdk::math::Rectangle(
                    sweetspot_start,
                    sweetspot_start.extend( extended_pred, 160.f ),
                    105.f
                );
                return rect.to_polygon( );
            }
            case 2:
            {
                const auto extended_pred   = g_local->position.extend( dir, 800.f );
                const auto sweetspot_start = g_local->position.extend( dir, 350.f );

                const auto rect = sdk::math::Rectangle(
                    sweetspot_start,
                    sweetspot_start.extend( extended_pred, 150.f ),
                    275.f
                );
                return rect.to_polygon( );
            }
            case 3:
            {
                const auto sweetspot_start = g_local->position.extend( dir, 190.f );

                const auto circle = sdk::math::Circle( sweetspot_start, 150.f );
                return circle.to_polygon( );
            }
            default:
                break;
            }

            return { };
        }

        static auto get_focus_point( const int index, const Vec3& direction ) -> Vec3{
            const auto dir = g_local->position + direction;

            switch ( index ) {
            case 1:
            {
                const auto extended_pred   = g_local->position.extend( dir, 800.f );
                const auto sweetspot_start = g_local->position.extend( dir, 475.f );
                const auto focus_point     = sweetspot_start.extend( extended_pred, 80.f );

                return focus_point;
            }
            case 2:
            {
                const auto extended_pred   = g_local->position.extend( dir, 800.f );
                const auto sweetspot_start = g_local->position.extend( dir, 350.f );
                const auto focus_point     = sweetspot_start.extend( extended_pred, 75.f );

                return focus_point;
            }
            case 3:
            {
                const auto sweetspot_start = g_local->position.extend( dir, 190.f );

                return sweetspot_start;
            }
            default:
                break;
            }

            return Vec3( );
        }

        auto reset_cast( ) -> void{
            m_cast_active       = false;
            m_cast_detected     = false;
            m_fixed_direction   = false;
            m_cast_direction    = Vec3( );
            m_cast_hitbox       = 0;
            m_cast_target_index = 0;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1200.f;
            }
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        // q cast tracking
        bool    m_cast_active{ };
        bool    m_cast_detected{ };
        bool    m_fixed_direction{ };
        Vec3    m_cast_direction{ };
        int     m_cast_hitbox{ };
        int16_t m_cast_target_index{ };

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 150.f, 225.f, 300.f };

        float m_q_range{ 625.f };
        float m_w_range{ 765.f };
        float m_e_range{ 300.f };
        float m_r_range{ 0.f };
    };
}
