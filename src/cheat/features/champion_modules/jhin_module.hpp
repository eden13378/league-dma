#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class jhin_module final : public IModule {
    public:
        virtual ~jhin_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "jhin_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Jhin" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "jhin" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto exploit    = navigation->add_section( _( "exploit" ) );

            q_settings->checkbox( _( "enable" ), g_config->jhin.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->jhin.q_harass );

            w_settings->checkbox( _( "enable" ), g_config->jhin.w_enabled );
            w_settings->checkbox( _( "killsteal" ), g_config->jhin.w_killsteal );
            w_settings->checkbox( _( "only if can root" ), g_config->jhin.w_only_on_root );
            w_settings->select(
                _( "hitchance" ),
                g_config->jhin.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->jhin.e_enabled );
            e_settings->checkbox( _( "antigapcloser" ), g_config->jhin.e_antigapcloser );
            e_settings->select(
                _( "hitchance" ),
                g_config->jhin.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->jhin.r_enabled );
            r_settings->select(
                _( "hitchance" ),
                g_config->jhin.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );


            drawings->checkbox( _( "draw q range" ), g_config->jhin.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->jhin.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->jhin.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->jhin.dont_draw_on_cooldown );

            exploit->checkbox( _( "force crit exploit (?)" ), g_config->jhin.force_crit )->set_tooltip(
                "will force crit AA if crit chance >= 55%"
            );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->jhin.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->jhin.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range + 65.f,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->jhin.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->jhin.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->jhin.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->jhin.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 3 ) return;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            g_render->circle_3d( cursor, Color( 66, 239, 245 ), 500.f, 2, 80, 2.f );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            m_in_ult = g_local->spell_book.get_spell_cast_info( ) && g_local->spell_book.get_spell_cast_info( )->slot ==
                3;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            killsteal_w( );
            antigapclose_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->jhin.q_harass->get< bool >( ) ) spell_q( );
                break;
            default: ;
            }

            // w stun buff
            // jhinespotteddebuff

            // reload buff
            // JhinPassiveReload
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->jhin.q_enabled->get< bool >( ) || m_in_ult || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "JhinPassiveReload" ) ) && !g_features->
                orbwalker->should_reset_aa( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->jhin.w_enabled->get< bool >( ) || m_in_ult || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->jhin.w_only_on_root->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "jhinespotteddebuff" ) );
                if ( !buff || buff->buff_data->end_time - *g_time <= 0.75f ) return false;
            }

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 45.f, 0.75f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->jhin.w_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.75f );
                return true;
            }

            return false;
        }

        auto killsteal_w( ) -> bool{
            if ( !g_config->jhin.w_killsteal->get< bool >( ) || m_in_ult || *g_time - m_last_w_time <= 0.4f || !m_slot_w
                ->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_w_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::w, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::w, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                5000.f,
                45.f,
                0.75f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->jhin.w_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.75f );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->jhin.e_enabled->get< bool >( ) || m_in_ult || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 80.f, 1.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->jhin.e_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->jhin.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_in_ult
                || !m_slot_r->is_ready( ) )
                return false;

            const auto    cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto          closest_distance{ 99999.f };
            const Object* target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) ) continue;

                const auto distance = enemy->position.dist_to( cursor );
                if ( distance > 500.f ) continue;

                if ( distance < closest_distance ) {
                    target           = enemy;
                    closest_distance = distance;
                }
            }

            if ( !target || target->dist_to_local( ) > 3500.f ) return false;

            auto pred = g_features->prediction->predict( target->index, 3500.f, 5000.f, 80.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < g_config->jhin.r_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->jhin.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 1.f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return;

            const auto path_end = path[ 1 ];

            if ( cast_spell( ESpellSlot::e, path_end ) ) m_last_e_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->attack_damage( ) * 0.5f,
                    target->index,
                    true
                );
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::w:
            {
                const auto tt   = 0.75f + g_local->position.dist_to( target->position ) / 5000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.75f + g_local->position.dist_to( pred.value( ) ) / 5000.f;
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

        bool m_in_ult{ };

        std::array< float, 6 > m_q_damage = { 0.f, 40.f, 65.f, 90.f, 115.f, 140.f };
        std::array< float, 6 > m_w_damage = { 0.f, 60.f, 95.f, 130.f, 165.f, 200.f };

        float m_q_range{ 550.f };
        float m_w_range{ 2520.f };
        float m_e_range{ 750.f };
    };
}
