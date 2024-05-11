#pragma once
#include "../../menu/menu.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "module.hpp"


namespace features::champion_modules {
    class akshan_module final : public IModule {
    public:
        virtual ~akshan_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "akshan_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Akshan" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "akshan" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->akshan.q_enabled );
            q_settings->checkbox( _( "weave between aa" ), g_config->akshan.q_aa_reset );
            q_settings->select(
                _( "hitchance" ),
                g_config->akshan.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->akshan.e_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->akshan.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->akshan.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->akshan.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->akshan.q_draw_range->get< bool >( ) &&
                !g_config->akshan.e_draw_range->get< bool >( ) &&
                !g_config->akshan.r_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );

            auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( g_config->akshan.q_draw_range->get< bool >( ) && slot && slot->level > 0 &&
                ( slot->is_ready( true ) || !g_config->akshan.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    m_circle_segments,
                    2.f
                );
            }

            slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( g_config->akshan.e_draw_range->get< bool >( ) && slot && slot->level > 0 &&
                ( slot->is_ready( true ) && !m_e_active || !g_config->akshan.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 173, 47, 68, 255 ),
                    m_e_range,
                    Renderer::outline,
                    m_circle_segments,
                    2.f
                );
            }

            //for ( auto point : m_hook_points ) {
            //  g_render->circle_3d( point, Color::white( ), 20.f, Renderer::outline, 26, 2.f );
            //}
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            spell_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( spell_q( ) ) return;
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->akshan.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || g_config->akshan.q_aa_reset->get< bool >( ) && !g_features->orbwalker
                ->should_reset_aa( ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1500.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->akshan.q_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;
                return true;
            }

            return true;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->akshan.e_enabled->get< bool >( ) ) return false;
            //std::cout << "[ Akshan: E ] Name: " << m_slot_e->get_name( ) << " | state: " << std::dec << (int)m_slot_e->get_usable_state(  ) << std::endl;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            switch ( rt_hash( m_slot_e->get_name( ).data( ) ) ) {
            case ct_hash( "AkshanE" ):
            {
                if ( !GetAsyncKeyState( 0x37 ) || *g_time - m_last_e_time <= 0.4f ) return false;

                bool found_point{ };
                Vec3 selected_point{ };

                for ( auto point : m_hook_points ) {
                    if ( point.dist_to( cursor ) > 500.f ) continue;

                    found_point    = true;
                    selected_point = point;
                    break;
                }

                if ( !found_point ) return false;

                if ( cast_spell( ESpellSlot::e, selected_point ) ) {
                    m_last_e_time = *g_time;

                    std::cout << "Casted hookshot | " << *g_time << std::endl;
                }

                break;
            }
            case ct_hash( "AkshanE2" ):
            {
                if ( *g_time - m_last_cast_time < 0.1f ) return false;

                if ( cast_spell( ESpellSlot::e, cursor ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;

                    std::cout << "E Recast | " << *g_time << std::endl;
                }
                break;
            }
            case ct_hash( "AkshanE3" ):
                break;
            default:
                return false;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_cast_time{ };

        std::array< Vec3, 2 > m_hook_points = { Vec3{ 4148.f, 155.898f, 7746.f }, Vec3{ 2576.f, 155.934f, 7602.f } };

        float m_q_range{ 850.f };
        float m_w_range{ 0.f };
        float m_e_range{ 750.f };

        bool m_e_active{ };
        bool m_w_active{ };

        int32_t m_circle_segments{ 50 };
    };
} // namespace features::champion_modules
