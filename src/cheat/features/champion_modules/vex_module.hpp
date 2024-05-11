#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class vex_module final : public IModule {
    public:
        virtual ~vex_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "vex_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Vex" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation     = g_window->push( _( "vex" ), menu_order::champion_module );
            const auto q_settings     = navigation->add_section( _( "q settings" ) );
            const auto drawings       = navigation->add_section( _( "drawings" ) );
            const auto w_settings     = navigation->add_section( _( "w settings" ) );
            const auto combo_settings = navigation->add_section( _( "combo settings" ) );
            const auto e_settings     = navigation->add_section( _( "e settings" ) );
            const auto r_settings     = navigation->add_section( _( "recall ult" ) );

            q_settings->checkbox( _( "enable" ), g_config->vex.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->vex.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->vex.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->vex.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->vex.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->vex.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            combo_settings->checkbox(
                _( "ignore hitchance in full combo (?)" ),
                g_config->vex.combo_override_hitchance
            )->set_tooltip( _( "In full combo, force Q/E hitchance to fast for quick combo" ) );

            r_settings->checkbox( _( "enable recall ult (?)" ), g_config->vex.r_recall_ult )->set_tooltip(
                _( "Will R killable targets who are recalling" )
            );
            r_settings->checkbox( _( "prefer delay (?)" ), g_config->vex.r_recall_ult_delay )->set_tooltip(
                _( "Prefer to wait until last possible time to ult, recommended" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->vex.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->vex.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->vex.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->vex.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->vex.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->vex.q_draw_range->get< bool >( ) &&
                !g_config->vex.w_draw_range->get< bool >( ) &&
                !g_config->vex.e_draw_range->get< bool >( ) &&
                !g_config->vex.r_draw_range->get< bool >( ) &&
                !g_config->vex.r_recall_ult->get< bool >( ) || g_local->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->vex.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vex.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->vex.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vex.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->vex.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vex.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->vex.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->vex.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range + 50.f,
                        Renderer::outline,
                        50,
                        2.f
                    );
                    g_render->circle_minimap( g_local->position, Color( 173, 47, 68, 255 ), m_r_range + 50.f, 50, 1.f );
                }
            }

            if ( m_recall_ult_active ) {
                auto& target = g_entity_list.get_by_index( m_target_index );
                if ( !target ) return;

                const auto  duration   = m_predicted_cast_time - m_baseult_start_time;
                const auto  time_left  = m_predicted_cast_time - *g_time;
                std::string champ_name = target->champion_name.text;

                if ( time_left > 0.f ) {
                    auto angle = 360.f - 360.f * ( time_left / duration );
                    g_render->circle_3d( g_local->position, Color( 50, 255, 50 ), 150.f, 2, 50, 3.f, angle );
                }

                Vec2 indicator_base{ 960.f, 600.f };
                Vec2 texture_size{ 40.f, 40.f };

                Vec2 indicator_data{ indicator_base.x, indicator_base.y + 28.f };
                Vec2 texture_position{ indicator_base.x, indicator_base.y - texture_size.y * 0.5f };

                auto box_color = Color( 255, 255, 50 );

                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                    || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                    std::string warning_text = _( "Wont cast due to orbwalker mode " );

                    if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                        warning_text += _(
                            "COMBO"
                        );
                    } else warning_text += _( "FLEE" );

                    Vec2 indicator_warning{ indicator_base.x, indicator_base.y + 48.f };

                    const auto text_size_warning = g_render->get_text_size( warning_text, g_fonts->get_bold( ), 20 );
                    g_render->text_shadow(
                        Vec2( indicator_warning.x - text_size_warning.x / 2.f, indicator_warning.y ),
                        Color( 255, 50, 50 ),
                        g_fonts->get_bold( ),
                        warning_text.data( ),
                        20
                    );
                    box_color = Color( 200, 200, 200 );
                } else if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                    m_recall_ult_active = false;
                    return;
                }

                std::string text      = "RECALL ULT";
                const auto  text_size = g_render->get_text_size( text, g_fonts->get_bold( ), 32 );

                auto data_text = champ_name + " in ";
                auto time_text = std::to_string( time_left );
                time_text.resize( 4 );

                data_text += time_text + "s";
                const auto text_size_data = g_render->get_text_size( data_text, g_fonts->get_bold( ), 16 );

                Vec2 box_start = { indicator_base.x - text_size.x / 2.f - 3.f, indicator_base.y + 3.f };

                g_render->filled_box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, Color( 20, 20, 20, 90 ) );
                g_render->box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, box_color );

                auto texture_path =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    texture_path.has_value(  ) ? *texture_path : ""
                );
                if ( texture ) {
                    g_render->image(
                        { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                        texture_size,
                        texture
                    );
                }

                g_render->text_shadow(
                    Vec2( indicator_base.x - text_size.x / 2.f, indicator_base.y ),
                    box_color,
                    g_fonts->get_bold( ),
                    text.data( ),
                    32
                );
                g_render->text_shadow(
                    Vec2( indicator_data.x - text_size_data.x / 2.f, indicator_data.y ),
                    Color( 255, 255, 255 ),
                    g_fonts->get_bold( ),
                    data_text.data( ),
                    16
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) || g_features->orbwalker->
                in_attack( ) )
                return;

            m_r_range = 1450.f + m_slot_r->level * 500.f;

            recall_ult( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_w( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->vex.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->vex.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto position_pred = g_features->prediction->predict_default( target->index, 0.15f + 0.833f );
            if ( !position_pred ) return false;

            Vec3 cast_position{ };

            const auto fast_hitchance = g_config->vex.combo_override_hitchance->get< bool >( )
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && GetAsyncKeyState(
                    VK_CONTROL
                );


            if ( g_local->position.dist_to( *position_pred ) > 500.f ) {
                const auto extended_position = g_local->position.extend( *position_pred, 500.f );

                auto pred = g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    3200.f,
                    100.f,
                    0.15f + 0.8333f,
                    extended_position
                );
                if ( !pred.valid || g_local->position.dist_to( pred.position ) >= m_q_range
                    || !fast_hitchance && ( int )pred.hitchance < g_config->vex.q_hitchance->get< int >( ) )
                    return false;

                cast_position = pred.position;
            } else {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 600.f, 100.f, 0.15f );
                if ( !pred.valid || !fast_hitchance && ( int )pred.hitchance < g_config->vex.q_hitchance->get<
                    int >( ) )
                    return false;

                cast_position = pred.position;
            }

            if ( cast_position.dist_to( g_local->position ) > m_q_range ) return false;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->vex.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 1.25f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_w_range ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->vex.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto fast_hitchance = g_config->vex.combo_override_hitchance->get< bool >( )
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && GetAsyncKeyState(
                    VK_CONTROL
                );

            auto pred = g_features->prediction->predict( target->index, m_e_range, 1300.f, 200.f, 0.25f );
            if ( !pred.valid || !fast_hitchance && pred.hitchance < static_cast< Prediction::EHitchance >( g_config->
                vex.e_hitchance->get< int >( ) ) )
                return false;

            const auto radius = 150.f + g_local->position.dist_to( pred.position ) / 800.f * 100.f;

            pred = g_features->prediction->predict( target->index, m_e_range, 1300.f, radius, 0.25f );
            if ( !pred.valid || !fast_hitchance && pred.hitchance < static_cast< Prediction::EHitchance >( g_config->
                vex.e_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto recall_ult( ) -> void{
            if ( !g_config->vex.r_recall_ult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready(
                    true
                )
                || rt_hash( m_slot_r->get_name().c_str() ) == ct_hash( "VexR2" ) ) {
                m_recall_ult_active = false;
                return;
            }


            if ( m_recall_ult_active ) {
                base_ult_tracking( );
                return;
            }

            const Object* target{ };
            bool          found_target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || !enemy->is_recalling( ) || enemy->dist_to_local( ) >=
                    m_r_range )
                    continue;

                const auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                const auto recall_time_left = recall->finish_time - *g_time;
                const auto travel_time      = 0.25f + g_local->position.dist_to( enemy->position ) / 1600.f;
                if ( travel_time >= recall_time_left ) continue;

                float health_regenerated{ };

                if ( enemy->is_invisible( ) ) {
                    const auto last_seen_data = g_features->tracker->get_last_seen_data( enemy->index );
                    if ( !last_seen_data ) continue;

                    const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                    const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                    if ( time_missing * enemy->movement_speed >= 130.f ) continue;

                    health_regenerated = ( *g_time - last_seen_data->last_seen_time ) * enemy->total_health_regen;
                    health_regenerated += std::ceil( travel_time ) * enemy->total_health_regen;
                } else health_regenerated = std::ceil( travel_time ) * enemy->total_health_regen;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy );
                if ( damage < enemy->health + health_regenerated ) continue;

                target       = enemy;
                found_target = true;
                break;
            }

            if ( !found_target || !target ) return;

            m_target_index       = target->index;
            m_baseult_start_time = *g_time;
            m_recall_ult_active  = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_recall_ult_active || !m_slot_r->is_ready( true ) || rt_hash( m_slot_r->get_name().c_str() ) ==
                ct_hash( "VexR2" ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || !target->is_recalling( ) || target->dist_to_local( ) > m_r_range ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 0.25f + target->dist_to_local( ) / 1600.f;
            const auto damage           = get_spell_damage( ESpellSlot::r, target.get( ) );

            const auto time_until_hit = recall_time_left - travel_time;
            auto       health_regenerated{ std::ceil( time_until_hit ) * target->total_health_regen };
            auto       min_possible_health_regen = std::ceil( travel_time ) * target->total_health_regen;

            if ( target->is_invisible( ) ) {
                const auto last_seen_data = g_features->tracker->get_last_seen_data( target->index );
                if ( !last_seen_data ) {
                    m_recall_ult_active = false;
                    return;
                }

                const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                if ( time_missing * target->movement_speed > 130.f ) {
                    m_recall_ult_active = false;
                    return;
                }

                health_regenerated += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
                min_possible_health_regen += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
            }

            bool should_cast{ };

            m_predicted_cast_time = recall->finish_time - travel_time - 0.5f;

            if ( damage > target->health + health_regenerated ) should_cast = time_until_hit <= 0.5f;
            else if ( damage > target->health + min_possible_health_regen ) {
                if ( g_config->vex.r_recall_ult_delay->get< bool >( ) ) {
                    int        max_wait_time{ };
                    const auto seconds_until_recall = static_cast< int >( std::floor( recall_time_left - 1.f ) );

                    for ( auto i = 1; i <= seconds_until_recall; i++ ) {
                        const auto regen_amount = min_possible_health_regen + target->total_health_regen * i;
                        if ( target->health + regen_amount >= damage ) break;

                        max_wait_time = i;
                    }

                    m_predicted_cast_time = recall->finish_time - travel_time - ( static_cast< float >( max_wait_time )
                        + 1.f ) - 0.5f;
                    should_cast = max_wait_time <= 1;
                } else should_cast = true;
            } else {
                m_recall_ult_active = false;
                return;
            }

            if ( !should_cast ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                m_recall_ult_active = false;
                return;
            }

            if ( cast_spell( ESpellSlot::r, target->position ) ) {
                m_recall_ult_active = false;
                m_last_r_time       = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                auto damage = m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.2f;
                damage += m_r2_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.5f;

                return helper::calculate_damage( damage, target->index, false );
            }
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            case ESpellSlot::r:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        // recall ult
        bool    m_recall_ult_active{ };
        int16_t m_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        std::vector< float > m_q_damage  = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::vector< float > m_e_damage  = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage  = { 0.f, 75.f, 125.f, 175.f };
        std::vector< float > m_r2_damage = { 0.f, 150.f, 250.f, 350.f };

        float m_q_range{ 1200.f };
        float m_w_range{ 475.f };
        float m_e_range{ 800.f };
        float m_r_range{ 2000.f };
    };
}
