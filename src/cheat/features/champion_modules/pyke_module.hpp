#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"

namespace features::champion_modules {
    class pyke_module final : public IModule {
    public:
        virtual ~pyke_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "pyke_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Pyke" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "pyke" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->pyke.q_enabled );
            q_settings->checkbox( _( "allow melee q" ), g_config->pyke.q_melee );
            q_settings->select(
                _( "hitchance" ),
                g_config->pyke.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->pyke.e_enabled );
            e_settings->checkbox( _( "check safe position" ), g_config->pyke.e_safe_position_check )->set_tooltip(
                _( "if E end position is dangerous, dont cast" )
            );
            e_settings->select(
                _( "dash direction" ),
                g_config->pyke.e_mode,
                { _( "Backward" ), _( "Forward" ), _( "Both" ) }
            );

            //r_interrupt_q
            r_settings->checkbox( _( "enable" ), g_config->pyke.r_enabled );
            r_settings->checkbox( _( "interrupt q (?)" ), g_config->pyke.r_interrupt_q )->set_tooltip(
                _( "Interrupt Q channel if enemy is executable" )
            );
            r_settings->select(
                _( "hitchance (?)" ),
                g_config->pyke.r_new_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            )->set_tooltip( _( "we recommend fast hitchance for fast ults" ) );

            drawings->checkbox( _( "draw q range" ), g_config->pyke.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->pyke.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->pyke.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->pyke.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->pyke.q_draw_range->get< bool >( ) &&
                !g_config->pyke.e_draw_range->get< bool >( ) &&
                !g_config->pyke.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->pyke.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->pyke.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_updated_q_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( g_config->pyke.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->pyke.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }


            if ( g_config->pyke.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->pyke.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 200, 50, 75, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( sci && sci->slot == 0 ) m_last_q_channel_time = *g_time;
            else if ( !sci ) m_updated_q_range = 1100.f;


            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "PykeQ" ) ) ) {
                    if ( g_config->pyke.r_interrupt_q->get< bool >( ) ) spell_r( );

                    spell_q( );
                    return;
                }


                spell_r( );
                spell_e( );

                if ( g_features->orbwalker->in_attack( ) ) return;

                spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->pyke.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 1.f ) return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "PykeQ" ) );

            if ( !buff ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > m_q_range * 1.1f ) return false;

                const auto pred = g_features->prediction->predict_default( target->index, 0.5f );
                if ( !pred || g_features->prediction->minion_in_line_predicted(
                        g_local->position,
                        target->position,
                        70.f,
                        0.15f,
                        2000.f
                    ) ||
                    g_features->prediction->minion_in_line_predicted( g_local->position, *pred, 70.f, 0.15f, 2000.f ) )
                    return false;

                if ( cast_spell( ESpellSlot::q ) ) {
                    g_features->orbwalker->set_cast_time( 0.025f );
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                    return true;
                }

                return false;
            }
            m_q_cast_begin = 0.f;
            m_channeling_q = true;

            auto bonus_range = 0.f;
            if ( *g_time - buff->buff_data->start_time >= 0.5f - g_features->orbwalker->get_ping( ) ) {
                const auto time_channeled = *g_time - buff->buff_data->start_time + g_features->orbwalker->get_ping( );
                bonus_range               = std::min( std::floor( ( time_channeled - 0.4f ) / 0.1f ) * 116.67f, 700.f );
            }
            auto hitchance = *g_time - buff->buff_data->start_time > 2.f && g_config->pyke.q_hitchance->get< int >( ) >
                             0
                                 ? 0
                                 : g_config->pyke.q_hitchance->get< int >( );

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            m_updated_q_range = m_q_base_range + bonus_range;

            const auto is_melee = *g_time - buff->buff_data->start_time <= 0.4f;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_base_range + bonus_range,
                is_melee ? 0.f : 2000.f,
                is_melee ? 100.f : 70.f,
                is_melee ? 0.25f : 0.2f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ||
                !g_config->pyke.q_melee->get< bool >( ) && is_melee || !is_melee && g_features->prediction->
                minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    70.f,
                    0.2f,
                    2000.f
                ) )
                return false;

            if ( release_chargeable( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time  = *g_time;
                m_channeling_q = false;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->pyke.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->is_ready(
                    true
                ) ||
                *g_time - m_last_q_channel_time >= 1.f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 800.f || // dont E if can execute with ult
                m_slot_r->is_ready( true ) && target->health < get_ult_execute_threshold( ) )
                return false;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr || !aimgr->is_moving || !aimgr->is_dashing ) return false;

            const auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return false;

            const auto cast_direction = path[ path.size( ) - 1 ];
            if ( cast_direction.dist_to( g_local->position ) > 500.f ) return false;

            if ( g_config->pyke.e_mode->get< int >( ) != 2 ) {
                const auto current_pos = g_local->position;
                const auto v1          = cast_direction - current_pos;
                const auto v2          = path[ 0 ] - current_pos;
                const auto dot         = v1.normalize( ).dot_product( v2.normalize( ) );
                const auto angle       = acos( dot ) * 180.f / 3.14159265358979323846f;

                if ( g_config->pyke.e_mode->get< int >( ) == 0 && angle < 80.f ||
                    g_config->pyke.e_mode->get< int >( ) == 1 && angle > 20.f ) {
                    //std::cout << "bad angle: " << angle << std::endl;
                    return false;
                }
            }

            if ( g_config->pyke.e_safe_position_check->get< bool >( ) ) {
                const auto dash_end_position = g_local->position.extend( cast_direction, 550.f );

                if ( !is_position_in_turret_range( g_local->position ) && is_position_in_turret_range(
                    dash_end_position
                ) )
                    return false;

                int enemy_count{ };
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->network_id == target->
                        network_id
                        || enemy->position.dist_to( dash_end_position ) > 500.f )
                        continue;

                    ++enemy_count;
                }

                if ( enemy_count > 1 ) return false;
            }

            if ( cast_spell( ESpellSlot::e, cast_direction ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->pyke.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.5f ||
                *g_time - m_last_q_time <= 0.5f || !m_slot_r->is_ready( true ) )
                return false;

            bool       allow_r{ };
            Vec3       cast_position{ };
            const auto execute_threshold{ get_ult_execute_threshold( ) };
            unsigned   target_nid{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->network_id == m_ult_target_nid || *g_time - m_last_ult_time <= 1.5f ||
                    g_features->target_selector->is_bad_target( enemy->index ) || enemy->dist_to_local( ) > 1250.f ||
                    enemy->health + enemy->total_health_regen >= execute_threshold )
                    continue;

                const auto pred = g_features->prediction->predict(
                    enemy->index,
                    m_r_range,
                    0.f,
                    125.f,
                    0.5f,
                    { },
                    true
                );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->pyke.
                    r_new_hitchance->get< int >( ) ) )
                    return false;

                cast_position = pred.position;
                allow_r       = true;
                target_nid    = enemy->network_id;
                break;
            }

            if ( !allow_r || g_local->position.dist_to( cast_position ) >= m_r_range ) return false;

            if ( cast_spell( ESpellSlot::r, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_ult_target_nid = target_nid;
                m_last_ult_time  = *g_time;
                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 0.6f,
                    target->index,
                    true
                );
            case ESpellSlot::r:
            {
                float base_damage;

                switch ( g_local->level ) {
                case 6:
                    base_damage = 250;
                    break;
                case 7:
                    base_damage = 290;
                    break;
                case 8:
                    base_damage = 330;
                    break;
                case 9:
                    base_damage = 370;
                    break;
                case 10:
                    base_damage = 400;
                    break;
                case 11:
                    base_damage = 430;
                    break;
                case 12:
                    base_damage = 450;
                    break;
                case 13:
                    base_damage = 470;
                    break;
                case 14:
                    base_damage = 490;
                    break;
                case 15:
                    base_damage = 510;
                    break;
                case 16:
                    base_damage = 530;
                    break;
                case 17:
                    base_damage = 540;
                    break;
                case 18:
                    base_damage = 550;
                    break;
                default:
                    base_damage = 550;
                    break;
                }

                const auto bonus_mod = g_local->bonus_attack_damage( ) * 0.8f;
                const auto raw       = base_damage + bonus_mod;

                if ( raw >= target->health ) return 9999.f;

                return helper::calculate_damage( raw / 2, target->index, true );
            }
            }
            return 0.f;
        }

        static auto get_ult_execute_threshold( ) -> float{
            float base_damage{ };

            switch ( g_local->level ) {
            case 6:
                base_damage = 250;
                break;
            case 7:
                base_damage = 290;
                break;
            case 8:
                base_damage = 330;
                break;
            case 9:
                base_damage = 370;
                break;
            case 10:
                base_damage = 400;
                break;
            case 11:
                base_damage = 430;
                break;
            case 12:
                base_damage = 450;
                break;
            case 13:
                base_damage = 470;
                break;
            case 14:
                base_damage = 490;
                break;
            case 15:
                base_damage = 510;
                break;
            case 16:
                base_damage = 530;
                break;
            case 17:
                base_damage = 540;
                break;
            default:
                base_damage = 550;
                break;
            }

            base_damage += g_local->bonus_attack_damage( ) * 0.8f;
            base_damage += g_local->get_lethality( ) * 1.5f;

            return base_damage;
        }

        static auto get_q_melee_traveltime( const Object* unit ) -> float{ return 0.25f; }

        static auto get_q_ranged_traveltime( const Object* unit ) -> float{
            const auto tt   = 0.2f + g_local->position.dist_to( unit->position ) / 2000.f;
            const auto pred = g_features->prediction->predict_default( unit->index, tt );
            if ( !pred ) return 0.f;

            return 0.2f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
        }

        static auto get_r_traveltime( const Object* unit ) -> float{ return 0.5f; }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_q_channel_time{ };

        unsigned m_ult_target_nid{ };
        float    m_last_ult_time{ };

        bool  m_channeling_q{ };
        float m_q_cast_begin{ };
        float m_q_base_range{ 400.f };

        std::array< float, 6 > m_q_damage = { 0.f, 100.f, 150.f, 200.f, 250.f, 300.f };
        std::array< float, 6 > m_e_damage = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };

        float m_updated_q_range{ };

        float m_q_range{ 1100.f };
        float m_w_range{ 0.f };
        float m_e_range{ 550.f };
        float m_r_range{ 750.f };
    };
}
