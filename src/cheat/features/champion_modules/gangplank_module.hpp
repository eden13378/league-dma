#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    struct barrel_instance_t {
        int16_t  index{ };
        unsigned network_id{ };
        Vec3     position{ };

        bool is_object_created{ true };

        float start_time{ };
        float primed_time{ };
        float end_time{ };

        bool is_primed{ };
        bool fixed_prime_time{ };

        bool  will_detonate{ };
        float detonation_time{ };

        bool is_chain_reaction{ };
        Vec3 signal_origin{ };

        bool calculated_detonation{ };
    };

    struct chain_instance_t {
        barrel_instance_t                start_barrel{ };
        barrel_instance_t                end_barrel{ };
        std::vector< barrel_instance_t > barrel_chain{ };

        int  count{ };
        bool valid{ };
    };

    struct direct_chain_instance_t {
        barrel_instance_t root{ };
        barrel_instance_t end_barrel{ };

        bool valid{ };
    };

    class gangplank_module final : public IModule {
    public:
        virtual ~gangplank_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "gangplank_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Gangplank" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "gangplank" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto humanizer  = navigation->add_section( _( "barrel humanizer" ) );

            q_settings->checkbox( _( "enable" ), g_config->gangplank.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->gangplank.q_harass );
            q_settings->checkbox( _( "lasthit q" ), g_config->gangplank.q_lasthit );

            e_settings->checkbox( _( "enable" ), g_config->gangplank.e_enabled );
            e_settings->select(
                _( "barrel chain logic" ),
                g_config->gangplank.e_chain_barrel_logic,
                { _( "On target" ), _( "Max range" ) }
            );
            e_settings->select(
                _( "hitchance" ),
                g_config->gangplank.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            e_settings->slider_int(
                _( "prediction hitbox compensation" ),
                g_config->gangplank.e_hitbox_compensation_value,
                0,
                300,
                1
            );
            e_settings->slider_int(
                _( "simulated barrel radius (?)" ),
                g_config->gangplank.e_simulated_radius,
                0,
                365,
                1
            )->set_tooltip( _( "Smaller = more accurate but can sometimes delay casting E more than it should" ) );

            humanizer->checkbox( _( "humanize barrel placement" ), g_config->gangplank.humanize_barrel_placement );
            humanizer->slider_int( _( "humanizer strength" ), g_config->gangplank.humanizer_strength, 10, 100, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->gangplank.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->gangplank.e_draw_range );
            drawings->checkbox( _( "draw simulated barrel" ), g_config->gangplank.draw_simulated_barrel );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->gangplank.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->gangplank.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->gangplank.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( g_config->gangplank.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->gangplank.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( m_magnet_active ) {
                for ( auto point : m_magnet_points ) {
                    g_render->circle_3d(
                        point,
                        Color( 255, 255, 25, 75 ),
                        15.f,
                        Renderer::outline | Renderer::filled,
                        16,
                        1.f
                    );
                }
            }

            if ( m_phantom_detonation_time > *g_time ) {
                //auto modifier = std::max( 1.f - ( m_phantom_detonation_time - *g_time ) / m_phantom_detonation_duration, 0.f);

                g_render->circle_3d(
                    m_phantom_barrel_point,
                    g_features->orbwalker->get_pulsing_color( ).alpha( 45 ),
                    m_barrel_connect_radius,
                    Renderer::outline | Renderer::filled,
                    72,
                    3.f
                );
            }

            Vec2 sp{ };

            for ( auto barrel : m_barrels ) {
                /* if ( barrel.will_detonate && barrel.is_chain_reaction ) {

                    if ( barrel.detonation_time - m_chain_detonation_delay > *g_time) {
                        g_render->line_3d( barrel.position, barrel.signal_origin, color( 255, 255, 255 ), 3.f );
                    } else {

                        auto modifier = 1.f - std::min(( barrel.detonation_time - *g_time ) / m_chain_detonation_delay, 1.f);
                        if ( modifier > 1.f ) modifier = 1.f;

                        g_render->line_3d( barrel.position, 
                            barrel.signal_origin.extend( barrel.position, barrel.position.dist_to( barrel.signal_origin ) * modifier ),
                            color( 255, 50, 25 ), 3.f );
                    }
                }*/


                if ( barrel.will_detonate ) {
                    g_render->circle_3d(
                        barrel.position,
                        barrel.will_detonate
                            ? g_features->orbwalker->get_pulsing_color( ).alpha( 200 )
                            : Color( 50, 255, 25, 155 ),
                        m_barrel_connect_radius,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }

                if ( !world_to_screen( barrel.position, sp ) ) continue;

                auto text = std::to_string( barrel.end_time - *g_time );
                text.resize( 4 );

                auto text_size     = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );
                Vec2 text_position = { sp.x - text_size.x / 2.f, sp.y += text_size.y };

                g_render->text_shadow( text_position, Color::white( ), g_fonts->get_zabel_16px( ), text.data( ), 16 );

                sp.y += text_size.y + 2.f;

                auto bar_width  = 40.f;
                auto bar_height = 5.f;

                Vec2 loading_start = { sp.x - bar_width / 2.f, sp.y };
                Vec2 loading_size  = { bar_width, bar_height };

                Vec2 border_start = { loading_start.x - 1.f, loading_start.y - 1.f };
                Vec2 border_size  = { loading_size.x + 2.f, loading_size.y + 2.f };

                g_render->filled_box( loading_start, loading_size, Color( 10, 10, 10, 200 ), -1 );
                g_render->box( border_start, border_size, Color( 1, 1, 1, 255 ), -1, 1.f );

                auto modifier = std::min(
                    ( *g_time - barrel.start_time ) / ( barrel.primed_time - barrel.start_time ),
                    1.f
                );
                if ( modifier > 1.f ) modifier = 1.f;

                if ( barrel.will_detonate && modifier < 1.f ) modifier = 1.f;

                auto bar_color = barrel.will_detonate
                                     ? g_features->orbwalker->get_pulsing_color( )
                                     : barrel.is_primed
                                           ? Color( 25, 180, 25 )
                                           : Color( 255, 255, 25 );

                Vec2 bar_size = { bar_width * modifier, bar_height };

                g_render->filled_box( loading_start, bar_size, bar_color, -1 );
            }

            if ( g_config->gangplank.draw_simulated_barrel->get< bool >( ) ) {
                auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return;

                auto barrel = get_closest_barrel( target->position );
                if ( !barrel ) return;

                auto pred = g_features->prediction->predict(
                    target->index,
                    1365.f,
                    0.f,
                    0.f,
                    0.5f,
                    { },
                    false,
                    Prediction::render_thread
                );
                if ( !pred.valid ) return;

                auto sim = get_chain_point( barrel->position, pred.position, true, false );
                if ( !sim ) return;

                if ( sim.value( ).dist_to( pred.position ) <= m_barrel_detonation_radius || m_slot_e->charges <= 1 ) {
                    sim = get_chain_point( barrel->position, pred.position );
                    if ( !sim ) return;

                    g_render->circle_3d(
                        *sim,
                        Color( 10, 85, 250, 155 ),
                        m_barrel_connect_radius,
                        Renderer::outline,
                        64,
                        4.f
                    );

                    return;
                }

                g_render->circle_3d(
                    *sim,
                    Color( 10, 85, 250, 155 ),
                    m_barrel_connect_radius,
                    Renderer::outline,
                    64,
                    4.f
                );

                sim = get_chain_point( *sim, pred.position );
                if ( !sim ) return;

                g_render->circle_3d(
                    *sim,
                    Color( 10, 85, 250, 100 ),
                    m_barrel_connect_radius,
                    Renderer::outline,
                    64,
                    4.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            cast_tracking( );
            update_barrels( );
            phantom_barrel_logic( );

            update_humanizer( );

            m_barrel_hitbox_compensation =
                static_cast< float >( g_config->gangplank.e_hitbox_compensation_value->get< int >( ) );

            m_barrel_detonation_radius =
                static_cast< float >( g_config->gangplank.e_simulated_radius->get< int >( ) );

            if ( m_spells_disabled && *g_time > m_disable_expire_time ) m_spells_disabled = false;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            semi_manual_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                autoattack_logic( );
                spell_q( );
                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                harass_q( );
                lasthit_q( );
            case Orbwalker::EOrbwalkerMode::lasthit:
            case Orbwalker::EOrbwalkerMode::laneclear:
                lasthit_q( );
                break;
            default:
                break;
            }

            //std::cout << "charges: " << m_slot_e->charges << " | ready: " << m_slot_e->is_ready( ) << std::endl;
            //std::cout << "addy# " << std::hex << m_slot_e.get_address( ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->gangplank.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || m_e_active || !m_slot_q->is_ready( true ) || m_spells_disabled )
                return false;

            if ( multibarrel_q( ) || phantom_barrel_q( ) || direct_q_barrel( ) || direct_q_barrel_chain( ) ) {
                return
                    true;
            }

            if ( direct_q( ) ) return true;

            return false;
        }

        auto harass_q( ) -> bool{
            if ( !g_config->gangplank.q_harass->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) || m_spells_disabled )
                return false;

            if ( phantom_barrel_q( ) || direct_q( ) ) return true;

            return false;
        }

        auto direct_q( ) -> bool{
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return false;

            auto allow_q{ m_slot_e->charges == 0 };

            if ( !allow_q ) {
                const auto closest_barrel = get_closest_barrel( target->position );
                if ( closest_barrel && g_features->orbwalker->is_attackable( closest_barrel->index ) ) allow_q = true;
            }

            if ( !allow_q ) {
                const auto barrels_in_range = get_barrels_in_range( m_q_range + 125.f );
                if ( barrels_in_range.empty( ) ) allow_q = true;
                else {
                    bool found_valid_barrel{ };

                    for ( const auto barrel : barrels_in_range ) {
                        if ( barrel.will_detonate ) continue;


                        found_valid_barrel = true;
                    }

                    allow_q = !found_valid_barrel;
                }
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }
            return false;
        }

        auto phantom_barrel_q( ) -> bool{
            if ( m_slot_e->charges == 0 ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrels = get_barrels_in_range( m_q_range );
            unsigned   target_network_id{ };

            const auto local_pred     = g_features->prediction->predict_default( g_local->index, 0.f, true );
            const auto local_position = local_pred.has_value( ) ? local_pred.value( ) : g_local->position;

            for ( auto barrel : barrels ) {
                const auto travel_time = 0.25f + g_local->position.dist_to( barrel.position ) / 2400.f;
                if ( barrel.primed_time > *g_time + travel_time || local_position.dist_to( barrel.position ) <=
                    600.f )
                    continue;

                const auto pred = g_features->prediction->predict( target->index, 1200.f, 0.f, 0.f, 0.8f );
                if ( !pred.valid ) continue;

                const auto phantom_point = get_chain_point( barrel.position, pred.position, true, false );
                if ( !phantom_point || phantom_point->dist_to( pred.position ) > 300.f ) return false;

                target_network_id = barrel.network_id;
                break;
            }

            if ( target_network_id == 0 ) return false;

            if ( cast_spell( ESpellSlot::q, target_network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->disable_for( 0.2f );
                disable_magnet( );

                return true;
            }

            return false;
        }

        auto autoattack_logic( ) -> bool{
            if ( m_spells_disabled || *g_time - m_last_aa_time <= 0.75f ) return false;

            if ( multibarrel_autoattack( ) || autoattack_direct_barrel( ) || autoattack_direct_barrel_chain( ) ) {
                return
                    true;
            }

            return false;
        }

        auto q_barrel_logic( ) -> bool{
            if ( !g_config->gangplank.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) || m_spells_disabled )
                return false;

            if ( direct_q_barrel( ) ) {
                std::cout << "[ buffer ] direct q barrel logic\n";
                return true;
            }

            if ( direct_q_barrel_chain( ) ) {
                std::cout << "[ buffer ] direct q barrel chain logic\n";
                return true;
            }

            return false;
        }

        auto direct_q_barrel( ) -> bool{
            if ( m_spells_disabled ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrels = get_barrels_in_range( m_q_range );

            unsigned   target_network_id{ };
            const auto extra_time = std::max( m_e_server_cast_time - *g_time, 0.f );

            for ( auto barrel : barrels ) {
                const auto travel_time = 0.25f + g_local->position.dist_to( barrel.position ) / 2400.f + extra_time;
                if ( barrel.primed_time > *g_time + travel_time ) continue;

                auto pred = g_features->prediction->predict(
                    target->index,
                    m_barrel_targeting_radius,
                    0.f,
                    350.f,
                    travel_time,
                    barrel.position
                );
                if ( !pred.valid || ( int )pred.hitchance < g_config->gangplank.e_hitchance->get< int >( ) ||
                    pred.default_position.dist_to( barrel.position ) > 300.f )
                    continue;

                target_network_id = barrel.network_id;
                break;
            }

            if ( target_network_id == 0 ) return false;

            if ( cast_spell( ESpellSlot::q, target_network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                disable_spells( 0.15f );
                return true;
            }

            return false;
        }

        auto direct_q_barrel_chain( ) -> bool{
            if ( m_spells_disabled ) return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto barrels = get_barrels_in_range( m_q_range );

            unsigned target_network_id{ };
            auto     extra_time = std::max( m_e_server_cast_time - *g_time, 0.f );

            for ( auto barrel : barrels ) {
                auto travel_time = 0.25f + g_local->position.dist_to( barrel.position ) / 2400.f + extra_time;
                if ( barrel.primed_time > *g_time + travel_time ) continue;

                auto chain = get_shortest_chain( barrel, target->position );
                if ( !chain.valid ) continue;

                auto pred = g_features->prediction->predict(
                    target->index,
                    m_barrel_targeting_radius,
                    0.f,
                    350.f,
                    travel_time + 0.3f * chain.count,
                    chain.end_barrel.position
                );
                if ( !pred.valid || !m_e_active && ( int )pred.hitchance < g_config->gangplank.e_hitchance->get<
                        int >( ) ||
                    pred.position.dist_to( chain.end_barrel.position ) > m_barrel_detonation_radius )
                    continue;

                target_network_id = barrel.network_id;
                break;
            }

            if ( target_network_id == 0 ) return false;

            if ( cast_spell( ESpellSlot::q, target_network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                disable_spells( 0.15f );
                return true;
            }

            return false;
        }

        auto multibarrel_q( ) -> bool{
            if ( m_spells_disabled || m_slot_e->charges == 0 || !m_slot_e->is_ready( ) ) return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto barrels = get_barrels_in_range( m_q_range );

            unsigned target_network_id{ };

            for ( auto barrel : barrels ) {
                auto travel_time = 0.25f + g_local->position.dist_to( barrel.position ) / 2400.f;
                if ( barrel.primed_time > *g_time + travel_time ) continue;

                auto chain = get_shortest_chain( barrel, target->position );
                if ( !chain.valid || chain.count == 0 && travel_time <= 0.5f ) continue;

                auto pred = g_features->prediction->predict(
                    target->index,
                    m_e_range + m_barrel_detonation_radius,
                    0.f,
                    m_barrel_hitbox_compensation,
                    travel_time + 0.3f + 0.3f * chain.count,
                    chain.end_barrel.position
                );

                if ( !pred.valid || ( int )pred.hitchance < g_config->gangplank.e_hitchance->get< int >( ) ) continue;

                auto simulated_barrel = get_chain_point( chain.end_barrel.position, pred.position, true );
                if ( !simulated_barrel || pred.default_position.dist_to( *simulated_barrel ) >
                    m_barrel_detonation_radius )
                    continue;

                target_network_id = barrel.network_id;
                break;
            }

            if ( target_network_id == 0 ) return false;

            if ( cast_spell( ESpellSlot::q, target_network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ gangplan ] multibarrel q\n";
                return true;
            }

            return false;
        }

        auto autoattack_direct_barrel( ) -> bool{
            if ( g_features->orbwalker->get_next_aa_time( ) > *g_time + g_features->orbwalker->get_attack_cast_delay( )
                + ( m_e_active ? m_e_server_cast_time - *g_time : 0.f ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrels = get_barrels_in_range( );

            unsigned target_network_id{ };
            int16_t  target_index{ };

            const auto extra_time = std::max( m_e_server_cast_time - *g_time, 0.f );

            for ( auto barrel : barrels ) {
                const auto travel_time = g_features->orbwalker->get_next_aa_time( ) + extra_time + g_features->orbwalker
                    ->
                    get_ping( );
                if ( barrel.primed_time > travel_time ) continue;

                auto pred = g_features->prediction->predict(
                    target->index,
                    10000.f,
                    0.f,
                    350.f,
                    travel_time - *g_time
                );
                if ( !pred.valid ||
                    !m_e_active && ( int )pred.hitchance < g_config->gangplank.e_hitchance->get< int >( ) ||
                    pred.default_position.dist_to( barrel.position ) > m_barrel_detonation_radius )
                    continue;

                target_network_id = barrel.network_id;
                target_index      = barrel.index;
                break;
            }

            if ( target_network_id == 0 || target_index == 0 ) return false;

            g_features->orbwalker->override_target( target_index );
            m_last_aa_time   = *g_time;
            m_last_cast_time = *g_time;
            disable_spells(
                g_features->orbwalker->get_next_aa_time( ) - *g_time + extra_time +
                g_features->orbwalker->get_ping( ) * 2.f
            );

            if ( m_e_active ) g_features->orbwalker->ignore_spell_during_attack( m_e_server_cast_time );
            else if ( m_q_active ) g_features->orbwalker->ignore_spell_during_attack( m_q_server_cast_time );

            std::cout << "[ gangplank ] direct barrel autoattack | " << *g_time << std::endl;

            return true;
        }

        auto autoattack_direct_barrel_chain( ) -> bool{
            if ( g_features->orbwalker->get_next_aa_time( ) > *g_time + g_features->orbwalker->get_attack_cast_delay( )
                + ( m_e_active ? m_e_server_cast_time - *g_time : 0.f ) )
                return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto barrels = get_barrels_in_range( );

            unsigned target_network_id{ };
            int16_t  target_index{ };

            auto extra_time = std::max( m_e_server_cast_time - *g_time, 0.f );

            for ( auto barrel : barrels ) {
                auto travel_time = g_features->orbwalker->get_next_aa_time( ) + extra_time;
                if ( barrel.will_detonate || !barrel.is_primed && barrel.primed_time > travel_time ) continue;

                auto chain = get_shortest_chain( barrel, target->position );
                if ( !chain.valid ) continue;

                auto pred = g_features->prediction->predict(
                    target->index,
                    10000.f,
                    0.f,
                    350.f,
                    travel_time - *g_time + 0.3f * chain.count,
                    barrel.position
                );
                if ( !pred.valid || !m_e_active && ( int )pred.hitchance < g_config->gangplank.e_hitchance->get<
                        int >( ) ||
                    pred.default_position.dist_to( chain.end_barrel.position ) > m_barrel_detonation_radius )
                    continue;

                target_network_id = barrel.network_id;
                target_index      = barrel.index;
                break;
            }

            if ( target_network_id == 0 || target_index == 0 ) return false;

            g_features->orbwalker->override_target( target_index, true );
            disable_spells(
                g_features->orbwalker->get_next_aa_time( ) - *g_time + extra_time + g_features->orbwalker->get_ping( ) *
                2.f
            );
            m_last_aa_time   = *g_time;
            m_last_cast_time = *g_time;

            if ( m_e_active ) g_features->orbwalker->ignore_spell_during_attack( m_e_server_cast_time );
            else if ( m_q_active ) g_features->orbwalker->ignore_spell_during_attack( m_q_server_cast_time );

            std::cout << "[ gangplank ] normal multibarrel autoattack | " << *g_time << std::endl;

            return true;
        }

        auto multibarrel_autoattack( ) -> bool{
            if ( m_spells_disabled || m_slot_e->charges == 0 || !m_slot_e->is_ready( ) ) return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto barrels = get_barrels_in_range( );

            unsigned target_network_id{ };
            int16_t  target_index{ };

            for ( auto barrel : barrels ) {
                if ( barrel.primed_time > g_features->orbwalker->get_next_aa_time( ) ) continue;

                auto chain = get_shortest_chain( barrel, target->position );
                if ( !chain.valid || chain.count < 1 ) continue;

                auto travel_time = g_features->orbwalker->get_attack_cast_delay( );

                auto pred = g_features->prediction->predict(
                    target->index,
                    m_e_range + m_barrel_detonation_radius,
                    0.f,
                    m_barrel_hitbox_compensation,
                    travel_time + 0.3f + 0.3f * chain.count,
                    chain.end_barrel.position
                );
                if ( !pred.valid || ( int )pred.hitchance < g_config->gangplank.e_hitchance->get< int >( ) ) continue;

                auto simulated_barrel = get_chain_point( chain.end_barrel.position, pred.position, true );
                if ( !simulated_barrel || pred.default_position.dist_to( *simulated_barrel ) >
                    m_barrel_detonation_radius )
                    continue;

                target_network_id = barrel.network_id;
                target_index      = barrel.index;
                break;
            }

            if ( target_network_id == 0 || target_index == 0 ) return false;

            g_features->orbwalker->override_target( target_index, true );

            if ( m_e_active ) g_features->orbwalker->ignore_spell_during_attack( m_e_server_cast_time );
            else if ( m_q_active ) g_features->orbwalker->ignore_spell_during_attack( m_q_server_cast_time );

            m_last_aa_time   = *g_time;
            m_last_cast_time = *g_time;

            std::cout << "[ gangplank ] multibarrel autoattack | " << *g_time << std::endl;

            return false;
        }

        auto post_barrel_q( ) -> bool{
            if ( !m_should_detonate || m_autoattack_detonation
                || *g_time - m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_entity_list.get_by_index( m_detonation_target );
            if ( !target || target->dist_to_local( ) > m_q_range ) return false;

            const auto base_barrel = get_barrel( target->network_id );
            if ( !base_barrel ) return false;

            const auto barrel_target = g_features->target_selector->get_default_target( );
            if ( !barrel_target ) return false;

            const auto pred = g_features->prediction->predict(
                barrel_target->index,
                1200.f,
                0.f,
                0.f,
                0.25f + g_local->position.dist_to( target->position ) / 2600.f + 0.3f
            );
            if ( !pred.valid || pred.position.dist_to( m_last_e_position ) > 365.f ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time       = *g_time;
                m_last_cast_time    = *g_time;
                m_should_detonate   = false;
                m_detonation_target = 0;

                std::cout << "[ E-Q ] Cast\n";
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->gangplank.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) || m_slot_e->charges == 0 ||
                m_spells_disabled )
                return false;

            //simulate_next_barrel( );

            if ( build_barrel_chain( ) || build_autoattack_chain( ) || build_q_chain( ) ) return true;

            return false;
        }

        auto semi_manual_e( ) -> bool{
            if ( !GetAsyncKeyState( 0x33 ) || !m_slot_e->is_ready( ) || m_slot_e->charges <= 0 ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_e_time <= 0.4f )
                return false;

            const auto cursor         = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto closest_barrel = get_closest_barrel( cursor );
            if ( !closest_barrel ) return false;

            const auto barrel_point = get_chain_point( closest_barrel->position, cursor );
            if ( !barrel_point ) return false;

            if ( cast_spell( ESpellSlot::e, barrel_point.value( ) ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->gangplank.q_lasthit->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) )
                return false;

            const auto            lasthit_data = get_targetable_lasthit_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );

            if ( !lasthit_data ) return false;


            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time * 2.f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto construct_melee_chain( ) -> bool{
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                1000.f,
                0.f,
                300.f,
                0.25f + g_features->orbwalker->get_attack_cast_delay( ) + 0.3f
            );
            if ( !pred.valid ) return false;

            const auto target_position = pred.position;

            const Object* barrel{ };
            auto          lowest_distance = FLT_MAX;
            for ( auto inst : get_barrels_in_range( g_local->attack_range + 100.f ) ) {
                if ( inst.position.dist_to( target_position ) > lowest_distance ) continue;

                auto obj = g_entity_list.get_by_index( inst.index );
                if ( !obj || obj->is_dead( ) ) continue;

                barrel          = obj.get( );
                lowest_distance = obj->position.dist_to( target_position );
            }

            if ( !barrel ) return false;

            const auto barrel_instance = get_barrel( barrel->network_id );
            if ( !barrel_instance || barrel_instance->will_detonate ) return false;

            const auto can_hit = barrel_instance->is_primed || barrel_instance->primed_time - *g_time <= g_features->
                orbwalker
                ->get_attack_cast_delay( ) + 0.25f;
            if ( g_features->orbwalker->get_next_possible_aa_time( ) > *g_time + 0.25f || !can_hit ) return false;

            if ( pred.position.dist_to( barrel->position ) <= 300.f ) {
                if ( barrel_instance->primed_time <= *g_time + g_features->orbwalker->get_attack_cast_delay( ) ) {
                    m_should_detonate       = true;
                    m_autoattack_detonation = true;
                    m_detonation_target     = barrel->index;

                    g_features->orbwalker->override_target( barrel->index );
                    m_last_cast_time = *g_time;
                    disable_spells( 0.3f );
                    //std::cout << "[ MELEE BARREL ] Detonating barrel without building more\n";

                    return true;
                }

                return false;
            }

            auto simulated = get_chain_point( barrel->position, target_position, can_hit );
            if ( !simulated ) return false;

            auto cast_position = *simulated;
            if ( cast_position.dist_to( target_position ) > 350.f ) {
                simulated = get_chain_point( barrel->position, target_position, can_hit, true );
                if ( !simulated ) return false;

                cast_position = *simulated;

                if ( cast_position.dist_to( target_position ) > 345.f ) return false;
            }

            if ( is_barrel_nearby( cast_position ) ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                m_should_detonate       = true;
                m_autoattack_detonation = true;
                m_detonation_target     = barrel->index;

                //std::cout << "[ E MELEE ] Constructing melee chain\n";
                disable_spells( 0.25f );

                m_simulated_barrel_point = Vec3( );
            }

            return false;
        }

        auto construct_q_chain( ) -> bool{
            if ( !m_slot_q->is_ready( true ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 300.f, 0.75f );
            if ( !pred.valid ) return false;

            const auto target_position = pred.position;

            const Object* barrel{ };
            auto          lowest_distance = FLT_MAX;
            for ( auto inst : get_barrels_in_range( m_q_range ) ) {
                if ( inst.position.dist_to( target_position ) > lowest_distance ) continue;

                auto obj = g_entity_list.get_by_index( inst.index );
                if ( !obj || obj->is_dead( ) ) continue;

                barrel          = obj.get( );
                lowest_distance = obj->position.dist_to( target_position );
            }

            if ( !barrel || g_features->orbwalker->is_attackable( barrel->index ) ) return false;

            const auto barrel_instance = get_barrel( barrel->network_id );
            if ( !barrel_instance || barrel_instance->will_detonate ) return false;

            const auto can_hit = barrel_instance->is_primed ||
                barrel_instance->primed_time - *g_time <= 0.5f + g_local->position.dist_to( barrel->position ) / 2600.f;
            if ( !can_hit ) return false;

            auto simulated = get_chain_point( barrel->position, target_position, can_hit );
            if ( !simulated || !can_hit && target_position.dist_to( simulated.value( ) ) <= 500.f ) return false;

            auto cast_position = *simulated;
            if ( cast_position.dist_to( target_position ) > 350.f ) {
                simulated = get_chain_point( barrel->position, target_position, can_hit );
                if ( !simulated ) return false;

                cast_position = *simulated;

                if ( cast_position.dist_to( target_position ) > 345.f ) return false;
            }

            if ( is_barrel_nearby( cast_position ) ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                m_should_detonate       = true;
                m_autoattack_detonation = false;
                m_detonation_target     = barrel->index;

                m_last_e_position = cast_position;

                m_simulated_barrel_point = Vec3( );
            }

            return false;
        }

        auto build_barrel_chain( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.75f || m_slot_e->charges <= 1 ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrel = get_closest_barrel( target->position );
            if ( !barrel ) return false;

            const auto barrel_instance = get_barrel( barrel->network_id );
            if ( !barrel_instance || barrel_instance->will_detonate || !barrel_instance->is_object_created ) {
                return
                    false;
            }

            const auto pred = g_features->prediction->predict(
                target->index,
                1600.f,
                0.f,
                0.f,
                0.5f,
                barrel_instance->position
            );
            if ( !pred.valid ) return false;

            const auto simulated = get_chain_point( barrel->position, pred.position );
            if ( !simulated ) return false;

            if ( g_local->position.dist_to( *simulated ) > m_e_range ||
                simulated->dist_to( pred.position ) <= m_barrel_detonation_radius ||
                is_barrel_nearby( *simulated ) )
                return false;

            if ( cast_spell( ESpellSlot::e, simulated.value( ) ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                m_simulated_barrel_point = Vec3( );

                //std::cout << "[ CHAIN E ] Constructing barrel chain toward " << target->name.text
                //          << " | aa trigger: " << can_detonate_aa << " | q trigger: " << can_detonate_q << std::endl;

                return true;
            }

            return false;
        }

        auto build_autoattack_chain( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.75f || g_features->orbwalker->get_next_aa_time( ) > *g_time + 0.25f +
                g_features->orbwalker->get_attack_cast_delay( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrels = get_barrels_in_range( );
            if ( barrels.empty( ) ) return false;

            Vec3 cast_position{ };

            for ( auto barrel : barrels ) {
                if ( !barrel.is_primed && barrel.primed_time > *g_time + 0.25f + g_features->orbwalker->
                    get_attack_cast_delay( ) ||
                    !barrel.is_object_created )
                    continue;

                const auto travel_time = g_features->orbwalker->get_attack_cast_delay( ) + 0.25f + 0.3f;

                auto pred = g_features->prediction->predict( target->index, 10000.f, 0.f, 365.f, travel_time );
                auto future_predict = g_features->prediction->predict(
                    target->index,
                    10000.f,
                    0.f,
                    m_barrel_hitbox_compensation,
                    travel_time
                );
                if ( !pred.valid || !future_predict.valid ||
                    ( int )pred.hitchance < g_config->gangplank.e_hitchance->get< int >( ) ||
                    future_predict.position.dist_to( barrel.position ) >
                    m_barrel_chain_range + m_barrel_detonation_radius )
                    continue;

                auto simulated = get_chain_point( barrel.position, future_predict.position, true );
                if ( !simulated ) continue;

                if ( g_local->position.dist_to( *simulated ) > m_e_range ||
                    simulated->dist_to( pred.default_position ) > m_barrel_detonation_radius ||
                    is_barrel_nearby( *simulated ) )
                    continue;

                cast_position = *simulated;
                break;
            }

            if ( cast_position.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ chain detonatable: autoattack ] " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto build_q_chain( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.75f || !m_slot_q->is_ready( true ) || *g_time - m_last_q_time <=
                0.4f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrels = get_barrels_in_range( m_q_range );
            if ( barrels.empty( ) ) return false;

            Vec3 cast_position{ };

            for ( auto barrel : barrels ) {
                if ( g_features->orbwalker->is_attackable( barrel.index ) || !barrel.is_object_created ) continue;

                const auto q_delay = 0.25f + g_local->position.dist_to( barrel.position ) / 2400.f;
                if ( barrel.primed_time > *g_time + q_delay ) continue;

                const auto travel_time = 0.5f + 0.3f;

                auto pred = g_features->prediction->predict( target->index, 10000.f, 0.f, 365.f, travel_time );
                auto future_predict = g_features->prediction->predict(
                    target->index,
                    10000.f,
                    0.f,
                    m_barrel_hitbox_compensation,
                    travel_time
                );

                if ( !pred.valid || !future_predict.valid || ( int )pred.hitchance < g_config->gangplank.e_hitchance->
                    get< int >( ) ||
                    future_predict.position.dist_to( barrel.position ) >
                    m_barrel_chain_range + m_barrel_detonation_radius )
                    continue;

                auto simulated = get_chain_point( barrel.position, future_predict.position );
                if ( !simulated ) continue;

                if ( g_local->position.dist_to( *simulated ) > m_e_range ||
                    simulated->dist_to( future_predict.position ) > m_barrel_detonation_radius ||
                    is_barrel_nearby( *simulated ) )
                    continue;

                cast_position = *simulated;
                break;
            }

            if ( cast_position.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ chain detonatable: q ] " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto post_detonation_barrel( unsigned root_barrel_network_id, bool guaranteed = false ) -> bool{
            if ( *g_time - m_last_e_time <= 0.5f || m_slot_e->charges == 0 || !m_slot_e->is_ready( ) ) return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto closest_barrel = get_closest_barrel( target->position );
            if ( closest_barrel ) {
                auto barrel_instance = get_barrel( closest_barrel->network_id );
                if ( barrel_instance.has_value( ) ) {
                    auto barrel = *barrel_instance;
                    if ( barrel.will_detonate ) {
                        auto pred = g_features->prediction->predict(
                            target->index,
                            10000.f,
                            0.f,
                            365.f,
                            barrel.detonation_time - *g_time,
                            barrel.position,
                            false,
                            0
                        );
                        if ( pred.valid &&
                            pred.default_position.dist_to( barrel.position ) <= m_barrel_detonation_radius )
                            return false;
                    }
                }
            }

            auto barrels = get_barrels_in_range( m_e_range );

            Vec3 cast_position{ };
            bool allow_cast{ };

            auto possible_root_barrel = get_barrel( root_barrel_network_id );
            if ( !possible_root_barrel ) return false;

            auto root_barrel = *possible_root_barrel;
            if ( !root_barrel.will_detonate ) return false;

            auto chain = get_shortest_chain( root_barrel, target->position );
            if ( !chain.valid || chain.end_barrel.detonation_time <= ( m_aa_active
                                                                           ? m_aa_server_cast_time + 0.25f
                                                                           : m_q_server_cast_time + 0.25f ) ) {
                return
                    false;
            }

            auto travel_time = root_barrel.detonation_time - *g_time + 0.3f + 0.3f * chain.count;
            auto pred        = g_features->prediction->predict(
                target->index,
                m_e_range + m_barrel_detonation_radius,
                0.f,
                365.f,
                travel_time
            );
            auto future_predict = g_features->prediction->predict(
                target->index,
                m_e_range + m_barrel_detonation_radius,
                0.f,
                m_barrel_hitbox_compensation,
                travel_time
            );

            if ( !pred.valid || !future_predict.valid ) return false;

            auto sim = get_chain_point( chain.end_barrel.position, future_predict.position );
            if ( !sim || sim->dist_to( future_predict.position ) > m_barrel_detonation_radius ) return false;

            cast_position = sim.value( );
            allow_cast    = true;


            if ( !allow_cast || cast_position.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                std::cout << "[ post detonation barrel ] Executed logic\n";

                m_phantom_barrel_point        = cast_position;
                m_phantom_detonation_time     = chain.end_barrel.detonation_time + 0.3f;
                m_phantom_detonation_duration = m_phantom_detonation_time - *g_time;

                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto ghost_barrel_logic( ) -> bool{
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto barrel_object = get_closest_barrel( target->position );
            if ( !barrel_object ) return false;

            const auto barrel = get_barrel( barrel_object->network_id );
            if ( !barrel || !barrel->will_detonate ) return false;

            const auto traveltime = barrel->detonation_time - *g_time;
            auto pred = g_features->prediction->predict( target->index, 1365.f, 0.f, 0.f, traveltime, { }, false, 0 );
            if ( !pred.valid || pred.position.dist_to( barrel->position ) < 300.f ) return false;

            pred = g_features->prediction->predict( target->index, 1365.f, 0.f, 0.f, traveltime + 0.3f );
            if ( !pred.valid ) return false;

            const auto simulated = get_chain_point( barrel_object->position, pred.position, true, true );
            if ( !simulated ) return false;

            const auto cast_position = simulated.value( );

            const auto time_to_detonation = barrel->detonation_time - *g_time;
            if ( time_to_detonation > 0.4f + g_features->orbwalker->get_ping( ) ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                std::cout << "[ GHOST E ] Built last second barrel to hit target\n";

                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                m_simulated_barrel_point = Vec3( );

                return true;
            }

            return false;
        }

        auto simulate_next_barrel( ) -> void{
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            const auto barrel = get_closest_barrel( target->position );
            if ( !barrel ) return;

            const auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, 0.5f );
            if ( !pred.valid ) return;

            const auto sim = get_chain_point( barrel->position, pred.position );
            if ( !sim ) {
                m_simulated_barrel_point = Vec3( );
                return;
            }


            m_simulated_barrel_point = *sim;
            m_last_simulation_time   = *g_time;
        }

        auto get_chain_point(
            Vec3       base_point,
            Vec3       target_point,
            bool       detonation_point = false,
            const bool humanize         = true
        ) const
            -> std::optional< Vec3 >{
            const auto chain_range = humanize ? m_humanized_chain_range : m_barrel_chain_range;

            base_point.y   = g_navgrid->get_height( base_point );
            target_point.y = base_point.y;

            auto extended_point{ base_point.extend( target_point, chain_range ) };
            extended_point.y = g_navgrid->get_height( extended_point );

            if ( g_config->gangplank.e_chain_barrel_logic->get< int >( ) == 0 &&
                extended_point.dist_to( base_point ) > base_point.dist_to( target_point ) ) {
                extended_point = target_point;

                if ( target_point.dist_to( base_point ) <= 365.f ) {
                    extended_point = base_point.extend(
                        target_point,
                        365.f
                    );
                } else extended_point = target_point;
            }

            for ( auto i = 1; i <= 2; i++ ) {
                const auto distance = extended_point.dist_to( base_point );
                if ( distance <= chain_range ) break;

                const auto adjust_value = distance - chain_range;
                if ( adjust_value < 5.f ) break;

                auto temp = base_point.extend( target_point, chain_range - adjust_value );
                temp.y    = g_navgrid->get_height( extended_point );

                extended_point = temp;
            }

            //if ( g_navgrid->is_wall( extended_point ))
            //    return std::nullopt;

            if ( g_local->position.dist_to( extended_point ) > 980.f ) {
                Vec3 safe_point{ };
                Vec3 unknown_point{ };

                const auto max_extend_range = 975.f;

                for ( auto i = 1; i <= 15; i++ ) {
                    auto temp = base_point.extend( target_point, base_point.dist_to( extended_point ) / 15.f * i );

                    safe_point    = i == 1 ? base_point : unknown_point;
                    unknown_point = temp;

                    if ( temp.dist_to( g_local->position ) > max_extend_range ) break;
                }

                auto cast_point{ safe_point };

                for ( auto i = 1; i <= 15; i++ ) {
                    auto temp = safe_point.extend( unknown_point, safe_point.dist_to( unknown_point ) / 15.f * i );
                    if ( temp.dist_to( g_local->position ) >= 980.f ) break;


                    cast_point = temp;
                }

                extended_point = cast_point;
            }

            if ( extended_point.dist_to( g_local->position ) > 990.f ) return std::nullopt;

            return std::make_optional( extended_point );
        }

        auto get_shortest_chain( barrel_instance_t root_barrel, Vec3 target_position ) const -> chain_instance_t{
            barrel_instance_t closest_barrel{ };
            auto              lowest_distance{ FLT_MAX };

            for ( auto barrel : m_barrels ) {
                if ( barrel.network_id == root_barrel.network_id || barrel.position.dist_to( target_position ) >
                    lowest_distance )
                    continue;

                closest_barrel  = barrel;
                lowest_distance = barrel.position.dist_to( target_position );
            }

            if ( root_barrel.position.dist_to( closest_barrel.position ) < 690.f ) {
                return {
                    root_barrel,
                    closest_barrel,
                    { closest_barrel },
                    1,
                    true
                };
            }

            chain_instance_t best_chain{ };

            for ( auto barrel : m_barrels ) {
                if ( barrel.network_id == closest_barrel.network_id || barrel.network_id == root_barrel.
                    network_id )
                    continue;

                if ( barrel.position.dist_to( closest_barrel.position ) > m_barrel_chain_range ||
                    barrel.position.dist_to( root_barrel.position ) > m_barrel_chain_range )
                    continue;

                best_chain = { root_barrel, closest_barrel, { barrel, closest_barrel }, 2, true };
                break;
            }

            if ( !best_chain.valid ) return { root_barrel, root_barrel, { }, 0, true };

            return best_chain;
        }

        auto get_direct_chain( barrel_instance_t root_barrel, Vec3 target_position ) const -> direct_chain_instance_t{
            barrel_instance_t closest_barrel{ };
            auto              lowest_distance{ FLT_MAX };

            for ( auto barrel : m_barrels ) {
                if ( barrel.network_id == root_barrel.network_id ||
                    barrel.position.dist_to( target_position ) > lowest_distance )
                    continue;

                closest_barrel  = barrel;
                lowest_distance = barrel.position.dist_to( target_position );
            }

            if ( root_barrel.position.dist_to( closest_barrel.position ) < 690.f ) {
                direct_chain_instance_t chain{ root_barrel, closest_barrel, true };

                return chain;
            }

            return { };
        }

        static auto get_chain_travel_time( const chain_instance_t& chain, Vec3 target_position ) -> float{
            return chain.count * 0.3f;
        }

        auto get_closest_barrel( const Vec3& position ) const -> Object*{
            if ( m_barrels.empty( ) ) return { };

            Object* best_barrel{ };
            auto    lowest_distance = FLT_MAX;

            for ( auto inst : m_barrels ) {
                if ( inst.position.dist_to( position ) > lowest_distance ) continue;

                auto barrel = g_entity_list.get_by_index( inst.index );
                if ( !barrel || barrel->is_dead( ) ) continue;

                best_barrel     = barrel.get( );
                lowest_distance = inst.position.dist_to( position );
            }

            return best_barrel;
        }

        auto get_barrels_in_range( const float range = 0.f ) const -> std::vector< barrel_instance_t >{
            const auto pred           = g_features->prediction->predict_default( g_local->index, 0.f, true );
            const auto local_position = pred.has_value( ) ? pred.value( ) : g_local->position;

            std::vector< barrel_instance_t > barrel_candidates{ };

            for ( auto barrel : m_barrels ) {
                if ( range > 0.f && barrel.position.dist_to( local_position ) > range
                    || range <= 0.f && !g_features->orbwalker->is_attackable( barrel.index ) )
                    continue;

                barrel_candidates.push_back( barrel );
            }

            return barrel_candidates;
        }

        auto update_humanizer( ) -> void{
            if ( !g_config->gangplank.humanize_barrel_placement->get< bool >( ) ) {
                m_humanized_chain_range = 685.f;
                return;
            }

            const auto randomized   = rand( ) % g_config->gangplank.humanizer_strength->get< int >( );
            m_humanized_chain_range = 685.f - static_cast< float >( randomized );
        }

        auto cast_tracking( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 0 && sci->server_cast_time - 0.05f > *g_time ) {
                    m_q_active           = true;
                    m_q_server_cast_time = sci->server_cast_time;

                    m_was_target_barrel          = false;
                    m_q_target_barrel_network_id = 0;

                    auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                    if ( target ) {
                        m_q_target_distance = sci->start_position.dist_to( target->position );
                        auto barrel         = get_barrel( target->network_id );
                        if ( barrel && barrel->primed_time < sci->server_cast_time + sci->start_position.
                            dist_to( barrel->position ) / 2400.f ) {
                            set_barrel_detonation(
                                barrel->network_id,
                                m_q_server_cast_time + sci->start_position.dist_to( barrel->position ) / 2400.f
                            );

                            m_was_target_barrel          = true;
                            m_q_target_barrel_network_id = barrel->network_id;
                        }
                    }
                }
            } else {
                if ( m_was_target_barrel && m_q_server_cast_time - *g_time <= 0.125f )
                    post_detonation_barrel(
                        m_q_target_barrel_network_id
                    );

                if ( m_q_active && *g_time > m_q_server_cast_time ) {
                    m_should_detonate = false;
                    m_q_active        = false;
                }
            }

            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 2 && sci->server_cast_time > *g_time ) {
                    m_e_active           = true;
                    m_e_server_cast_time = sci->server_cast_time;

                    barrel_instance_t barrel{ };

                    barrel.is_object_created = false;
                    barrel.position          = sci->end_position;
                    barrel.start_time        = *g_time;
                    barrel.end_time          = sci->server_cast_time;
                    barrel.primed_time       = barrel.end_time;
                    m_barrels.push_back( barrel );
                }
            } else if ( m_e_active ) {
                if ( m_e_server_cast_time - *g_time <= 0.15f ) {
                    autoattack_logic( );
                    //q_barrel_logic( );
                }

                if ( *g_time > m_e_server_cast_time ) {
                    m_should_detonate = false;
                    m_e_active        = false;
                }
            }

            if ( !m_aa_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->is_autoattack && sci->server_cast_time > *g_time ) {
                    m_aa_active           = true;
                    m_aa_server_cast_time = sci->server_cast_time;

                    m_should_detonate       = false;
                    m_autoattack_detonation = false;
                    m_was_target_barrel     = false;

                    auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                    if ( target ) {
                        auto barrel = get_barrel( target->network_id );
                        if ( barrel && barrel->primed_time <= sci->server_cast_time ) {
                            set_barrel_detonation( barrel->network_id, sci->server_cast_time );

                            m_aa_target_barrel_network_id = barrel->network_id;
                            m_was_target_barrel           = true;
                        }
                    }
                }
            } else {
                if ( m_was_target_barrel && m_aa_server_cast_time - *g_time < 0.025f )
                    post_detonation_barrel(
                        m_aa_target_barrel_network_id
                    );

                if ( *g_time > m_aa_server_cast_time ) m_aa_active = false;
            }
        }

        auto get_barrel( const unsigned network_id ) const -> std::optional< barrel_instance_t >{
            for ( const auto barrel : m_barrels )
                if ( barrel.network_id == network_id )
                    return std::make_optional(
                        barrel
                    );

            return std::nullopt;
        }

        auto set_barrel_detonation( const unsigned network_id, const float detonation_time ) -> void{
            for ( auto& barrel : m_barrels ) {
                if ( barrel.network_id != network_id ) continue;


                barrel.will_detonate     = true;
                barrel.detonation_time   = detonation_time;
                barrel.is_chain_reaction = false;
                //std::cout << "[ DETONATION ] Set barrel to detonate in " << detonation_time - *g_time << " seconds\n";

                calculate_detonation_consequences( barrel );
                barrel.calculated_detonation = true;
                return;
            }
        }

        auto update_barrels( ) -> void{
            const auto activation_delay = g_local->level >= 13 ? 0.5f : g_local->level >= 7 ? 1.f : 2.f;

            for ( const auto object : g_entity_list.get_enemy_minions( ) ) {
                if ( !object ||
                    object->is_dead( ) ||
                    object->is_invisible( ) ||
                    object->dist_to_local( ) > 1500.f ||
                    !object->is_barrel( ) ||
                    is_barrel_tracked( object->network_id )
                )
                    continue;

                barrel_instance_t instance{ };

                instance.index      = object->index;
                instance.network_id = object->network_id;
                instance.position   = object->position;

                const auto buff = g_features->buff_cache->get_buff(
                    object->index,
                    ct_hash( "gangplankebarrelactive" )
                );
                if ( !buff || !buff->buff_data ) continue;

                instance.start_time  = buff->buff_data->start_time;
                instance.primed_time = instance.start_time + activation_delay * 2.f;
                instance.end_time    = buff->buff_data->end_time;

                m_barrels.push_back( instance );
            }

            if ( m_barrels.empty( ) ) return;

            for ( auto& barrel : m_barrels ) {
                if ( barrel.end_time <= *g_time ) {
                    remove_barrel( barrel.network_id );
                    continue;
                }

                if ( !barrel.is_object_created ) continue;

                auto object = g_entity_list.get_by_index( barrel.index );
                if ( !object ) {
                    remove_barrel( barrel.network_id );
                    continue;
                }

                object.update( );

                if ( object->is_dead( ) ) {
                    remove_barrel( barrel.network_id );
                    continue;
                }

                if ( !barrel.is_primed ) {
                    const auto simulated_health = 3.f - std::min(
                        std::floor( ( *g_time - barrel.start_time ) / activation_delay ),
                        2.f
                    );

                    if ( simulated_health > object->health && !barrel.fixed_prime_time ) {
                        barrel.primed_time -= activation_delay;
                        barrel.fixed_prime_time = true;
                    }

                    if ( barrel.primed_time <= *g_time || object->health == 1 ) {
                        barrel.is_primed   = true;
                        barrel.primed_time = *g_time;
                    }
                }

                if ( barrel.will_detonate && barrel.detonation_time + 0.075f < *g_time ) {
                    barrel.will_detonate         = false;
                    barrel.calculated_detonation = false;
                    barrel.detonation_time       = 0.f;
                }

                if ( barrel.will_detonate == barrel.calculated_detonation ) continue;

                calculate_detonation_consequences( barrel );
                barrel.calculated_detonation = true;
            }
        }

        auto calculate_detonation_consequences( const barrel_instance_t& root_barrel ) -> void{
            if ( !root_barrel.will_detonate ) return;

            const auto detonation_time = root_barrel.detonation_time;

            for ( auto& barrel : m_barrels ) {
                if ( barrel.network_id == root_barrel.network_id ||
                    barrel.position.dist_to( root_barrel.position ) > 690.f )
                    continue;

                if ( barrel.will_detonate && barrel.detonation_time < detonation_time ) continue;

                const auto simulated_detonation_time = detonation_time + m_chain_detonation_delay;
                //std::cout << "[ CHAIN DETONATION ] Calculated indirect detonation time\n";

                if ( barrel.end_time < simulated_detonation_time ) continue;

                barrel.detonation_time   = simulated_detonation_time;
                barrel.is_chain_reaction = true;
                barrel.signal_origin     = root_barrel.position;
                barrel.will_detonate     = true;
            }
        }

        auto is_barrel_nearby( const Vec3& position ) const -> bool{
            for ( auto barrel : m_barrels ) if ( barrel.position.dist_to( position ) <= 345.f ) return true;

            return false;
        }

        auto phantom_barrel_logic( ) -> void{
            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::harass || !m_slot_q->is_ready(
                true
            ) || m_slot_e->charges == 0 ) {
                m_phantom_active = false;

                if ( m_magnet_active ) {
                    if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                    g_features->orbwalker->allow_movement( true );
                    g_features->orbwalker->allow_attacks( true );
                    m_magnet_active = false;
                }

                if ( m_barrel_glowing ) {
                    if ( g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            m_phantom_barrel_network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 25 ),
                            3,
                            3,
                            true
                        );

                        m_barrel_glowing = true;
                    }
                }

                return;
            }

            if ( !m_phantom_active ) {
                if ( !GetAsyncKeyState( 0x01 ) ) return;

                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                int16_t  barrel_index{ };
                unsigned barrel_nid{ };

                auto lowest_distance{ 200.f };

                for ( auto barrel : m_barrels ) {
                    if ( barrel.position.dist_to( cursor ) > lowest_distance ) continue;

                    barrel_index    = barrel.index;
                    barrel_nid      = barrel.network_id;
                    lowest_distance = barrel.position.dist_to( cursor );
                }

                if ( barrel_index == 0 ) return;

                m_phantom_active            = true;
                m_phantom_barrel_index      = barrel_index;
                m_phantom_barrel_network_id = barrel_nid;

                if ( g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow( barrel_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 5, 2, false );

                    m_barrel_glowing = true;
                }
            }


            if ( !m_barrel_glowing && g_function_caller->is_glow_queueable( ) ) {
                g_function_caller->enable_glow(
                    m_phantom_barrel_network_id,
                    D3DCOLOR_ARGB( 255, 255, 255, 25 ),
                    5,
                    2,
                    false
                );

                m_barrel_glowing = true;
            }

            magnet_qe( );
        }

        auto magnet_qe( ) -> void{
            if ( !m_phantom_active || g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::harass ) {
                disable_magnet( );
                return;
            }

            const auto barrel = g_entity_list.get_by_index( m_phantom_barrel_index );
            if ( !barrel ) {
                disable_magnet( );
                return;
            }

            Vec3       goal_point{ };
            auto       lowest_distance{ 9999.f };
            bool       found_point{ };
            const auto local_under_turret = is_position_in_turret_range( g_local->position );

            const auto magnet_position{ barrel->position };
            const auto magnet_radius{ 627.5f };

            const auto candidates = g_render->get_3d_circle_points(
                magnet_position,
                magnet_radius,
                10,
                33.f,
                ( g_local->position - magnet_position ).normalize( ).rotated( -0.33f )
            );

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            m_magnet_points.clear( );
            for ( auto& point : candidates ) {
                if ( point.dist_to( g_local->position ) > 250.f + ( m_magnet_active ? 30.f : 0.f ) ||
                    point.dist_to( g_local->position ) > lowest_distance &&
                    g_local->position.dist_to( magnet_position ) > magnet_radius ||
                    point.dist_to( cursor ) > lowest_distance &&
                    g_local->position.dist_to( magnet_position ) <= magnet_radius ) {
                    m_magnet_points.push_back( point );
                    continue;
                }

                auto walk_point{ point };

                if ( point.dist_to( g_local->position ) <= 120.f ) {
                    continue;
                    auto point_dist   = point.dist_to( g_local->position );
                    auto extend_value = 180.f - point_dist;

                    walk_point = magnet_position.extend( point, magnet_radius - extend_value );
                }

                if ( g_navgrid->is_wall( walk_point ) ||
                    !local_under_turret && is_position_in_turret_range( walk_point ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                goal_point      = walk_point;
                lowest_distance = g_local->position.dist_to( magnet_position ) < magnet_radius
                                      ? point.dist_to( cursor )
                                      : point.dist_to( g_local->position );
                found_point = true;

                m_magnet_points.push_back( walk_point );
            }

            if ( !found_point ) {
                // enable orb movement when not magneting
                // std::cout << "[magnet] found no valid point\n";
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }

            if ( !g_features->orbwalker->is_movement_disabled( ) ) {
                g_features->orbwalker->allow_movement( false );
                g_features->orbwalker->allow_attacks( false );
            }

            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                const auto path     = aimgr->get_path( );
                const auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( goal_point ) < 10.f ) return;

                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_magnet_active  = true;
                }
            }
        }

        auto disable_magnet( ) -> void{
            m_phantom_active = false;

            if ( m_magnet_active ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                m_magnet_active = false;
            }

            if ( m_barrel_glowing ) {
                if ( g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        m_phantom_barrel_network_id,
                        D3DCOLOR_ARGB( 255, 255, 255, 25 ),
                        3,
                        3,
                        true
                    );

                    m_barrel_glowing = true;
                }
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ),
                    target->index,
                    true
                );
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2400.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2400.f;
            }
            case ESpellSlot::w:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto remove_barrel( const unsigned network_id ) -> void{
            if ( m_barrels.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_barrels,
                    [ & ]( const barrel_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_barrels.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_barrel_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_barrels ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto disable_spells( const float duration ) -> void{
            m_disable_expire_time = *g_time + duration;
            m_spells_disabled     = true;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_aa_time{ };
        float m_last_cast_time{ };

        // Phantom barrel harass
        bool     m_phantom_active{ };
        int16_t  m_phantom_barrel_index{ };
        unsigned m_phantom_barrel_network_id{ };
        bool     m_barrel_glowing{ };

        bool                m_magnet_active{ };
        std::vector< Vec3 > m_magnet_points{ };
        float               m_last_move_time{ };

        // e humanizer
        float m_humanized_chain_range{ };


        // spell disable
        bool  m_spells_disabled{ };
        float m_disable_expire_time{ };

        // cast tracking
        bool  m_q_active{ };
        float m_q_server_cast_time{ };

        bool     m_was_target_barrel{ };
        float    m_q_target_distance{ };
        unsigned m_q_target_barrel_network_id{ };

        bool  m_e_active{ };
        float m_e_server_cast_time{ };
        Vec3  m_last_e_position{ };

        bool  m_aa_active{ };
        float m_aa_server_cast_time{ };

        unsigned m_aa_target_barrel_network_id{ };

        bool    m_should_detonate{ };
        int16_t m_detonation_target{ };
        bool    m_autoattack_detonation{ };

        // E data
        float m_barrel_chain_range{ 680.f };
        float m_barrel_connect_radius{ 345.f };
        float m_chain_detonation_delay{ 0.3f };

        float m_barrel_detonation_radius{ 365.f };

        float m_barrel_targeting_radius{ 325.f };

        float m_barrel_hitbox_compensation{ 300.f };

        // e simulation
        Vec3  m_simulated_barrel_point{ };
        float m_last_simulation_time{ };

        Vec3  m_phantom_barrel_point{ };
        float m_phantom_detonation_time{ };
        float m_phantom_detonation_duration{ };

        // barrel tracking
        std::vector< barrel_instance_t > m_barrels{ };

        std::vector< float > m_q_damage = { 0.f, 10.f, 40.f, 70.f, 100.f, 130.f };

        float m_q_range{ 625.f };
        float m_w_range{ 0.f };
        float m_e_range{ 1000.f };
    };
} // namespace features::champion_modules
