#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "module.hpp"
#include "../entity_list.hpp"

namespace features::champion_modules {
    class azir_module final : public IModule {
    public:
        virtual ~azir_module( ) = default;

        struct soldier_instance_t {
            int16_t  index{ };
            unsigned network_id{ };

            Object* object{ };

            float start_time{ };
            float end_time{ };

            Vec3 position{ };
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "azir_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Azir" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "azir" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->azir.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->azir.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->azir.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->azir.w_enabled );
            w_settings->checkbox( _( "use in harass" ), g_config->azir.w_harass );

            e_settings->checkbox( _( "enable" ), g_config->azir.e_enabled );
            e_settings->checkbox( _( "shuffle on flee" ), g_config->azir.e_shuffle_on_flee );

            r_settings->checkbox( _( "enable (?)" ), g_config->azir.r_full_combo_shuffle )->set_tooltip(
                _( "Only enabled in full combo" )
            );
            r_settings->checkbox( _( "mode switch key (?)" ), g_config->azir.r_shuffle_key )->set_tooltip(
                _( "Press 'U' to switch modes on the fly" )
            );
            r_settings->select(
                _( "Full combo mode" ),
                g_config->azir.r_direction,
                { _( "To Tower" ), _( "To Allies" ), _( "To Enemies" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->azir.q_draw_range );
            drawings->checkbox( _( "draw soldier timer" ), g_config->azir.w_draw_duration );

            m_combo_mode = g_config->azir.r_direction->get< int >( );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->azir.q_draw_range->get< bool >( ) && !g_config->azir.w_draw_range->get< bool >( ) &&
                !g_config->azir.e_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->azir.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->ahri.dont_draw_on_cooldown->get< bool >( ) ) ) {
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
            Vec2 sp{ };
            for ( const auto inst : m_soldiers ) {
                if ( !inst.object ) continue;

                auto obj = g_entity_list.get_by_index( inst.index );
                if ( !obj ) continue;

                obj.update( );

                auto object_position = obj->position;

                if ( g_config->azir.w_draw_duration->get< bool >( ) && world_to_screen( object_position, sp ) ) {
                    const Vec2 base_position = { sp.x + 50.f, sp.y - 35.f };

                    auto modifier =
                        1.f - std::max( 1.f - ( inst.end_time - *g_time ) / ( inst.end_time - inst.start_time ), 0.f );
                    if ( modifier > 1.f ) modifier = 1.f;
                    else if ( modifier < 0.f ) modifier = 0.f;

                    std::string text = " ";
                    auto        data = std::to_string( inst.end_time - *g_time );
                    data.resize( 3 );

                    text += data + "s";

                    const auto size       = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );
                    auto       dummy_size = g_render->get_text_size( _( " 3.5s" ), g_fonts->get_zabel_16px( ), 16 );

                    g_render->text_shadow(
                        base_position,
                        Color( 255, 255, 255 ),
                        g_fonts->get_zabel_16px( ),
                        text.data( ),
                        16
                    );

                    float bar_width  = dummy_size.x + 5.f;
                    auto  bar_height = 6.f;

                    Vec2 bar_position = { base_position.x, base_position.y + size.y };
                    Vec2 bar_size     = { bar_width, bar_height };

                    Vec2 background_position = { bar_position.x - 1.f, bar_position.y - 1.f };
                    Vec2 background_size     = { bar_size.x + 2.f, bar_size.y + 2.f };

                    g_render->filled_box( background_position, background_size, Color( 10, 10, 10, 155 ) );


                    Vec2 progress_size = Vec2{ bar_size.x * modifier, bar_size.y };

                    g_render->filled_box( bar_position, progress_size, Color( 255, 255, 25 ) );
                }
            }

            if ( !g_config->azir.r_draw_shuffle_mode->get< bool >( ) ) return;

            if ( !world_to_screen( g_local->position, sp ) ) return;
            std::string text = "x";
            switch ( m_combo_mode ) {
            case 0:
                text = "Shuffle Mode: To Tower";
                break;
            case 1:
                text = "Shuffle Mode: To Allies";
                break;
            case 2:
                text = "Shuffle Mode: To Enemies";
                break;
            default:
                break;
            }

            //const auto text = std::format( ( "Shuffle Mode : {}" ), m_r_potential_heals );

            const auto text_size_set = 20.0f;
            const auto text_size     = g_render->get_text_size( text, g_fonts->get_block( ), text_size_set );

            g_render->text_shadow(
                { sp.x - ( text_size.x / 4.f ), sp.y + text_size.y },
                Color( 255, 25, 25, 220 ),
                g_fonts->get_default_bold( ),
                text.c_str( ),
                text_size_set
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            update_soldiers( );
            in_dash( );
            update_r_mode( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                break;

            case Orbwalker::EOrbwalkerMode::harass:
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                spell_w_flee( );
                spell_e_flee( );
                spell_q_flee( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{ return false; }

        auto spell_r( ) -> bool override{ return false; }

        auto spell_q_flee( ) -> bool{
            if ( !g_config->azir.e_shuffle_on_flee->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) || in_dash( ) == false )
                return false;
            auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            for ( const auto inst : m_soldiers ) {
                if ( !inst.object ) continue;

                auto obj = g_entity_list.get_by_index( inst.index );
                if ( !obj ) continue;

                obj.update( );

                auto object_position = obj->position;

                if ( object_position.dist_to( g_local->position ) < 200 ) break;

                //m_closest_to_cursor_soldier = object_position;
            }

            if ( cast_spell( ESpellSlot::q, cursor ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e_flee( ) -> bool{
            if ( !g_config->azir.e_shuffle_on_flee->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) || in_dash( ) )
                return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w_flee( ) -> bool{
            if ( !g_config->azir.e_shuffle_on_flee->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) || in_dash( ) )
                return false;

            auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            if ( cast_spell( ESpellSlot::w, cursor ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }
            return false;
        }

        static auto in_dash( ) -> bool{
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "azireshield" ) );

            if ( buff ) return true;

            return false;
        }

        auto update_soldiers( ) -> void{
            for ( const auto unit : g_entity_list.get_ally_minions( ) ) {
                if ( !unit || unit->is_dead( ) || !unit->is_sand_soldier( ) || is_tracked( unit->network_id ) ) {
                    continue;
                }


                soldier_instance_t inst{ unit->index, unit->network_id, unit, *g_time, *g_time + 10.f, unit->position };


                m_soldiers.push_back( inst );
            }

            for ( auto& inst : m_soldiers ) {
                if ( !inst.object || inst.object->is_dead( ) || !inst.object->is_sand_soldier( ) ) {
                    remove_soldier( inst.network_id );
                    continue;
                }

                inst.position = inst.object->position;
            }
        }

        auto remove_soldier( const unsigned network_id ) -> void{
            if ( m_soldiers.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_soldiers,
                    [ & ]( const soldier_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_soldiers.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_soldiers ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto update_r_mode( ) -> void{
            if ( !g_config->azir.r_shuffle_key->get< bool >( ) ) return;

            if ( !m_key_down && GetAsyncKeyState( 0x55 ) ) {
                m_combo_mode += 1;
                m_key_down = true;
                if ( m_combo_mode > 2 ) m_combo_mode = 0;
            } else if ( m_key_down && !GetAsyncKeyState( 0x55 ) ) m_key_down = false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.35f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + 0.5f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + 0.5f;
            }
            case ESpellSlot::w:
                return 0.25f;
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
        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 60.f, 80.f, 100.f, 120.f, 140.f };

        float m_q_range{ 740.f };
        float m_w_range{ 500.f };

        std::vector< soldier_instance_t > m_soldiers{ };

        Vec3 m_closest_to_cursor_soldier{ };

        bool m_key_down{ };
        int  m_combo_mode{ };
    };
} // namespace features::champion_modules
