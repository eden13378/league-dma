#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class viego_module final : public IModule {
    public:
        virtual ~viego_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "viego_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Viego" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct minion_buff {
            Object*  minion{ };
            unsigned network_id{ };
        };

        struct enemy_damage_t {
            int32_t  index{ };
            unsigned network_id{ };

            float damage{ };
            int   damage_percent{ };
        };

        struct dagger_instance_t {
            unsigned network_id{ };
            float    spawn_time{ };
            Vec3     position{ };
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "viego" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->viego.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->viego.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->viego.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->viego.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->viego.r_enabled );
            r_settings->select(
                _( "hitchance" ),
                g_config->viego.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->sivir.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->sivir.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->viego.q_draw_range->get< bool >( ) &&
                !g_config->viego.w_draw_range->get< bool >( ) &&
                !g_config->viego.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )

                g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->viego.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viego.dont_draw_on_cooldown->get<
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

            if ( g_config->viego.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viego.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        900,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->viego.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->viego.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;


            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( !!g_features->buff_cache->
                                   get_buff(
                                       g_local->index,
                                       ct_hash( "viegopassivetransform" )
                                   ) )
                    transform_manager( );

                spell_r( );
                spell_w( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->ahri.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_r( ) -> bool override{ return false; }

        auto transform_manager( ) -> bool{
            auto spell_name_q = m_slot_q->get_name( );
            debug_log( "{}", spell_name_q );
            if ( spell_name_q == "AsheQ" ) Ashe( );
            else if ( spell_name_q == "AatroxQ" || spell_name_q ==
                "AatroxQ2" || spell_name_q == "AatroxQ3" ) {
                Aatrox( );
                //need spell name
            } else if ( spell_name_q == "ahri" ) {
                Ahri( );
                //need spell name
            } else if ( spell_name_q == "akali" ) Akali( );
            else if ( spell_name_q == "RocketGrab" ) {
                Blitz( );
                //need spell name
            } else if ( spell_name_q == "belveth" ) {
                Belveth( );
                //need spell name
            } else if ( spell_name_q == "brand" ) {
                Brand( );
                //need spell name
            } else if ( spell_name_q == "caitlyn" ) {
                Caitlyn( );
                //need spell name
            } else if ( spell_name_q == "cassiopeia" ) {
                Cassiopeia( );
                //need spell name
            } else if ( spell_name_q == "cho" ) {
                Chogath( );
                //need spell name
            } else if ( spell_name_q == "corki" ) {
                Corki( );
                //need spell name
            } else if ( spell_name_q == "darius" ) {
                Darius( );
                //need spell name
            } else if ( spell_name_q == "draven" ) {
                Draven( );
                //need spell name
            } else if ( spell_name_q == "ezreal" ) {
                Ezreal( );
                //need spell name
            } else if ( spell_name_q == "gnar" ) {
                Gnar( );
                //need spell name
            } else if ( spell_name_q == "illaoi" ) {
                Illaoi( );
                //need spell name
            } else if ( spell_name_q == "irelia" ) {
                Irelia( );
                //need spell name
            } else if ( spell_name_q == "jax" ) {
                Jax( );
                //need spell name
            } else if ( spell_name_q == "jhin" ) {
                Jhin( );
                //need spell name
            } else if ( spell_name_q == "jinx" ) {
                Jinx( );
                //need spell name
            } else if ( spell_name_q == "kaisa" ) {
                Kaisa( );
                //need spell name
            } else if ( spell_name_q == "kalista" ) {
                Kalista( );
                //need spell name
            } else if ( spell_name_q == "karthus" ) {
                Karthus( );
                //need spell name
            } else if ( spell_name_q == "kassadin" ) {
                Kassadin( );
                //need spell name
            } else if ( spell_name_q == "katarina" ) {
                Katarina( );
                //need spell name
            } else if ( spell_name_q == "kayle" ) {
                Kayle( );
                //need spell name
            } else if ( spell_name_q == "kindred" ) {
                Kindred( );
                //need spell name
            } else if ( spell_name_q == "kogmaw" ) {
                Kogmaw( );
                //need spell name
            } else if ( spell_name_q == "leblanc" ) Leblanc( );
            else if ( spell_name_q == "leesin" ) {
                Leesin( );
                //need spell name
            } else if ( spell_name_q == "lucian" ) {
                Lucian( );
                //need spell name
            } else if ( spell_name_q == "lux" ) {
                Lux( );
                //need spell name
            } else if ( spell_name_q == "MF" ) {
                MissFortune( );
                //need spell name
            } else if ( spell_name_q == "Morg" ) {
                Morgana( );
                //need spell name
            } else if ( spell_name_q == "Mundo" ) {
                Mundo( );
                //need spell name
            } else if ( spell_name_q == "nilah" ) {
                Nilah( );
                //need spell name
            } else if ( spell_name_q == "olaf" ) {
                Olaf( );
                //need spell name
            } else if ( spell_name_q == "orianna" ) {
                Orianna( );
                //need spell name
            } else if ( spell_name_q == "pyke" ) {
                Pyke( );
                //need spell name
            } else if ( spell_name_q == "riven" ) {
                Riven( );
                //need spell name
            } else if ( spell_name_q == "ryze" ) {
                Ryze( );
                //need spell name
            } else if ( spell_name_q == "samira" ) {
                Samira( );
                //need spell name
            } else if ( spell_name_q == "senna" ) {
                Senna( );
                //need spell name
            } else if ( spell_name_q == "seraphine" ) {
                Seraphine( );
                //need spell name
            } else if ( spell_name_q == "sett" ) {
                Sett( );
                //need spell name
            } else if ( spell_name_q == "sion" ) {
                Sion( );
                //need spell name
            } else if ( spell_name_q == "sivir" ) Sivir( );
            else if ( spell_name_q == "swain" ) {
                //Swain();
                //need spell name
            } else if ( spell_name_q == "sylas" ) {
                //Sylas();
            } else if ( spell_name_q == "syndra" ) {
                //Syndra();
            }

            return false;
        }

        auto Ashe( ) -> bool{
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_ashe_Q( );
                spell_ashe_W( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_ashe_Q( ) -> bool{
            if ( *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) || !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "asheqcastready" ) );
            if ( !buff || buff->stacks( ) != 4 ) return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_ashe_W( ) -> bool{
            if ( *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            m_w_range = 1100.f;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                2000.f,
                60.f,
                0.25f
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->ashe.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto Aatrox( ) -> bool{
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                cast_tracker_aatrox( );
                spell_aatrox_Q( );
                break;
            default:
                break;
            }
            return true;
        }

        auto spell_aatrox_Q( ) -> bool{
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

        auto cast_tracker_aatrox( ) -> void{
            if ( !m_cast_active ) return;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot != 0 ) {
                if ( m_cast_detected ) reset_cast_aatrox( );
                return;
            }

            m_cast_detected = true;

            if ( !m_fixed_direction ) {
                m_cast_direction  = ( m_cast_direction - g_local->position ).normalize( );
                m_fixed_direction = true;
            }

            auto& target = g_entity_list->get_by_index( m_cast_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                reset_cast_aatrox( );
                return;
            }

            const auto duration_left = sci->server_cast_time - *g_time;
            auto       hitbox        = get_hitbox_aatrox( m_cast_hitbox, m_cast_direction );

            auto pred = g_features->prediction->predict_default( target->index, duration_left );
            if ( !pred ) return;

            if ( hitbox.is_inside( *pred ) ) return;

            auto       focus_point   = get_focus_point_aatrox( m_cast_hitbox, m_cast_direction );
            const auto distance      = focus_point.dist_to( *pred );
            Vec3       direction     = ( *pred - focus_point ).normalize( );
            const Vec3 cast_position = g_local->position.extend( g_local->position + direction, distance );

            auto dash_time = g_local->position.dist_to( cast_position ) / 805.f;

            if ( dash_time > duration_left + 0.075f ) return;

            adjust_e_aatrox( cast_position );
        }

        auto reset_cast_aatrox( ) -> void{
            m_cast_active       = false;
            m_cast_detected     = false;
            m_fixed_direction   = false;
            m_cast_direction    = Vec3( );
            m_cast_hitbox       = 0;
            m_cast_target_index = 0;
        }

        auto get_hitbox_aatrox( const int index, const Vec3& direction ) -> Polygon{
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

        static auto get_focus_point_aatrox( const int index, const Vec3& direction ) -> Vec3{
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

        auto adjust_e_aatrox( const Vec3& position ) -> bool{
            if ( !g_config->aatrox.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, position ) ) m_last_e_time = *g_time;
        }

        auto Ahri( ) -> bool{
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_ahri_Q( );
                spell_ahri_W( );
                spell_ahri_E( );
                break;
            default:
                break;
            }
            return true;
        }

        auto spell_ahri_Q( ) -> bool{
            if ( !g_config->ahri.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            m_q_range = 900.f;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_ahri( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_ahri( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1550.f, 100.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ahri.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_ahri_W( ) -> bool{
            if ( !g_config->ahri.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                650.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_ahri( ESpellSlot::w, unit ); }
            );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_ahri_E( ) -> bool{
            if ( !g_config->ahri.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            m_e_range = 1000.f;


            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1550.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ahri.e_hitchance->get
                    < int >( ) )
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_ahri( const ESpellSlot slot, Object* target ) -> float{
            m_q_damage = { 0.f, 40.f, 65.f, 90.f, 115.f, 140.f };
            m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        static auto get_spell_travel_time_ahri( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1550.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1550.f;
            }
            case ESpellSlot::w:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto Akali( ) -> bool{
            m_passive_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "AkaliPWeapon" ) );


            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_akali_Q( );
                spell_akali_E( );
                break;
            default:
                break;
            }

            if ( !m_ring_found ) {
                for ( auto object : g_entity_list->get_ally_minions( ) ) {
                    if ( !object || object->is_dead( ) || object->is_invisible( ) || object->dist_to_local( ) > 5000.f
                        || is_ignored_ring_akali( object->network_id ) )
                        continue;

                    std::string name = object->name.text;

                    if ( name.find( _( "TwilightShroud" ) ) == std::string::npos ) continue;

                    if ( object->dist_to_local( ) > 550.f ) {
                        m_ignored_rings.push_back( object->network_id );
                        continue;
                    }

                    m_ring_position   = object->position;
                    m_ring_index      = object->index;
                    m_ring_nid        = object->network_id;
                    m_ring_found      = true;
                    m_ring_start_time = *g_time;
                    break;
                }
            } else {
                auto& obj = g_entity_list->get_by_index( m_ring_index );
                if ( m_passive_active || !obj || obj->is_dead( ) ) {
                    m_ignored_rings.push_back( m_ring_nid );
                    reset_ring_akali( );
                }
            }

            return true;
        }

        auto spell_akali_Q( ) -> bool{
            if ( !g_config->akali.q_enabled->get< bool >( ) ||
                m_passive_active && g_config->akali.passive_block_cast->get< bool >( ) ||
                m_magnet_active && g_config->akali.magnet_block_cast->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            m_q_range = 500.f;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 0.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->akali.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_akali_E( ) -> bool{
            if ( !g_config->akali.e_enabled->get< bool >( )
                || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) )
                return false;

            if ( rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "AkaliE" ) ) {
                if ( !m_slot_e->is_ready( true ) ||
                    m_passive_active && g_config->akali.passive_block_cast->get< bool >( ) ||
                    ( m_magnet_active || *g_time - m_ring_start_time <= 0.2f ) && g_config->akali.magnet_block_cast->get
                    < bool >( ) ||
                    g_config->akali.e_hold_in_full_combo->get< bool >( ) && m_slot_r->is_ready( ) && rt_hash(
                        m_slot_r->get_name().c_str()
                    ) == ct_hash( "AkaliR" ) )
                    return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 825.f ) return false;

                auto pred = g_features->prediction->predict( target->index, m_e_range, 1800.f, 60.f, 0.25f );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->akali.e_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }

                return false;
            }

            Object* target{ };
            for ( auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

                auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "AkaliEMis" ) );
                if ( !buff ) continue;

                target = enemy;
                break;
            }

            if ( !target ) return false;

            auto allow_e{ g_config->akali.e_in_full_combo->get< bool >( ) && GetAsyncKeyState( VK_CONTROL ) };

            if ( !allow_e && g_config->akali.e_on_killable->get< bool >( ) ) {
                auto damage = get_spell_damage( ESpellSlot::e, target );
                if ( m_slot_q->is_ready( true ) ) damage += get_spell_damage( ESpellSlot::q, target );

                if ( damage + helper::get_aa_damage( target->index, true ) < target->health + target->
                    total_health_regen )
                    return false;

                allow_e = true;
            }

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }


            return false;
        }

        auto pass_magnet_akali( ) -> void{
            if ( !m_ring_found || m_passive_active || !g_config->akali.magnet_orbwalk->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                m_magnet_active = false;

                return;
            }

            Vec3       goal_point{ };
            auto       lowest_distance{ 9999.f };
            bool       found_point{ };
            const auto local_under_turret = is_position_in_turret_range( g_local->position );

            m_magnet_points.clear( );
            const auto candidates = get_circle_segment_points_akali( m_ring_position, 500.f, 60 );

            for ( auto& point : candidates.points ) {
                if ( point.dist_to( g_local->position ) > g_config->akali.magnet_distance->get< int >( ) + (
                        m_magnet_active ? 30.f : 0.f ) ||
                    point.dist_to( g_local->position ) > lowest_distance ) {
                    m_magnet_points.push_back( point );
                    continue;
                }

                auto walk_point{ point };

                if ( point.dist_to( g_local->position ) <= 80.f ) {
                    const auto point_dist   = point.dist_to( g_local->position );
                    const auto extend_value = 120.f - point_dist;

                    walk_point = m_ring_position.extend( point, 500.f + extend_value );
                }

                if ( g_navgrid->is_wall( walk_point ) || !local_under_turret && is_position_in_turret_range(
                    walk_point
                ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                goal_point      = walk_point;
                lowest_distance = point.dist_to( g_local->position );
                found_point     = true;

                m_magnet_points.push_back( walk_point );
            }

            if ( !found_point ) {
                // enable orb movement when not magneting
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;

                return;
            }

            if ( g_local->position.dist_to( m_ring_position ) > 500.f ) {
                // std::cout << "early magnet end\n";
                g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                m_ignored_rings.push_back( m_ring_nid );
                reset_ring_akali( );
                return;
            }

            if ( !g_features->orbwalker->is_movement_disabled( ) ) {
                g_features->orbwalker->allow_movement( false );
                g_features->orbwalker->allow_attacks( false );
            }

            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                auto path     = aimgr->get_path( );
                auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( goal_point ) < 5.f ) return;

                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_magnet_active  = true;
                }
            }
        }

        auto is_ignored_ring_akali( const unsigned network_id ) const -> bool{
            for ( const auto& inst : m_ignored_rings ) if ( inst == network_id ) return true;

            return false;
        }

        auto reset_ring_akali( ) -> void{
            m_ring_found      = false;
            m_ring_index      = 0;
            m_ring_nid        = 0;
            m_ring_position   = Vec3( 0, 0, 0 );
            m_ring_start_time = 0.f;
        }

        static auto get_circle_segment_points_akali(
            const Vec3& center,
            const float radius,
            const int   segments
        ) -> sdk::math::Polygon{
            const auto         angle_step = D3DX_PI * 2.0f / segments;
            sdk::math::Polygon poly{ };

            //if ( safe_distance ) radius += g_config->evade.extra_distance->get< int >( ) + m_safe_distance * 0.3f;

            for ( float angle = 0; angle < ( D3DX_PI * 2.0f ); angle += angle_step ) {
                // D3DXVECTOR3 v_start( radius * cosf( angle ) + center.x, radius * sinf( angle ) + center.z, center.y );
                D3DXVECTOR3 v_end(
                    radius * cosf( angle + angle_step ) + center.x,
                    radius * sinf( angle + angle_step ) + center.z,
                    center.y
                );

                const auto temp_z = v_end.z;
                v_end.z           = v_end.y;
                v_end.y           = temp_z;

                poly.points.push_back( { v_end.x, v_end.y, v_end.z } );
            }

            return poly;
        }

        auto Blitz( ) -> bool{
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_blitz_Q( );
                spell_blitz_E( );
                break;
            default:
                break;
            }
            return true;
        }

        auto spell_blitz_Q( ) -> bool{
            if ( !g_config->blitzcrank.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            m_q_range = 1075.f;

            const auto predicted = g_features->prediction->predict( target->index, m_q_range, 1800.f, 80.f, 0.25f );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->blitzcrank.q_hitchance->get<
                    int >( ) ) || g_features->prediction->minion_in_line(
                    g_local->position,
                    predicted.position,
                    100.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_blitz_E( ) -> bool{
            if ( !g_config->blitzcrank.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( g_config->blitzcrank.aa_reset_on_immobile->get< bool >( ) &&
                g_features->buff_cache->is_immobile( target->index )
            )
                if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto Belveth( ) -> bool{
            m_q_range = 400.f;
            m_w_range = 650.f;
            m_e_range = 500.f;

            initialize_spell_slots( );

            m_e_active_belveth = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "BelvethE" ) );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->in_attack( ) ) return false;

            m_flag_belveth = m_slot_q->get_usable_state( ) == 14 ? 0 : m_slot_q->get_usable_state( ) + 1;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_belveth_Q( );
                spell_belveth_E( );
                spell_belveth_W( );
                break;

            default:
                break;
            }
        }

        auto spell_belveth_Q( ) -> bool{
            if ( !g_config->belveth.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                g_config->belveth.q_mode->get< int >( ) == 0 &&
                g_features->orbwalker->is_attackable( target->index ) ||
                g_config->belveth.q_mode->get< int >( ) == 1 &&
                g_features->orbwalker->is_attackable( target->index ) &&
                g_local->mana > 0
            )
                return false;

            if ( m_e_active_belveth && target->dist_to_local( ) < 375.f ) return false;

            const auto raw_pred = g_features->prediction->predict(
                target->index,
                m_q_range + 200.f,
                m_q_speed_belveth[ m_slot_q->level ],
                0.f,
                0.f
            );
            if ( !raw_pred.valid ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range + 200.f,
                m_q_speed_belveth[ m_slot_q->level ],
                100.f,
                0.f
            );
            if ( !pred.valid ) return false;

            const auto dash_position = g_local->position.extend( pred.position, 400.f );
            if ( !can_dash_belveth( dash_position ) ||
                is_wall_blocking_belveth( pred.position ) ||
                is_position_in_turret_range( dash_position ) &&
                !is_position_in_turret_range( g_local->position )
            )
                return false;

            switch ( g_config->belveth.q_mode->get< int >( ) ) {
            case 0:
                if ( dash_position.dist_to( raw_pred.position ) > g_local->attack_range + 100.f ) return false;
                break;
            case 1:
                if ( !g_features->orbwalker->is_attackable( target->index ) &&
                    dash_position.dist_to( raw_pred.position ) > g_local->attack_range + 100.f ||
                    g_features->orbwalker->is_attackable( target->index ) &&
                    dash_position.dist_to( raw_pred.position ) > g_local->attack_range
                )
                    return false;
                break;
            default:
                return false;
            }

            if ( cast_spell( ESpellSlot::q, dash_position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_belveth_W( ) -> bool{
            if ( !g_config->belveth.w_enabled->get< bool >( ) || m_e_active_belveth || *g_time - m_last_w_time <= 0.4f
                || !m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                g_config->belveth.w_only_out_of_range->get< bool >( ) &&
                g_features->orbwalker->is_attackable( target->index )
            )
                return false;

            auto pred = g_features->prediction->predict( target->index, 650.f, 0.f, 110.f, 0.5f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->belveth.w_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.4f );

                return true;
            }

            return false;
        }

        auto spell_belveth_E( ) -> bool{
            if ( !g_config->belveth.e_enabled->get< bool >( ) || m_e_active_belveth || *g_time - m_last_e_time <= 0.4f
                || !m_slot_e->is_ready( ) ||
                g_config->belveth.e_disable_under_turret->get< bool >( ) && is_position_in_turret_range(
                    g_local->position
                ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                target->dist_to_local( ) > 350.f ||
                target->health / target->max_health * 100.f > g_config->belveth.e_min_health_percent->get< int >( )
            )
                return false;

            auto should_cast{ true };
            for ( const auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > 475.f || !
                    minion->is_normal_minion( ) )
                    continue;

                if ( minion->health< target->health ) {
                        should_cast = false;
                    break;


                    
                    
                    }


            
            }

            if ( !should_cast ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( *pred ) > 210.f ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        static auto is_wall_blocking_belveth( const Vec3& end_position ) -> bool{
            const auto max_distance = g_local->position.dist_to( end_position );

            for ( auto i = 1; i < 40; i++ ) {
                const auto distance = i * 10.f;
                if ( distance >= max_distance ) break;

                if ( g_navgrid->is_wall( g_local->position.extend( end_position, distance ) ) ) return true;
            }

            return false;
        }

        auto can_dash_belveth( const Vec3& position ) const -> bool{
            const auto value{ 10.f };
            const auto north_east{ g_local->position + Vec3( value, 0, value ) };
            const auto north_west{ g_local->position + Vec3( -value, 0, value ) };
            const auto south_east{ g_local->position + Vec3( value, 0, -value ) };
            const auto south_west{ g_local->position + Vec3( -value, 0, -value ) };

            const auto dash_direction = g_local->position.extend( position, 10.f );


            auto lowest_distance{ 999.f };
            int  number{ };

            for ( auto i = 0; i < 4; i++ ) {
                float dist{ };
                switch ( i ) {
                case 0:
                    dist = dash_direction.dist_to( north_east );
                    break;
                case 1:
                    dist = dash_direction.dist_to( north_west );
                    break;
                case 2:
                    dist = dash_direction.dist_to( south_west );
                    break;
                case 3:
                    dist = dash_direction.dist_to( south_east );
                    break;
                default:
                    return false;
                }

                if ( dist < lowest_distance ) {
                    number          = i;
                    lowest_distance = dist;
                }
            }

            if ( m_flag_belveth == 0 ) return true;

            switch ( number ) {
            case 0:
                return m_flag_belveth & 2;
            case 1:
                return m_flag_belveth & 1;
            case 2:
                return m_flag_belveth & 8;
            case 3:
                return m_flag_belveth & 4;
            default:
                break;
            }

            // North west = 1 |
            // North east = 2 |
            // South east = 4 |
            // South west = 8 |

            return false;
        }

        auto Brand( ) -> bool{
            m_q_range = 1040.f;
            m_w_range = 900.f;
            m_e_range = 675.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_brand_E( );
                spell_brand_W( );
                spell_brand_Q( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_brand_Q( ) -> bool{
            if ( !g_config->brand.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->brand.q_only_ablaze->get< bool >( ) && !g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "BrandAblaze" )
            ) )
                return false;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, 1040.f, 1600.f, 80.f, 0.25f );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->brand.q_hitchance->
                    get< int >( ) ) && !will_kill
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 80.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_brand_W( ) -> bool{
            if ( !g_config->brand.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_w->is_ready( true ) )
                return false;

            if ( g_config->brand.w_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_w_range, 0.f, 250.f, 1.f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::w, multihit.position ) ) {
                        g_features->orbwalker->on_cast( );
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 250.f, 1.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->brand.w_hitchance->
                get< int >( ) ) && !will_kill )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_brand_E( ) -> bool{
            if ( !g_config->brand.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_e_range ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto Caitlyn( ) -> bool{
            m_q_range = 1240.f;
            m_w_range = 800.f;
            m_e_range = 740.f;

            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            antigapcloser_e_cait( );
            autointerrupt_w_cait( );

            if ( g_features->orbwalker->in_attack( ) ) return false;

            animation_cancel_w_cait( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                antimelee_w_cait( );

                spell_e( );
                spell_w( );
                spell_q( );
                break;

            default:
                break;
            }

            return false;
        }

        auto spell_caitlyn_Q( ) -> bool{
            if ( !g_config->caitlyn.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->caitlyn.q_only_out_of_range->get< bool >( ) && g_features->orbwalker->
                is_attackable( target->index ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 2200.f, 80.f, 0.625f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->caitlyn.q_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return true;
        }

        auto spell_caitlyn_W( ) -> bool{
            if ( !g_config->caitlyn.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 1.25f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->buff_cache->get_buff( target->index, ct_hash( "CaitlynWSnare" ) )
                || g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 50.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->caitlyn.w_hitchance->get< int >( ) ) return false;

            if ( g_config->caitlyn.w_save_charge_for_cc->get< bool >( ) && ( int )pred.hitchance <= 3 && m_slot_w->
                charges < 2 )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time          = *g_time;
                m_last_cast_time       = *g_time;
                m_last_w_position_cait = pred.position;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_caitlyn_E( ) -> bool{
            if ( !g_config->caitlyn.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range / 100.f * g_config->caitlyn.e_max_range->get< int >( ),
                1600.f,
                70.f,
                0.15f
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->caitlyn.e_hitchance->
                    get< int >( ) )
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 100.f ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto antigapcloser_e_cait( ) -> void{
            if ( !g_config->caitlyn.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1600.f, 70.f, 0.15f );
            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                80.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto antimelee_w_cait( ) -> void{
            if ( !g_config->caitlyn.w_antimelee->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || *g_time -
                m_last_antimelee_time <= 1.3f || *g_time - m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->attack_range > 375.f || g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "CaitlynWSnare" )
            ) )
                return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.f ) return;

            if ( target->dist_to_local( ) > 275.f ) return;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred || g_local->position.dist_to( *pred ) > target->dist_to_local( ) ) return;

            if ( cast_spell( ESpellSlot::w, g_local->position ) ) {
                m_last_w_time              = *g_time;
                m_last_cast_time           = *g_time;
                m_last_antimelee_time_cait = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto autointerrupt_w_cait( ) -> void{
            if ( !g_config->caitlyn.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 1.3f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target || g_features->buff_cache->get_buff( target->index, ct_hash( "CaitlynWSnare" ) ) ) return;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "caitlynwsight" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.f ) return;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 50.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto animation_cancel_w_cait( ) -> void{
            if ( !g_config->caitlyn.w_animation_cancel->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || !m_slot_w->
                is_ready( true ) )
                return;

            if ( !m_active_cait ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 ) return;

                m_active_cait    = true;
                m_cast_time_cait = sci->server_cast_time;
                return;
            }

            const auto time_left = m_cast_time_cait - *g_time;
            if ( time_left > g_features->orbwalker->get_ping( ) * 0.5f ) {
                if ( *g_time > m_cast_time_cait + 0.075f ) {
                    m_active_cait    = false;
                    m_cast_time_cait = 0.f;
                }

                return;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 50.f, 1.f );
            if ( !pred.valid ) return;

            if ( g_config->caitlyn.w_save_charge_for_cc->get< bool >( ) && ( int )pred.hitchance < 3 && m_slot_w->
                charges < 2 )
                return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_active_cait    = false;
                m_cast_time_cait = 0.f;
                g_features->orbwalker->on_cast( );
            }
        }

        auto Cassiopeia( ) -> bool{
            m_q_range = 850.f;
            m_w_range = 700.f;
            m_e_range = 700.f;

            antigapclose_w_cass( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_cass_Q( );
                spell_cass_E( );
                spell_cass_W( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_cass_Q( ) -> bool{
            if ( !g_config->cassiopeia.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            if ( g_config->cassiopeia.q_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_q_range, 0.f, 200.f, 0.8f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        m_last_q_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;


            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiaqdebuff" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 1.5f ||
                g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiawpoison" ) ) )
                return false;


            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 190.f, 0.8f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->cassiopeia.
                q_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_cass_W( ) -> bool{
            if ( !g_config->cassiopeia.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiaqdebuff" ) ) || m_slot_q->
                is_ready( true ) || *g_time - m_last_q_time < 1.f )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 3000.f, 0.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->cassiopeia.
                    w_hitchance->get< int >( ) )
                || pred.position.dist_to( g_local->position ) > m_w_range * ( g_config->cassiopeia.w_max_range->get<
                    int >( ) / 100.f ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_cass_E( ) -> bool{
            if ( !g_config->cassiopeia.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 700.f ) return false;

            if ( g_config->cassiopeia.e_only_poisoned->get< bool >( ) && !g_features->buff_cache->get_buff(
                    target->index,
                    ct_hash( "cassiopeiaqdebuff" )
                )
                && !g_features->buff_cache->get_buff( target->index, ct_hash( "cassiopeiawpoison" ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto antigapclose_w_cass( ) -> void{
            if ( !g_config->cassiopeia.w_antigapclose->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return;

            const Vec3 path_end = path[ 1 ];

            if ( cast_spell( ESpellSlot::w, path_end ) ) m_last_w_time = *g_time;
        }

        auto Chogath( ) -> bool{
            m_q_range = 950.f;
            m_w_range = 650.f;
            m_e_range = 0.f;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->in_attack( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_w( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_cho_Q( ) -> bool{
            if ( !g_config->chogath.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 200.f, 1.2f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->chogath.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_cho_W( ) -> bool{
            if ( !g_config->chogath.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.5f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->chogath.w_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_cho_E( ) -> bool{
            if ( !g_config->chogath.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_slot_e->charges >
                0 || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Corki( ) -> bool{
            m_q_range = 825.f;
            m_e_range = 690.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_corki_Q( );
                spell_corki_E( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_corki_Q( ) -> bool{
            if ( !g_config->corki.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            if ( g_config->corki.q_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_q_range, 1000.f, 135.f, 0.25f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        m_last_q_time = *g_time;
                        g_features->orbwalker->set_cast_time( 0.25f );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1000.f, 130.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->corki.q_hitchance->get< int >( ) ) &&
                !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
                )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_corki_E( ) -> bool{
            if ( !g_config->corki.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) || !g_features->orbwalker->should_reset_aa( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Darius( ) -> bool{
            m_q_range = 400.f;
            m_e_range = 600.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            magnet_q_darius( );

            m_q_active_darius = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "dariusqcast" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_darius_E( );
                spell_darius_Q( );
                spell_darius_W( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_darius_Q( ) -> bool{
            if ( !g_config->darius.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.5f || *g_time -
                m_last_e_time <= 0.5f || !m_slot_q->is_ready( true )
                || m_q_active_darius
                || g_config->darius.prefer_e_over_q->get< bool >( ) && g_config->darius.e_enabled->get< bool >( )
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                && GetAsyncKeyState( VK_CONTROL ) && m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 475.f ||
                g_config->darius.q_aa_range_limit->get< bool >( ) && g_features->orbwalker->is_attackable(
                    target->index
                ) )
                return false;

            const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.1f );
            if ( !local_pred ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred || g_local->position.dist_to( *pred ) > target->dist_to_local( )
                && local_pred.value( ).dist_to( *pred ) > target->dist_to_local( ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time   = *g_time;
                m_magnet_target = target->index;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_darius_W( ) -> bool{
            if ( !g_config->darius.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || m_q_active_darius ||
                !m_slot_w->is_ready( true ) )
                return false;

            if ( g_config->darius.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) ) {
                return
                    false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 25.f ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_darius_E( ) -> bool{
            if ( !g_config->darius.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_q_active_darius ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->darius.e_aa_range_limit->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, 575.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid ) return false;

            auto cast_position{ pred.position };
            auto allow_e = static_cast< int >( pred.hitchance ) >= g_config->darius.e_hitchance->get< int >( );

            if ( !allow_e && g_config->darius.e_hitchance->get< int >( ) <= 3
                /* && pred.position != target->position*/ ) {
                const auto sect      = sdk::math::Sector( g_local->position, target->position, 50.f, 575.f );
                const auto pred_sect = sdk::math::Sector( g_local->position, pred.position, 50.f, 575.f );

                auto base_poly = sect.to_polygon_new( );
                auto pred_poly = pred_sect.to_polygon_new( );

                allow_e = pred_poly.is_inside( target->position ) && pred_poly.is_inside( pred.position );

                if ( !allow_e && base_poly.is_inside( target->position ) && base_poly.is_inside( pred.position ) ) {
                    allow_e       = true;
                    cast_position = target->position;
                }
            }

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto magnet_q_darius( ) -> void{
            if ( !g_config->darius.q_magnet->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "dariusqcast" ) );
            if ( !buff ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                return;
            }

            Object* target;
            auto&   obj = g_entity_list->get_by_index( m_magnet_target );
            if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->dist_to_local( ) > 500.f ) {
                target = g_features->target_selector->get_default_target( );
                if ( !target ) return;
            } else target = obj.get( );

            const auto time_left = buff->buff_data->end_time - *g_time;

            const auto pred = g_features->prediction->predict_default( target->index, time_left );
            if ( !pred ) return;

            const auto target_position{ *pred };

            const auto distance = g_local->position.dist_to( target_position );
            const auto prioritize_outer{ distance > 350.f };
            const auto point_distance = prioritize_outer ? 380.f : 250.f;

            const auto candidates = g_features->evade->get_circle_segment_points(
                target_position,
                point_distance,
                60
            ).points;

            m_magnet_points.clear( );
            const auto is_local_under_turret{ is_position_in_turret_range( ) };
            const auto max_distance    = ( time_left > 0.2f ? time_left - 0.1f : time_left ) * g_local->movement_speed;
            auto       cursor          = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto cursor_position = g_local->position.extend( cursor, max_distance );

            Vec3 goal_point{ };
            auto lowest_distance{ 999999.f };
            bool found_point{ };

            for ( auto point : candidates ) {
                auto walk_point{ point };

                /*if (point.dist_to(g_local->position) <= 80.f) {
                    auto point_dist = point.dist_to(g_local->position);
                    auto extend_value = 100.f - point_dist;

                    walk_point = target_position.extend(point, target_position.dist_to( point ) + extend_value);
                }*/

                if ( g_navgrid->is_wall( walk_point ) || g_local->position.dist_to( walk_point ) > max_distance
                    || !is_local_under_turret && is_position_in_turret_range( walk_point ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                if ( walk_point.dist_to( cursor_position ) > lowest_distance ) continue;

                goal_point      = walk_point;
                lowest_distance = point.dist_to( cursor_position );
                found_point     = true;

                m_magnet_points.push_back( walk_point );
            }

            if ( !found_point ) {
                // enable orb movement when not magneting
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;

                return;
            }


            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                g_features->orbwalker->allow_movement( false );
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                auto path     = aimgr->get_path( );
                auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( goal_point ) < 5.f ) return;

                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_magnet_active  = true;
                }
            }
        }

        auto Draven( ) -> bool{
            m_q_range = 0.f;
            m_w_range = 0.f;
            m_e_range = 1100.f;
            initialize_spell_slots( );

            update_axes_draven( );
            update_axe_count( );
            update_attack_draven( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            axe_catcher_draven( );
            antigapclose_e_draven( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_draven_W( );
                spell_draven_Q( );

                if ( g_features->orbwalker->in_attack( ) ) return false;

                spell_draven_E( );
                break;
            default:
                break;
            }
            return false;

            // DravenSpinningAttack | also AA missile name
            // DravenFury
            // dravenfurybuff
        }

        auto spell_draven_Q( ) -> bool{
            if ( !g_config->draven.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            bool allow_q{ };
            if ( g_config->draven.q_keep_buff->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
                if ( buff && buff->buff_data->end_time - *g_time <= 0.1f ) allow_q = true;
            }

            if ( !allow_q && g_features->orbwalker->in_attack( ) && m_axe_count < 2 ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci ) return false;

                auto& target = g_entity_list->get_by_index( sci->get_target_index( ) );
                if ( !target || !target->is_hero( ) ) return false;

                allow_q = true;
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_draven_W( ) -> bool{
            if ( !g_config->draven.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                    true
                )
                || g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenFury" ) ) )
                return false;

            bool allow_w{ };
            if ( g_config->draven.w_attackspeed->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "dravenfurybuff" ) );
                if ( !buff || buff->buff_data->end_time - *g_time <= 0.1f ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( target && g_features->orbwalker->
                                               is_attackable(
                                                   target->index,
                                                   g_local->attack_range + 25.f
                                               ) )
                        allow_w = true;
                }
            }

            if ( !allow_w && g_config->draven.w_on_slow->get< bool >( ) ) {
                allow_w = g_features->buff_cache->has_buff_type(
                    g_local->index,
                    { EBuffType::slow, EBuffType::attack_speed_slow },
                    0.2f
                );
            }

            if ( !allow_w && g_config->draven.w_chase->get< bool >( ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 900.f ) return false;

                const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.2f );
                const auto pred       = g_features->prediction->predict_default( target->index, 0.2f );
                if ( !pred || !local_pred ) return false;

                allow_w = g_local->position.dist_to( target->position ) < g_local->position.dist_to( *pred ) && target->
                    dist_to_local( ) > target->position.dist_to( *local_pred );
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_draven_E( ) -> bool{
            if ( !g_config->draven.e_anti_melee->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 275.f ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 130.f, 0.25f );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;

            return false;
        }

        auto update_axes_draven( ) -> void{
            for ( auto missile : g_entity_list->get_ally_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 3000.f || is_axe_logged( missile->index )
                    || *g_time - missile->missile_spawn_time( ) >= 1.3f )
                    continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto name = data->get_name( );
                if ( name.find( "DravenSpinningReturn" ) == std::string::npos ||
                    name.find( "DravenSpinningReturnCatch" ) != std::string::npos )
                    continue;

                m_axes.push_back( missile->index );
            }

            for ( auto& index : m_axes ) {
                auto& missile = g_entity_list->get_by_index( index );
                if ( !missile || *g_time - missile->missile_spawn_time( ) > 1.3f ) {
                    remove_axe( index );
                    continue;
                }
            }
        }

        auto antigapclose_e_draven( ) -> void{
            if ( !g_config->draven.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 130.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto axe_catcher_draven( ) -> void{
            if ( !g_config->draven.q_catch_axes->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                return;
            }

            Vec3 catch_position{ };
            auto axe_land_time{ 999.f };
            bool found_axe{ };

            Vec3 cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            for ( auto& index : m_axes ) {
                auto& axe = g_entity_list->get_by_index( index );
                if ( !axe ) {
                    remove_axe( index );
                    continue;
                }

                auto axe_position = axe->missile_end_position;
                if ( axe_position.dist_to( cursor ) > g_config->draven.catch_axe_range->get< int >( ) ||
                    false && axe_position.dist_to( g_local->position ) > 350.f )
                    continue;

                float time_to_catch = 1.25f - ( *g_time - axe->missile_spawn_time( ) );
                float time_to_axe   = ( g_local->position.dist_to( axe_position ) - 150.f ) / g_local->movement_speed;

                if ( time_to_axe > time_to_catch ||
                    found_axe && time_to_catch > axe_land_time )
                    continue;

                catch_position = axe_position;
                axe_land_time  = time_to_catch;
                found_axe      = true;
            }

            if ( !found_axe ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) {
                    g_features->orbwalker->allow_movement( true );
                    g_features->orbwalker->allow_fast_move( );
                }

                g_features->orbwalker->allow_attacks( true );
                return;
            }

            const auto distance       = g_local->position.dist_to( catch_position );
            const auto free_move_time = distance <= 110.f ? ( 130.f - distance ) / g_local->movement_speed : 0.f;

            if ( axe_land_time <= 0.05f ) return;

            // local inside axe radius
            if ( distance <= 110.f /* || free_move_time > axe_land_time*/ ) {
                if ( g_features->orbwalker->is_movement_disabled( ) && ( free_move_time >= axe_land_time || distance <=
                    50.f ) ) {
                    g_features->orbwalker->allow_movement( true );
                    g_features->orbwalker->allow_fast_move( );
                }

                if ( g_config->draven.catch_mode->get< int >( ) == 1 && m_will_impact && ( !m_moved_for_impact || *
                    g_time - m_last_move_time >= 0.03f ) ) {
                    g_features->orbwalker->allow_attacks( false );
                    g_features->orbwalker->allow_movement( false );

                    const auto move_position = g_local->position.extend( cursor, 100.f );
                    if ( g_features->orbwalker->send_move_input( move_position, true ) ) {
                        m_last_move_time   = *g_time;
                        m_moved_for_impact = true;
                        // std::cout << "move to redirect #1\n";
                    }

                    return;
                }

                if ( distance <= 95.f ) g_features->orbwalker->allow_attacks( true );

                if ( ( !m_missile_found || m_time_to_impact > 0.125f ) &&
                    g_features->orbwalker->is_movement_disabled( ) && g_features->orbwalker->should_reset_aa( ) &&
                    distance <= 75.f ) {
                    const auto pred = g_features->prediction->predict_default( g_local->index, 0.1f );
                    if ( !pred || g_local->position.dist_to( *pred ) > 1.f ) return;

                    const auto move_position = g_local->position.dist_to( catch_position ) <= 65.f
                                                   ? g_local->position.extend( catch_position, 100.f )
                                                   : catch_position;

                    if ( ( !g_config->draven.q_slow_order->get< bool >( ) || *g_time - m_last_move_time > 0.05f ) &&
                        g_features->orbwalker->send_move_input( move_position, true ) ) {
                        m_last_move_time = *g_time;
                        g_features->orbwalker->allow_movement( true );
                        // std::cout << "move after attack: " << distance << "\n";
                    }
                }

                /* if (!m_will_impact && g_features->orbwalker->get_mode() == c_orbwalker::e_orbwalker_mode::combo)
                 {
                     if (axe_land_time < g_features->orbwalker->get_attack_cast_delay() ||
                         axe_land_time > g_features->orbwalker->get_attack_cast_delay() + 0.075f) g_features->orbwalker->allow_attacks(true);
                     else
                     {
                         std::cout << "delaying attack stack by: " << axe_land_time - g_features->orbwalker->get_attack_cast_delay() << "\n";
                         g_features->orbwalker->allow_attacks(false);
                     }
                 }*/

                return;
            }

            g_features->orbwalker->allow_movement( false );

            const auto nearest_point = catch_position.extend( g_local->position, 140.f );
            const auto time_to_axe   = nearest_point.dist_to( g_local->position ) / g_local->movement_speed;
            const auto extra_time    = time_to_axe < axe_land_time ? time_to_axe - axe_land_time : 0.f;

            g_features->orbwalker->allow_attacks(
                g_config->draven.q_prefer_dps->get< bool >( ) || extra_time > g_features->orbwalker->
                get_attack_cast_delay( )
            );

            // send movement order to axe position
            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                auto path     = aimgr->get_path( );
                auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( catch_position ) < 10.f ) return;

                if ( g_features->orbwalker->send_move_input( catch_position, true ) ) {
                    m_last_move_time = *g_time;
                    //std::cout << "catcher issueorder: " << distance << "\n";
                }
            }
        }

        auto update_axe_count( ) -> void{
            int        axe_count{ };
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
            if ( buff ) axe_count = buff->stacks( );
            axe_count += m_axes.size( );
            m_axe_count = axe_count;
        }

        auto update_axes( ) -> void{
            for ( auto missile : g_entity_list->get_ally_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 3000.f || is_axe_logged( missile->index )
                    || *g_time - missile->missile_spawn_time( ) >= 1.3f )
                    continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto name = data->get_name( );
                if ( name.find( "DravenSpinningReturn" ) == std::string::npos ||
                    name.find( "DravenSpinningReturnCatch" ) != std::string::npos )
                    continue;

                m_axes.push_back( missile->index );
            }

            for ( auto& index : m_axes ) {
                auto& missile = g_entity_list->get_by_index( index );
                if ( !missile || *g_time - missile->missile_spawn_time( ) > 1.3f ) {
                    remove_axe( index );
                    continue;
                }
            }
        }

        auto update_attack_draven( ) -> void{
            if ( !m_attack_active ) {
                if ( !g_features->orbwalker->in_attack( ) ) return;

                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
                if ( !buff ) return;

                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci ) return;

                m_attack_active    = true;
                m_attack_time      = sci->start_time;
                m_server_cast_time = sci->server_cast_time;
                m_missile_nid      = sci->missile_nid;
                m_target_index     = sci->get_target_index( );
                return;
            }

            auto& missile = g_entity_list->get_by_network_id( m_missile_nid );
            if ( !missile ) {
                if ( m_missile_found || *g_time > m_server_cast_time && *g_time - m_server_cast_time > g_features->
                    orbwalker->get_attack_cast_delay( ) * 2.f )
                    reset_attack( );

                return;
            }

            m_missile_found = true;
            auto& target    = g_entity_list->get_by_index( m_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                reset_attack( );
                return;
            }

            missile.update( );
            target.update( );

            const auto time_to_collision = missile->position.dist_to( target->position ) / 1700.f;
            m_time_to_impact             = time_to_collision;

            if ( !m_will_impact && time_to_collision <= 0.08f ) {
                // std::cout << "will impact!\n";
                m_will_impact = true;
            }
        }

        auto reset_attack( ) -> void{
            m_attack_active    = false;
            m_missile_found    = false;
            m_will_impact      = false;
            m_moved_for_impact = false;
            m_attack_time      = 0.f;
            m_server_cast_time = 0.f;
            m_time_to_impact   = 0.f;
            m_target_index     = 0;
            m_missile_nid      = 0;
        }

        auto is_axe_logged( const int16_t index ) const -> bool{
            for ( auto& handle : m_axes ) if ( handle == index ) return true;

            return false;
        }

        auto remove_axe( const int16_t index ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_axes,
                [&]( const int16_t& axe_index ) -> bool{ return axe_index == index; }
            );

            if ( to_remove.empty( ) ) return;

            m_axes.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto Ezreal( ) -> bool{
            m_q_range = 1150.f;
            m_w_range = 1150.f;
            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_ezreal_W( );
                spell_ezreal_Q( );

                break;
            default: ;
            }
        }

        auto spell_ezreal_Q( ) -> bool{
            if ( !g_config->ezreal.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            //const auto target = g_features->target_selector->get_spell_specific_target(m_q_range,[this]( c_object* unit ) -> float{return get_spell_travel_time( e_spell_slot::q, unit );},[this]( c_object* unit ) -> float{return get_spell_damage( e_spell_slot::q, unit );});
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto q_damage = get_spell_damage( ESpellSlot::q, target );

            // prefer w over q
            if ( g_config->ezreal.w_preferred->get< bool >( ) && m_slot_w->is_ready( true ) && target->health > q_damage
                && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo )
                return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                60.f,
                0.25f,
                { },
                true
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->ezreal.q_hitchance->get<
                    int >( ) )
            )
                return false;

            if ( g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f ) ) return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );

                // save path
                auto aimgr = target->get_ai_manager( );
                if ( aimgr ) {
                    auto path = aimgr->get_path( );

                    if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) return true;

                    m_target_current_position = target->position;
                    m_next_path_node          = aimgr->next_path_node;
                    m_cast_path               = path;
                    m_cast_time               = *g_time;
                    m_cast_pos                = predicted.position;
                }

                return true;
            }

            return false;
        }

        auto spell_ezreal_W( ) -> bool{
            if ( !g_config->ezreal.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto will_kill = get_spell_damage( ESpellSlot::q, target ) > target->health + target->
                total_health_regen;
            if ( !get_slot_q( )->is_ready( true ) &&
                !g_features->orbwalker->is_attackable( target->index ) ||
                get_slot_q( )->is_ready( true ) &&
                !g_features->prediction->minion_in_line( g_local->position, target->position, 60 ) &&
                will_kill
            )
                return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                1700.f,
                70.f,
                0.25f,
                { },
                true
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->ezreal.w_hitchance->get<
                    int >( ) )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto Gnar( ) -> bool{
            m_q_range = 1125.f;
            m_w_range = 575.f;
            m_e_range = 475.f;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->in_attack( ) ) return false;

            m_big_gnar = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "gnartransform" ) ) || !!
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "gnartransformsoon" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_gnar_q( );
                spell_gnar_w( );
                spell_gnar_e( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_gnar_q( ) -> bool{
            if ( !g_config->gnar.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( m_big_gnar ) {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2100.f, 80.f, 0.5, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.q_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 90.f ) )
                    return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            } else {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2500.f, 55.f, 0.25, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.q_hitchance->get< int >( ) ||
                    g_features->prediction->minion_in_line( g_local->position, pred.position, 55.f )
                )
                    return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_gnar_w( ) -> bool{
            if ( !g_config->gnar.w_enabled->get< bool >( ) || !m_big_gnar || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 90.f, 0.6f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.w_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_gnar_e( ) -> bool{
            if ( !g_config->gnar.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->gnar.e_disable_aa_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            if ( m_big_gnar ) {
                const auto pred = g_features->prediction->predict( target->index, m_e_range, 800.f, 100.f, 0.f );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->gnar.e_hitchance
                    ->get< int >( ) ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            } else {
                const auto pred = g_features->prediction->predict( target->index, m_e_range, 900.f, 10.f, 0.f );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->gnar.e_hitchance
                    ->get< int >( ) ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto Illaoi( ) -> bool{
            m_q_range = 750.f;
            m_w_range = 0.f;
            m_e_range = 950.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_illaoi_E( );
                spell_illaoi_Q( );
                spell_illaoi_W( );
                spell_r( );
                break;

            default: ;
            }
            return false;
        }

        auto spell_illaoi_Q( ) -> bool{
            if ( !g_config->illaoi.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 100.f, 0.75f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->illaoi.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_illaoi_W( ) -> bool{
            if ( !g_config->illaoi.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            if ( g_config->illaoi.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) ) {
                return
                    false;
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 225.f ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_illaoi_E( ) -> bool{
            if ( !g_config->illaoi.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 1900.f, 50.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->illaoi.e_hitchance->get< int >( )
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 90.f ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Irelia( ) -> bool{
            m_q_range = 600.f;
            m_w_range = 0.f;
            m_e_range = 850.f;

            m_underturret_q = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::combo;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_irelia_E( );
                spell_irelia_Q( );
                q_gapclose_irelia( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
            case Orbwalker::EOrbwalkerMode::laneclear:
                q_lasthit_irelia( );
                break;
            default:
                break;
            }

            // ireliamark | Unsteady buff
            // ireliapassivestacks


            bool found{ };

            if ( rt_hash( m_slot_e->get_name().c_str() ) != ct_hash( "IreliaE" ) ) {
                for ( auto obj : g_entity_list->get_ally_minions( ) ) {
                    if ( !obj || obj->is_dead( ) || rt_hash( obj->name.text ) != ct_hash( "Blade" ) ) continue;

                    m_e_index = obj->index;
                    found     = true;
                    break;
                }
            }

            if ( !found ) m_e_index = 0;

            return false;
        }

        auto spell_irelia_Q( ) -> bool{
            if ( !g_config->irelia.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return false;

            bool allow_q{ };
            if ( g_features->buff_cache->get_buff( target->index, ct_hash( "ireliamark" ) )
                || target->health < get_spell_damage( ESpellSlot::q, target ) )
                allow_q = true;
            else if ( g_config->irelia.q_prekill->get< bool >( ) ) {
                const auto damage    = get_spell_damage( ESpellSlot::q, target );
                const auto aa_damage = helper::get_aa_damage( target->index );

                allow_q = !g_features->orbwalker->is_attackable( target->index )
                              ? target->health - damage <= aa_damage * 1.9f
                              : target->health < damage;
            }

            if ( !allow_q || !m_underturret_q && is_position_in_turret_range( target->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_irelia_E( ) -> bool{
            if ( !g_config->irelia.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.2f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            Vec3 cast_position{ };
            bool allow_cast{ };

            if ( rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "IreliaE" ) ) {
                if ( g_config->irelia.e_only_recast->get< bool >( ) || !m_slot_e->is_ready( true )
                    || target->dist_to_local( ) > m_e_range )
                    return false;

                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return false;

                if ( aimgr->is_moving && aimgr->is_dashing ) {
                    auto path = aimgr->get_path( );
                    if ( path.size( ) == 2 && path[ path.size( ) - 1 ].dist_to( g_local->position ) < m_e_range ) {
                        cast_position = path[ path.size( ) - 1 ];
                        allow_cast    = true;

                        //std::cout << "hide e in dash end\n";
                    }
                }

                if ( !allow_cast ) {
                    if ( g_config->irelia.e_hide->get< bool >( ) ) {
                        const auto pred = g_features->prediction->predict_default( g_local->index, 0.1f );
                        cast_position   = pred.has_value( ) ? *pred : g_local->position;
                    } else cast_position = g_local->position.extend( target->position, target->dist_to_local( ) / 2.f );
                }

                if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }

                return false;
            }

            if ( m_e_index == 0 ) return false;

            auto& blade = g_entity_list->get_by_index( m_e_index );
            if ( !blade ) return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range + 50.f, 2000.f, 40.f, 0.265f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->irelia.e_hitchance->get<
                int >( ) )
                return false;

            for ( auto i = 1; i <= 8; i++ ) {
                Vec3 possible_position = blade->position.extend(
                    pred.position,
                    blade->position.dist_to( pred.position ) + 15.f * i
                );
                if ( possible_position.dist_to( g_local->position ) > m_e_range ) break;

                cast_position = possible_position;
            }

            if ( cast_position.length( ) == 0.f ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                m_last_e_time = *g_time;
                return true;
            }


            return false;
        }

        auto q_lasthit_irelia( ) -> void{
            if ( !GetAsyncKeyState( VK_CONTROL ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto          lasthit_data = get_targetable_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); }
            );
            if ( !lasthit_data ) return;

            auto& target = g_entity_list->get_by_index( lasthit_data->index );
            if ( target && !m_underturret_q && is_position_in_turret_range( target->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return;

            if ( cast_spell( ESpellSlot::q, lasthit_data->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->ignore_minion( lasthit_data->index, 1.f );
            }

            return;
        }

        auto q_gapclose_irelia( ) -> void{
            if ( *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) ) return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto    lowest_distance{ std::numeric_limits< float >::max( ) };
            Object* target_minion{ };

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "ireliapassivestacks" ) );
            const auto is_passive_stacked{ buff && buff->stacks( ) == 4 };

            for ( auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_q_range
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                auto minion_distance = minion->position.dist_to( target->position );
                if ( minion_distance > target->dist_to_local( ) ) continue;

                auto damage = get_spell_damage( ESpellSlot::q, minion );
                if ( damage < minion->health && !g_features->buff_cache->get_buff(
                    minion->index,
                    ct_hash( "ireliamark" )
                ) )
                    continue;

                if ( minion_distance < lowest_distance && ( !is_passive_stacked || minion_distance + 125.f < target->
                    dist_to_local( ) ) ) {
                    target_minion   = minion;
                    lowest_distance = minion_distance;
                }
            }

            if ( !target_minion || !m_underturret_q && is_position_in_turret_range( target_minion->position ) && !
                is_position_in_turret_range( g_local->position ) )
                return;

            if ( cast_spell( ESpellSlot::q, target_minion->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
            }

            return;
        }

        auto Jax( ) -> bool{
            m_q_range = 700.f;
            m_e_range = 300.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                m_w_range  = g_local->attack_range + 65.f + 50.f;
                m_w_active = false;
            } else m_w_active = true;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxCounterStrike" ) ) ) m_e_active = true;
            else m_e_active                                                                                     = false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( spell_jax_E( ) ) return true;
                if ( spell_jax_W( ) ) return true;
                if ( spell_jax_Q( ) ) return true;
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->jax.w_laneclear->get< bool >( ) ) spell_w( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_jax_Q( ) -> bool{
            if ( !g_config->jax.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( target->position.dist_to( g_local->position ) >= m_q_range ||
                g_config->jax.q_only_out_of_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                    target->index
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.05f );
                return true;
            }

            return false;
        }

        auto spell_jax_W( ) -> bool{
            if ( !g_config->jax.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) || m_w_active )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable(
                    target->index,
                    m_w_range
                ) )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_jax_E( ) -> bool{
            if ( !g_config->jax.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_e_range + 25.f ) return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxCounterStrike" ) ) ) {
                if ( !g_config->jax.e_recast->get< bool >( ) ) return false;
            } else if ( g_config->jax.e_only_on_autoattack->get< bool >( ) ) {
                auto target_sci = target->spell_book.get_spell_cast_info( );
                if ( !target_sci || !target_sci->is_autoattack || target_sci->get_target_index( ) != g_local->
                    index )
                    return false;
            }


            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Jhin( ) -> bool{
            m_q_range = 550.f;
            m_w_range = 2520.f;
            m_e_range = 750.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            killsteal_w_jhin( );
            antigapclose_e_jhin( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_jhin_W( );
                spell_jhin_E( );
                spell_jhin_Q( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_jhin_Q( ) -> bool{
            if ( !g_config->jhin.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
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

        auto spell_jhin_W( ) -> bool{
            if ( !g_config->jhin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_w_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::w, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::w, unit ); }
            );
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

        auto spell_jhin_E( ) -> bool{
            if ( !g_config->jhin.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
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

        auto killsteal_w_jhin( ) -> bool{
            if ( !g_config->jhin.w_killsteal->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_w_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_jhin( ESpellSlot::w, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_jhin( ESpellSlot::w, unit ); }
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

        auto antigapclose_e_jhin( ) -> void{
            if ( !g_config->jhin.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 1.f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return;

            const Vec3 path_end = path[ 1 ];

            if ( cast_spell( ESpellSlot::e, path_end ) ) m_last_e_time = *g_time;
        }

        auto get_spell_damage_jhin( const ESpellSlot slot, Object* target ) const -> float{
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

        static auto get_spell_travel_time_jhin( const ESpellSlot slot, Object* target ) -> float{
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

        auto Jinx( ) -> bool{
            m_w_range = 1450.f;
            m_e_range = 900.f;

            if ( g_features->orbwalker->in_action( ) ) return false;

            antigapclose_e_jinx( );
            autointerrupt_e_jinx( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return false;


            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "jinxqicon" ) ) ||
                !g_features->buff_cache->get_buff( g_local->index, ct_hash( "JinxQ" ) )
            ) {
                m_is_rocket_aa  = false;
                m_minigun_range = g_local->attack_range + 65.f;
                m_rocket_range  = m_minigun_range + m_extra_q_range[ m_slot_q->level ];
            } else {
                m_is_rocket_aa  = true;
                m_rocket_range  = g_local->attack_range + 65.f;
                m_minigun_range = m_rocket_range - m_extra_q_range[ m_slot_q->level ];
            }

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_jinx_E( );
                spell_jinx_Q( );
                spell_jinx_W( );
                spell_r( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_jinx_Q( ) -> bool{
            if ( !g_config->jinx.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::laneclear &&
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "jinxqicon" ) ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::harass
            ) {
                const auto aa_damage = helper::get_aa_damage( target->index );
                bool       should_q{ };

                if ( !m_is_rocket_aa && !g_features->orbwalker->is_attackable( target->index ) &&
                    g_features->orbwalker->is_attackable( target->index, m_rocket_range ) ||
                    m_is_rocket_aa && g_features->orbwalker->is_attackable( target->index, m_minigun_range ) &&
                    target->health - aa_damage * 2.f > 0.f
                )
                    should_q = true;

                if ( !should_q ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_jinx_W( ) -> bool{
            if ( !g_config->jinx.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w || !m_slot_w
                ->is_ready( true ) )
                return false;

            auto cast_time = 0.6f - 0.02f * std::floor( g_local->bonus_attack_speed / 0.25f );
            if ( cast_time < 0.4f ) cast_time = 0.4f;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->jinx.w_only_out_of_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                3300.f,
                60.f,
                cast_time,
                { },
                true
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->jinx.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( cast_time );
                return true;
            }

            return false;
        }

        auto spell_jinx_E( ) -> bool{
            if ( !g_config->jinx.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e || !m_slot_e
                ->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                115.f,
                0.9f
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->jinx.e_hitchance->get<
                    int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto autointerrupt_e_jinx( ) -> void{
            if ( !g_config->jinx.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto predicted = g_features->prediction->predict( target->index, m_e_range, 0.f, 115.f, 0.9f );
            if ( !predicted.valid || ( int )predicted.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) m_last_e_time = *g_time;
        }

        auto antigapclose_e_jinx( ) -> void{
            if ( !g_config->jinx.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto path     = aimgr->get_path( );
            const Vec3 path_end = path[ 1 ];

            if ( path.size( ) != 2 || g_features->prediction->windwall_in_line( g_local->position, path_end ) ) return;

            if ( cast_spell( ESpellSlot::e, path_end ) ) m_last_e_time = *g_time;
        }

        auto Kaisa( ) -> bool{
            m_q_range = 600.f;
            m_w_range = 3000.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "KaisaE" ) ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "KaisaEStealth" ) ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_kaisa_Q( );
                spell_kaisa_W( );
                spell_r( );

                break;
            default:
                break;
            }
            return false;
        }

        auto spell_kaisa_Q( ) -> bool{
            if ( !g_config->kaisa.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_q_range ) return false;

            /*int minion_count{};

            for ( const auto obj : g_entity_list->get_enemy_minions( ) ) {
                if ( obj->is_dead( ) || obj->is_invisible( ) || !obj->is_lane_minion() && !obj->is_jungle_monster()
                    || obj->dist_to_local( ) > m_q_range + 25.f ) continue;

                minion_count++;
            }

            if ( minion_count > 1 ) return false;*/

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_kaisa_W( ) -> bool{
            if ( !g_config->kaisa.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->kaisa.w_minimum_stacks->get< int >( ) > 0 ) {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kaisapassivemarker" ) );
                if ( !buff || buff->stacks( ) < g_config->kaisa.w_minimum_stacks->get< int >( ) ) return false;
            }

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                1750.f,
                100.f,
                0.4f,
                { },
                true
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kaisa.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 100.f ) )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto Kalista( ) -> bool{
            m_q_range = 1100.f;
            m_e_range = 1000.f;

            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_kalista_E( );
                spell_kalista_Q( );
                spell_r( );
                break;
            default:
                break;
            }

            if ( !g_config->kalista.e_draw_damage->get< bool >( ) ) return false;

            for ( auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || is_enemy_logged( enemy->network_id ) ||
                    enemy->dist_to_local( ) > m_e_range * 1.25f )
                    continue;

                auto damage = get_spell_damage( ESpellSlot::r, enemy );
                if ( damage == 0.f ) continue;

                int32_t damage_percent = static_cast< int32_t >( std::min(
                    std::floor( damage / enemy->health * 100 ),
                    100.f
                ) );

                m_enemy_damage.push_back( { enemy->index, enemy->network_id, damage, damage_percent } );
            }

            for ( auto& inst : m_enemy_damage ) {
                auto& enemy = g_entity_list->get_by_index( inst.index );

                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range *
                    1.25f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }

                auto damage = get_spell_damage( ESpellSlot::r, enemy.get( ) );
                if ( damage == 0.f ) {
                    remove_enemy( inst.network_id );
                    continue;
                }


                int damage_percent = static_cast< int >( std::min(
                    std::floor( damage / enemy->health * 100 ),
                    100.f
                ) );

                inst.damage         = damage;
                inst.damage_percent = damage_percent;
            }
        }

        auto spell_kalista_Q( ) -> bool{
            if ( !g_config->kalista.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f
                || g_features->orbwalker->in_attack( ) || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            bool low_hitchance{ };
            if ( g_config->kalista.jump_exploit->get< bool >( ) && g_features->orbwalker->
                                                                               is_attackable( target->index ) ) {
                if ( g_features->orbwalker->get_attack_cast_delay( ) <= 0.23f ) return false;

                low_hitchance = g_features->orbwalker->should_reset_aa( );

                if ( !low_hitchance ) return false;
            }

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                2400.f,
                40.f,
                0.25f
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kalista.q_hitchance->get<
                    int >( ) && !low_hitchance ) )
                return false;

            const auto raw_q_damage = m_q_damage_kalista[ m_slot_q->level ] + g_local->attack_damage( );

            if ( g_features->prediction->minion_in_line(
                g_local->position,
                predicted.position,
                40.f,
                0,
                raw_q_damage
            ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_kalista_E( ) -> bool{
            if ( !g_config->kalista.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_slot_e->
                get_usable_state( ) == 1 || !m_slot_e->is_ready( true ) )
                return false;

            bool allow_cast{ };

            for ( const auto hero : g_entity_list->get_enemies( ) ) {
                if ( !hero ||
                    g_features->target_selector->is_bad_target( hero->index ) ||
                    g_local->position.dist_to( hero->position ) > m_e_range
                )
                    continue;

                const auto damage = get_spell_damage_kalista( ESpellSlot::e, hero );
                if ( damage == 0.f ) continue;

                auto health = g_features->prediction->predict_health( hero, 0.25f );

                if ( damage >= health || g_config->kalista.e_to_reset_cooldown->get< bool >( ) && will_e_reset( ) ) {
                    allow_cast = true;
                    break;
                }
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto get_spell_damage_kalista( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kalistaexpungemarker" ) );
                if ( !buff ) return 0.f;

                auto raw_damage = m_e_base_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.7f;

                auto stacks = buff->stacks( ) - 1;

                if ( !target->is_major_monster( ) ) {
                    stacks += g_features->prediction->get_incoming_attack_count(
                        target->index
                    );
                }

                raw_damage += stacks * ( m_e_bonus_damage[ m_slot_e->level ] + g_local->attack_damage( ) *
                    m_e_ad_multiplier[ m_slot_e->level ] );

                if ( g_config->kalista.e_coup_de_grace->get< bool >( ) && target->is_hero( ) && target->health / target
                    ->max_health < 0.4f )
                    raw_damage *= 1.08f;
                else if ( target->is_major_monster( ) ) raw_damage *= 0.5f;

                return helper::calculate_damage( raw_damage, target->index, true );
            }
            case ESpellSlot::r:
            {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "kalistaexpungemarker" ) );
                if ( !buff ) return 0.f;

                auto raw_damage = m_e_base_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.7f;

                const auto stacks = buff->stacks( ) - 1;

                raw_damage += stacks * ( m_e_bonus_damage[ m_slot_e->level ] + g_local->attack_damage( ) *
                    m_e_ad_multiplier[ m_slot_e->level ] );

                if ( g_config->kalista.e_coup_de_grace->get< bool >( ) && target->is_hero( ) && target->health / target
                    ->max_health < 0.4f )
                    raw_damage *= 1.08f;
                else if ( target->is_major_monster( ) ) raw_damage *= 0.5f;

                return helper::calculate_damage( raw_damage, target->index, true );
            }
            }

            return 0.f;
        }

        auto will_e_reset( ) -> bool{
            for ( auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->health == minion->max_health
                    || minion->dist_to_local( ) > m_e_range || !minion->is_lane_minion( ) && !minion->
                    is_jungle_monster( ) )
                    continue;

                auto spell_damage = get_spell_damage_kalista( ESpellSlot::e, minion );

                if ( spell_damage <= minion->health ) continue;

                return true;
            }

            return false;
        }

        auto is_enemy_logged( const unsigned network_id ) const -> bool{
            for ( const auto& inst : m_enemy_damage ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto remove_enemy( const unsigned network_id ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_enemy_damage,
                [&]( const enemy_damage_t& inst ) -> bool{ return inst.network_id == network_id; }
            );

            if ( to_remove.empty( ) ) return;

            m_enemy_damage.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto Karthus( ) -> bool{
            m_q_range     = 875.f;
            m_q_radius    = 160.f;
            m_q_cast_time = 0.25f;
            m_q_dam_delay = 0.65f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            m_e_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "KarthusDefile" ) );

            //std::cout << g_local->spell_book.get_spell_slot(e_spell_slot::f)->get_name() << "\n";

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_karthus_Q( );
                spell_karthus_E( );
                spell_karthus_W( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_karthus_Q( ) -> bool{
            if ( !g_config->karthus.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || m_slot_q->charges <
                2 )
                return false;

            if ( g_config->karthus.q_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position(
                    m_q_range,
                    0.f,
                    m_q_radius,
                    m_q_cast_time + m_q_dam_delay,
                    false
                );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::q, multihit.position ) ) {
                        g_features->orbwalker->set_cast_time( m_q_cast_time );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_spell_specific_target( m_q_range );

            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                // range
                0.f,
                // speed
                m_q_radius,
                // width
                m_q_cast_time + m_q_dam_delay // delay
            );

            auto failed = false;

            auto default_valid = false;

            if ( predicted.valid && predicted.hitchance >= static_cast< Prediction::EHitchance >( g_config->karthus.
                q_hitchance->get< int >( ) ) ) {
                default_valid = true;
                for ( const auto minion : g_entity_list->get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) ) continue;

                    if ( predicted.position.dist_to( minion->position ) < m_q_radius + 55.f ) {
                        failed = true;
                        break;
                    }
                }

                if ( !failed || !g_config->karthus.q_prioritize_no_minion->get< bool >( ) ) {
                    if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                        debug_log( "STANDARD PREDICTION CAST!" );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->set_cast_time( m_q_cast_time );
                        return true;
                    }
                }
            }

            if ( !g_config->karthus.q_prioritize_no_minion->get< bool >( ) ) return false;

            const auto predicted_position = g_features->prediction->predict_default(
                target->index,
                m_q_cast_time + ( m_q_dam_delay /
                    1.5f ) // delay, not quite full dam delay as this seems to increase our hit chance? Not too sure tho!
            );

            if ( predicted_position && predicted_position.has_value( ) ) {
                const auto section_degrees = 360.f / m_q_degree_sections;

                for ( auto offset = m_q_offset_step; offset <= m_q_pred_max_offset; offset += m_q_offset_step ) {
                    std::vector< Vec3 > good_positions{ };
                    for ( auto direction_tranpose = 0.f; direction_tranpose < 360.f; direction_tranpose +=
                          section_degrees ) {
                        failed = false;
                        // hypotenuse = offset
                        // degrees = direction_transpose
                        const auto radians  = direction_tranpose * m_pi / 180.f;
                        const auto x_offset = sin( radians ) * offset;
                        const auto z_offset = cos( radians ) * offset;

                        Vec3 transposed_position = {
                            predicted_position->x + x_offset,
                            predicted_position->y,
                            predicted_position->z + z_offset
                        };

                        for ( const auto minion : g_entity_list->get_enemy_minions( ) ) {
                            if ( !minion || minion->is_dead( ) ) continue;

                            const auto minion_predict = g_features->prediction->predict_default(
                                minion->index,
                                m_q_cast_time + m_q_dam_delay
                            );
                            if ( transposed_position.dist_to( minion->position ) < m_q_radius + 55.f || minion_predict
                                && minion_predict->dist_to( transposed_position ) < m_q_radius + 55.f ) {
                                failed = true;
                                break;
                            }
                        }

                        if ( !failed ) {
                            if ( transposed_position.dist_to( g_local->position ) > m_q_range ) continue;
                            if ( transposed_position.dist_to( predicted_position.value( ) ) > m_q_radius ) continue;
                            good_positions.push_back( transposed_position );
                        }
                    }

                    if ( good_positions.empty( ) ) continue; // if no transpositions are minion-less

                    Vec3 best_pos{ };

                    auto distance = std::numeric_limits<
                        float >::max( ); // default high value so first value is always better than this
                    for ( auto pos : good_positions ) {
                        const auto our_distance = pos.
                            dist_to(
                                predicted.position
                            ); // using predicted rather than predicted_position cuz it's most likely more accurate as it uses spell data
                        if ( our_distance < distance ) {
                            best_pos = pos;
                            distance = our_distance;
                        }
                    }

                    if ( cast_spell( ESpellSlot::q, best_pos ) ) {
                        debug_log( "MATHS CAST!" );
                        m_last_q_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->set_cast_time( m_q_cast_time );
                        return true;
                    }
                }
            }

            if ( default_valid ) {
                if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                    debug_log( "STANDARD PREDICTION CAST (SECONDARY)!" );
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( m_q_cast_time );
                    return true;
                }
            }

            return false;
        }

        auto spell_karthus_W( ) -> bool{
            if ( !g_config->karthus.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_q_range );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 10.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->karthus.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_karthus_E( ) -> bool{
            if ( !g_config->karthus.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f )
                return false;

            if ( is_passive_active( ) ) return false;

            const auto mana = g_local->mana / g_local->max_mana * 100;

            if ( g_config->karthus.e_min_mana->get< int >( ) > mana ) {
                if ( g_config->karthus.e_disable_under_min_mana->get< bool >( ) && m_e_active ) {
                    if ( cast_spell( ESpellSlot::e ) ) {
                        m_last_e_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->set_cast_time( 0.0f );
                        return true;
                    }

                    return false;
                }
            }

            const auto min_enemy = g_config->karthus.e_min_enemy->get< int >( );

            auto enemy_count = 0;
            for ( const auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > m_e_range ) continue;

                enemy_count++;
            }

            if ( enemy_count >= min_enemy && !m_e_active ) {
                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.0f );
                    return true;
                }
            }

            if ( enemy_count < min_enemy && m_e_active ) {
                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.0f );
                    return true;
                }
            }
            return false;
        }

        static auto is_passive_active( ) -> bool{
            return static_cast< bool >( g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "KarthusDeathDefiedBuff" )
            ) );
        }

        auto Kassadin( ) -> bool{
            m_q_range = 650.f;
            m_e_range = 600.f;
            m_w_range = 0.f;

            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->in_attack( ) ) return false;

            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "NetherBlade" ) ) ) {
                m_w_range  = g_local->attack_range + 65.f + 50.f;
                m_w_active = false;
            } else m_w_active = true;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "forcepulsecancast" ) ) ) m_e_active = true;
            else m_e_active = false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_kassadin_W( );
                spell_r( );
                spell_kassadin_E( );
                spell_kassadin_Q( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_kassadin_Q( ) -> bool{
            if ( !g_config->kassadin.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                { },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target || g_features->prediction->windwall_in_line( g_local->position, target->position ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_kassadin_W( ) -> bool{
            if ( !g_config->kassadin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) || m_w_active )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable(
                    target->index,
                    m_w_range
                ) )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_kassadin_E( ) -> bool{
            if ( !g_config->kassadin.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_e_active || !
                m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 0.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kassadin.e_hitchance
                ->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Katarina( ) -> bool{
            initialize_spell_slots( );

            if ( g_local->is_dead( ) || g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_katarina_E( );
                spell_katarina_Q( );
                spell_r( );
                spell_katarina_W( );
                break;
            default: ;
            }

            update_daggers( );

            std::vector< Vec3 > daggers;
            // m_daggers.clear( );
            for ( auto obj : g_entity_list->get_ally_minions( ) ) {
                if ( !obj || obj->is_dead( ) || rt_hash( obj->champion_name.text ) != ct_hash( "TestCubeRender" )
                    || rt_hash( obj->name.text ) != ct_hash( "HiddenMinion" ) )
                    continue;

                if ( !is_dagger_logged( obj->network_id ) ) {
                    m_logged_daggers.push_back(
                        { obj->network_id, *g_time, obj->position }
                    );
                }
                daggers.push_back( obj->position );
            }
            m_daggers_mutex.lock( );
            m_daggers = daggers;
            m_daggers_mutex.unlock( );

            return false;
        }

        auto spell_katarina_Q( ) -> bool{
            if ( !g_config->katarina.q_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_q_range
                || g_features->prediction->windwall_in_line( g_local->position, target->position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
                return true;
            }

            return false;
        }

        auto spell_katarina_W( ) -> bool{
            if ( !g_config->katarina.w_enabled->get< bool >( ) || !m_allow_spells || *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 200.f ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( *pred ) > 275.f ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_katarina_E( ) -> bool{
            if ( !g_config->katarina.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            Vec3 cast_position{ };
            bool cast_allowed{ };

            if ( m_allow_spells ) {
                for ( auto& dagger : m_daggers ) {
                    if ( g_local->position.dist_to( dagger ) > 775.f ||
                        g_local->position.dist_to( dagger ) <= 250.f ||
                        target->position.dist_to( dagger ) >= 490.f
                    )
                        continue;

                    auto instance = get_dagger_instance( dagger );
                    if ( instance.has_value( ) && *g_time - instance->spawn_time < 0.95f ) continue;

                    cast_position = dagger.extend(
                        target->position,
                        target->position.dist_to( dagger ) <= 160.f ? target->position.dist_to( dagger ) : 160.f
                    );
                    cast_allowed = true;
                }
            }

            if ( !cast_allowed && target->dist_to_local( ) <= m_e_range ) {
                const auto pred = g_features->prediction->predict_default( target->index, 0.05f );
                if ( !pred ) return false;

                if ( m_allow_spells && g_config->katarina.w_enabled->get< bool >( ) && m_slot_w->is_ready( ) ) {
                    cast_position = g_local->position.extend( target->position, target->dist_to_local( ) - 50.f );
                    cast_allowed  = true;
                } else if ( !m_slot_q->is_ready( ) && !m_slot_w->is_ready( ) ) {
                    const auto e_damage = helper::calculate_damage(
                        m_e_damage[ m_slot_e->level ] + g_local->attack_damage( ) * 0.4f + g_local->ability_power( ) *
                        0.25f,
                        target->index,
                        false
                    );
                    const auto aa_to_kill = static_cast< int32_t >( std::ceil(
                        ( target->health - e_damage ) / helper::get_aa_damage( target->index )
                    ) );

                    if ( m_allow_spells && g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->
                        is_attackable( target->index, g_local->attack_range + 50.f ) && aa_to_kill <= 4 ) {
                        cast_allowed  = true;
                        cast_position = target->position.extend( *pred, 50.f );
                    } else if ( aa_to_kill <= 1 ) {
                        cast_allowed  = true;
                        cast_position = target->position.extend( *pred, 50.f );
                    }
                }
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto is_dagger_logged( const unsigned network_id ) const -> bool{
            for ( auto& inst : m_logged_daggers ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto update_daggers( ) -> void{
            for ( const auto& inst : m_logged_daggers )
                if ( inst.spawn_time + 5.3f <= *g_time )
                    remove_dagger(
                        inst.network_id
                    );
        }

        auto remove_dagger( const unsigned network_id ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_logged_daggers,
                [&]( const dagger_instance_t& dagger ) -> bool{ return dagger.network_id == network_id; }
            );

            if ( to_remove.empty( ) ) return;

            m_logged_daggers.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto get_dagger_instance( const Vec3& position ) const -> std::optional< dagger_instance_t >{
            for ( auto& inst : m_logged_daggers ) if ( inst.position == position ) return inst;

            return std::nullopt;
        }

        auto Kayle( ) -> bool{
            m_q_range  = 900.f;
            m_q_speed  = 1600.f;
            m_q_radius = 75.f;

            m_e_range = 550.f;

            m_w_cast_time = 0.25f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_r( );
                spell_kayle_Q( );
                spell_kayle_W( );
                spell_kayle_E( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_kayle_Q( ) -> bool{
            if ( !g_config->kayle.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                { },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto cast_time = g_features->orbwalker->get_attack_cast_delay( );

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                // range
                m_q_speed,
                // speed
                m_q_radius,
                // width
                cast_time + 0.264f // cast time + delay before sword shoots
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kayle.q_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, m_q_radius )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( cast_time );
                return true;
            }

            return false;
        }

        auto spell_kayle_W( ) -> bool{
            if ( !g_config->kayle.w_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ) return false;

            if ( g_config->kayle.w_only_combo->get< bool >( ) &&
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo
            )
                return false;

            const auto mana = g_local->mana * 100 / g_local->max_mana;

            if ( mana < g_config->kayle.w_min_mana->get< int >( ) ) return false;
            switch ( g_config->kayle.w_logic->get< int >( ) ) {
            default:
                break;
            case 0:
            {
                // wasteless
                const auto missing_hp = g_local->max_health - g_local->health;

                if ( missing_hp < get_w_heal( ) ) return false;
                break;
            }

            case 1:
            {
                // below%
                const auto hp_percent = g_local->health / g_local->max_health * 100.f;

                if ( hp_percent > g_config->kayle.w_min_hp_percent->get< int >( ) ) return false;

                break;
            }
            }

            if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( m_w_cast_time );
                return true;
            }
            return false;
        }

        auto spell_kayle_E( ) -> bool{
            if ( !g_config->kayle.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable(
                    target->index,
                    m_e_range
                ) )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        static auto get_w_heal( ) -> float{
            auto w_spell = g_local->spell_book.get_spell_slot( ESpellSlot::w );

            if ( !w_spell ) return 0;

            const auto level = w_spell->level;

            if ( level == 0 ) return 0;

            return 30 + level + g_local->ability_power( ) * .3f;
        }

        auto Kindred( ) -> bool{
            m_w_range = 1300.f;
            m_e_range = 425.f;

            if ( g_features->orbwalker->in_action( ) ) return false;

            m_q_range = 300.f + g_local->attack_range + 65.f;

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_kindred_E( );
                spell_kindred_W( );
                spell_kindred_Q( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_kindred_Q( ) -> bool{
            if ( !g_config->kindred.q_enabled->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) ||
                g_config->kindred.q_aa_reset->get< bool >( ) &&
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred ) return false;

            const Vec3 cursor_position{ hud->cursor_position_unclipped };
            const auto after_dash_position{
                g_local->position.extend(
                    cursor_position,
                    std::min( 300.f, g_local->position.dist_to( cursor_position ) )
                )
            };

            if ( after_dash_position.dist_to( *pred ) > g_local->attack_range + 90.f ) return false;

            if ( cast_spell( ESpellSlot::q, cursor_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->reset_aa_timer( );
                return true;
            }

            return false;
        }

        auto spell_kindred_W( ) -> bool{
            if ( !g_config->kindred.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !pred || g_local->position.dist_to( *pred ) > 500.f ) return false;

            if ( cast_spell( ESpellSlot::w, *pred ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_kindred_E( ) -> bool{
            if ( !g_config->kindred.e_enabled->get< bool >( ) ||
                *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) ||
                g_config->kindred.e_aa_reset->get< bool >( ) &&
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto Kogmaw( ) -> bool{
            m_q_range = 1175.f;
            m_w_range = 100.f;
            m_e_range = 1300.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            m_w_range = g_local->attack_range + ( 110 + 20 * m_slot_w->level ) + 65.f;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_kogmaw_W( );
                spell_r( );
                spell_kogmaw_Q( );
                spell_kogmaw_E( );
                break;
            default:
                break;
            }
            return true;
        }

        auto spell_kogmaw_Q( ) -> bool{
            if ( !g_config->kogmaw.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( 1175.f );
            if ( !target ) return false;


            if ( g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.3f
            )
                return false;


            const auto pred = g_features->prediction->predict( target->index, 1175.f, 1650.f, 70.f, 0.25f, { }, true );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kogmaw.q_hitchance->
                    get< int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_kogmaw_W( ) -> bool{
            if ( !g_config->kogmaw.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->kogmaw.w_extend_range->get< bool >( ) && !g_features->orbwalker->is_attackable(
                target->index,
                m_w_range,
                true
            ) )
                return false;
            else if ( !g_config->kogmaw.w_extend_range->get< bool >( ) && !g_features->orbwalker->is_attackable(
                target->index,
                g_local->attack_range
            ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_kogmaw_E( ) -> bool{
            if ( !g_config->kogmaw.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_e_range );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) &&
                !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.3f
            )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 115.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kogmaw.e_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Leblanc( ) -> bool{
            m_q_range  = 700.f;
            m_e_range  = 950.f;
            m_q_damage = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };
            m_e_damage = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_leblanc_Q( );
                spell_leblanc_E( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_leblanc_Q( ) -> bool{
            if ( !g_config->leblanc.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_leblanc( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_leblanc( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        static auto spell_leblanc_W( ) -> bool{ return false; }

        auto spell_leblanc_E( ) -> bool{
            if ( !g_config->leblanc.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_leblanc( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_leblanc( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1750.f, 50.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->leblanc.e_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_leblanc( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.3f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        static auto get_spell_travel_time_leblanc( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return g_features->orbwalker->get_ping( );
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1750.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1750.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

        auto Leesin( ) -> bool{
            m_q_range  = 1200.f;
            m_e_range  = 450.f;
            m_q_damage = { 0.f, 55.f, 80.f, 105.f, 130.f, 155.f };

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                m_target = CHolder::from_object( g_features->target_selector->get_default_target( ) );
                if ( !m_target ) return false;

                spell_leesin_E( );
                spell_leesin_Q( );
                spell_leesin_W( );
                spell_r( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_leesin_Q( ) -> bool{
            if ( !g_config->leesin.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "blindmonkqmanager" ) ) ) {
                Vec3 dash_endpoint{ };
                bool found_point{ };

                for ( const auto enemy : g_entity_list->get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > 1250.f || g_features->target_selector->is_bad_target(
                        enemy->index
                    ) )
                        continue;

                    const auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "BlindMonkQOne" ) );
                    if ( !buff ) continue;

                    dash_endpoint = enemy->position;
                    found_point   = true;
                    break;
                }

                if ( !found_point || is_position_in_turret_range( dash_endpoint ) && !is_position_in_turret_range(
                    g_local->position
                ) )
                    return false;

                if ( cast_spell( ESpellSlot::q ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            } else {
                if ( !m_slot_q->is_ready( true ) || *g_time - m_last_q_start_time <= 1.75f ) return false;

                const auto target = g_features->target_selector->get_spell_specific_target(
                    m_q_range,
                    [this]( Object* unit ) -> float{ return get_spell_travel_time_leesin( ESpellSlot::q, unit ); },
                    [this]( Object* unit ) -> float{ return get_spell_damage_leesin( ESpellSlot::q, unit ); }
                );
                if ( !target ) return false;

                const auto pred = g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    1800.f,
                    60.f,
                    0.25f,
                    { },
                    true
                );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->leesin.
                    q_hitchance->get< int >( ) ) )
                    return false;

                if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f ) ) return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->set_cast_time( 0.25f );
                    m_last_q_time       = *g_time;
                    m_last_q_start_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_leesin_W( ) -> bool{
            if ( !g_config->leesin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_w_start_time <= 0.75 )
                return false;


            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "blindmonkwmanager" ) ) ) {
                if ( !m_slot_w->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time = *g_time;
                    return true;
                }
            } else {
                if ( !m_slot_w->is_ready( true ) ) return false;
                bool allow_cast{ };

                for ( const auto enemy : g_entity_list->get_enemies( ) ) {
                    if ( g_features->target_selector->is_bad_target( enemy->index ) ) continue;

                    auto sci = enemy->spell_book.get_spell_cast_info( );
                    if ( !sci || !sci->is_autoattack || sci->get_target_index( ) != g_local->index ) continue;

                    allow_cast = true;
                    break;
                }

                if ( !allow_cast ) return false;

                if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                    m_last_q_time       = *g_time;
                    m_last_w_start_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_leesin_E( ) -> bool{
            if ( !g_config->leesin.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_e_start_time <= 0.75f || !m_slot_e->is_ready( true ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "blindmonkemanager" ) ) ) return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_e_range );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( pred.value( ) ) >= m_e_range ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time       = *g_time;
                m_last_e_start_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto get_spell_damage_leesin( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ),
                    target->index,
                    true
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_leesin( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1800.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1800.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

        auto Lucian( ) -> bool{
            m_q_range     = 500.f;
            m_q_max_range = 1000.f;
            m_q_damage    = { 0.f, 95.f, 130.f, 165.f, 200.f };
            m_e_range     = 425.f;
            m_w_range     = 900.f;
            m_w_damage    = { 0.f, 75.f, 110.f, 145.f, 180.f, 215.f };

            if ( !m_passive_logged ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "LucianPassiveBuff" ) ) && *g_time -
                    m_last_cast_time <= 0.8f )
                    return false;

                m_passive_logged = true;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( )
                || g_features->buff_cache->get_buff( g_local->index, ct_hash( "LucianPassiveBuff" ) ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_lucian_Q( );
                spell_lucian_E( );
                spell_lucian_W( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_lucian_Q( ) -> bool{
            if ( !g_config->lucian.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = get_target_q_lucian( );
            if ( !target ) return false;

            if ( target->is_hero( ) && g_config->lucian.q_aa_reset->get< bool >( ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto cast_time = 0.4f - 0.15f / 17.f * static_cast< float >( ( m_slot_q->level - 1 ) );
            if ( cast_time < 0.25f ) cast_time = 0.25f;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( cast_time );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto spell_lucian_W( ) -> bool{
            if ( !g_config->lucian.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->lucian.w_only_use_after_q->get< bool >( ) &&
                m_slot_q->is_ready( true ) ||
                g_config->lucian.w_aa_reset->get< bool >( ) &&
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 120.f, 1.2f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lucian.w_hitchance->get< int >( ) )
                &&
                !will_kill ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 55.f ) &&
                !g_features->orbwalker->is_attackable( target->index )
            )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto spell_lucian_E( ) -> bool{
            if ( !g_config->lucian.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->lucian.e_last_in_combo->get< bool >( ) &&
                ( m_slot_q->is_ready( true ) || m_slot_w->is_ready( true ) ) ||
                g_config->lucian.e_only_reset_aa->get< bool >( ) && !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;
            const Vec3 cursor_position{ hud->cursor_position_unclipped };

            if ( cast_spell( ESpellSlot::e, cursor_position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto get_spell_damage_lucian( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * m_q_bonus_ad_modifier[
                        get_slot_q( )->level ],
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        auto get_target_q_lucian( ) -> Object*{
            const auto target{ g_features->target_selector->get_default_target( ) };
            if ( !target ) return { };
            if ( g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return target;

            auto cast_time = 0.4f - 0.15f / 17.f * static_cast< float >( ( m_slot_q->level - 1 ) );
            if ( cast_time < 0.25f ) cast_time = 0.25f;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, cast_time );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lucian.q_hitchance->
                get< int >( ) ) && !will_kill )
                return { };

            for ( const auto hero : g_entity_list->get_enemies( ) ) {
                if ( target->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index, m_q_range ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    60.f
                );
                auto polygon = rect.to_polygon( 55 );

                // make a box around the minion
                // see if they overlap

                if ( polygon.is_inside( pred.position ) ) return hero;
            }

            for ( const auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) ||
                    minion->is_ward( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index, m_q_range )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    60.f
                );
                auto polygon = rect.to_polygon( 55 );

                // make a box around the minion
                // see if they overlap

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            return { };
        }

        auto Lux( ) -> bool{
            m_q_range  = 1240.f;
            m_q_damage = { 0.f, 80.f, 120.f, 160.f, 200.f, 240.f };
            m_e_range  = 1100.f;
            m_e_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };
            m_w_range  = 1175.f;

            e_recast_lux( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            spell_r( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_lux_E( );
                spell_lux_Q( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_lux_Q( ) -> bool{
            if ( !g_config->lux.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_lux( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_lux( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lux.q_hitchance->get< int >( ) ) &&
                !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage_lux( ESpellSlot::q, unit ); }
                ) ||
                g_features->prediction->count_minions_in_line( g_local->position, pred.position, 70.f ) > 1
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_lux_E( ) -> bool{
            if ( !g_config->lux.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                    true
                )
                || rt_hash( m_slot_e->get_name().data() ) == ct_hash( "LuxLightstrikeToggle" ) )
                return false;

            if ( g_config->lux.e_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_r_range, 1200.f, 100.f, 0.25f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::e, multihit.position ) ) {
                        m_last_e_time = *g_time;
                        g_features->orbwalker->set_cast_time( 0.25f );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_lux( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_lux( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1200.f, 100.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lux.e_hitchance->get< int >( ) ) &&
                !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage_lux( ESpellSlot::e, unit ); }
                )
            )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto e_recast_lux( ) -> bool{
            if ( !g_config->lux.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_e_recast_time <= 1.f || !m_slot_e->is_ready( )
                || rt_hash( m_slot_e->get_name().data() ) != ct_hash( "LuxLightstrikeToggle" ) )
                return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time        = *g_time;
                m_last_e_recast_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_lux( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        static auto get_spell_travel_time_lux( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            case ESpellSlot::e:
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

        auto MissFortune( ) -> bool{
            m_q_range = 0.f;
            m_e_range = 1000.f;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_config->missfortune.w_harass->get< bool >( ) && g_features->orbwalker->get_mode( ) ==
                Orbwalker::EOrbwalkerMode::harass )
                spell_mf_W( );

            if ( g_features->orbwalker->in_attack( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_mf_Q( );
                spell_mf_E( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_mf_Q( ) -> bool{
            if ( !g_config->missfortune.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto allow_q{
                g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->is_attackable( target->index )
            };
            auto target_nid{
                allow_q ? target->network_id : 0
            };

            if ( !allow_q ) {
                for ( auto minion : g_entity_list->get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) || minion->is_invisible( )
                        || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) || !g_features->orbwalker->
                        is_attackable( minion->index ) )
                        continue;

                    auto aimgr = minion->get_ai_manager( );
                    if ( !aimgr || aimgr->is_moving && g_config->missfortune.q_ignore_moving_minion->get<
                        bool >( ) )
                        continue;

                    auto sect = sdk::math::Sector(
                        minion->position,
                        minion->position.extend( g_local->position, -50.f ),
                        75.f,
                        500.f * ( static_cast< float >( g_config->missfortune.q_max_range->get< int >( ) ) / 100.f )
                    );
                    auto polygon = sect.to_polygon_new( );

                    auto travel_time = g_features->orbwalker->get_attack_cast_delay( ) + minion->dist_to_local( ) /
                        1400.f;
                    auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, travel_time );
                    if ( !pred.valid || ( int )pred.hitchance < g_config->missfortune.q_hitchance->get< int >( ) || !
                        polygon.is_inside( pred.position ) )
                        continue;

                    allow_q    = true;
                    target_nid = minion->network_id;
                    break;
                }
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q, target_nid ) ) m_last_q_time = *g_time;

            return false;
        }

        auto spell_mf_W( ) -> bool{
            if ( !g_config->missfortune.w_enabled->get< bool >( ) || *g_time - m_last_w_time < 0.5f || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto allow_w{
                g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_last_target( ) ==
                ETargetType::hero
                && g_features->orbwalker->is_attackable( target->index )
            };

            if ( !allow_w && g_features->orbwalker->in_attack( ) ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack ) return false;

                auto& obj = g_entity_list->get_by_index( sci->get_target_index( ) );
                if ( !obj || obj->is_dead( ) || obj->is_invisible( ) ) return false;

                allow_w = obj->is_hero( );
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) m_last_w_time = *g_time;

            return false;
        }

        auto spell_mf_E( ) -> bool{
            if ( !g_config->missfortune.e_enabled->get< bool >( ) || *g_time - m_last_e_time < 0.5f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->missfortune.e_only_out_of_range->get< bool >( ) && g_features->orbwalker->
                is_attackable( target->index ) )
                return false;

            if ( g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_next_possible_aa_time( ) <=
                *g_time + 0.25f )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 0.f, 0.275f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->missfortune.e_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Morgana( ) -> bool{
            m_q_range  = 1300.f;
            m_q_damage = { 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
            m_w_range  = 900.f;
            m_e_range  = 800.f;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            spell_morg_E( );

            if ( g_features->orbwalker->in_attack( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_morg_W( );
                spell_morg_Q( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_morg_Q( ) -> bool{
            if ( !g_config->morgana.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_morg( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_morg( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->morgana.q_hitchance->get< int >( ) )
                &&
                !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage_morg( ESpellSlot::q, unit ); }
                ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_morg_W( ) -> bool{
            if ( !g_config->morgana.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 100.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->morgana.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_morg_E( ) -> bool{
            if ( !g_config->morgana.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            unsigned target_nid{ };
            bool     found_target{ };

            for ( auto ally : g_entity_list->get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > 800.f ) continue;

                if ( should_spellshield_ally( ally ) ) {
                    found_target = true;
                    target_nid   = ally->network_id;
                    break;
                }
            }

            if ( !found_target ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_morg( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        static auto get_spell_travel_time_morg( const ESpellSlot slot, Object* target ) -> float{
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

        auto Mundo( ) -> bool{
            m_q_range = 1050.f;
            m_e_range = 0.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_mundo_E( );
                spell_mundo_Q( );
                spell_r( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_mundo_Q( ) -> bool{
            if ( !g_config->mundo.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_mundo( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 2000.f, 60.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->mundo.q_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_mundo_E( ) -> bool{
            if ( !g_config->mundo.e_enabled->get< bool >( ) ||
                *g_time - m_last_e_time <= 0.5f ||
                !g_features->orbwalker->should_reset_aa( ) ||
                !m_slot_e->is_ready( ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DrMundoE" ) ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        static auto get_spell_travel_time_mundo( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::w:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto Nilah( ) -> bool{
            m_q_range    = 600.f;
            m_q_ad_ratio = { 0.f, 0.9f, 1.f, 1.1f, 1.2f, 1.3f };
            m_q_damage   = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f };
            m_w_range    = 0.f;
            m_e_range    = 550.f;
            m_e_damage   = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            update_enemies( );


            m_action_blocked = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "NilahR" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_nilah_W( );
                spell_nilah_E( );
                spell_nilah_Q( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_nilah_Q( ) -> bool{
            if ( !g_config->nilah.q_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_q_time <= 0.4f || !
                m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 75.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->nilah.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_nilah_W( ) -> bool{
            if ( !g_config->nilah.w_enabled->get< bool >( ) ||
                m_action_blocked ||
                *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "NilahW" ) )
            )
                return false;

            if ( g_local->health / g_local->max_health > g_config->nilah.w_min_health_percent->get< int >( ) *
                0.01f )
                return false;

            bool allow_w{ };
            for ( auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 3000.f ) {
                    return
                        false;
                }

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || sci->server_cast_time <= *g_time || sci->get_target_index( ) != g_local->index ) continue;

                allow_w = true;
                break;
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_nilah_E( ) -> bool{
            if ( !g_config->nilah.e_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( true ) || m_slot_e->charges == 0 )
                return false;

            bool    killable{ };
            Object* target{ };
            if ( g_config->nilah.e_on_killable->get< bool >( ) ) {
                const auto kill_target = g_features->target_selector->get_default_target( );
                if ( kill_target && kill_target->dist_to_local( ) <= m_e_range && !g_features->orbwalker->
                    is_attackable( kill_target->index ) ) {
                    const auto damage = get_spell_damage_nilah( ESpellSlot::e, kill_target ) + helper::get_aa_damage(
                        kill_target->index,
                        true
                    ) * 2.f;

                    if ( damage >= kill_target->health ) {
                        killable = true;
                        target   = kill_target;
                    }
                }
            }

            if ( !killable ) {
                auto hud = g_pw_hud->get_hud_manager( );
                if ( !hud ) return false;

                const auto cursor = hud->cursor_position_unclipped;
                auto       lowest_distance{ std::numeric_limits< float >::max( ) };

                for ( auto enemy : g_entity_list->get_enemies( ) ) {
                    if ( !enemy || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range ||
                        enemy->position.dist_to( cursor ) > g_config->nilah.e_force_dash_range->get< int >( )
                        || g_features->target_selector->is_bad_target(
                            enemy->index,
                            g_config->nilah.e_exploit->get< bool >( )
                        ) )
                        continue;

                    auto distance = enemy->position.dist_to( cursor );

                    if ( !target || distance < lowest_distance ) {
                        target          = enemy;
                        lowest_distance = distance;
                    }
                }
            }

            if ( !target || g_local->position.dist_to( target->position ) < g_config->nilah.e_min_dash_range->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( 0.1f );
                g_features->orbwalker->reset_aa_timer( );

                if ( target->is_dead( ) )
                    debug_log( "casting on dead target << {}", *g_time );

                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_nilah( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * m_q_ad_ratio[ get_slot_q( )->
                        level ],
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->attack_damage( ) * 0.2f,
                    target->index,
                    true
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_nilah( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.25f;
            case ESpellSlot::e:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto is_enemy_saved( const int16_t index ) const -> bool{
            for ( auto& handle : m_handles ) if ( handle == index ) return true;

            return false;
        }

        auto update_enemies( ) -> void{
            auto enemies = g_entity_list->get_enemies( );
            if ( enemies.size( ) == m_handles.size( ) ) {
                //std::cout << "all enemies saved: " << m_handles.size() << std::endl;
                m_enemies_saved = true;
                return;
            }

            for ( auto enemy : enemies ) {
                if ( !enemy || is_enemy_saved( enemy->index ) ) continue;

                m_handles.push_back( enemy->index );
            }
        }

        auto Olaf( ) -> bool{
            m_q_range  = 1000.f;
            m_q_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };

            m_e_range  = 325.f;
            m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };

            if ( g_local->is_dead( ) || g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_olaf_W( );
                spell_olaf_Q( );
                spell_olaf_E( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_olaf_Q( ) -> bool{
            if ( !g_config->olaf.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1600.f, 90.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->olaf.q_hitchance->get<
                int >( ) )
                return false;

            auto cast_position = pred.position;
            if ( g_local->position.dist_to( pred.position ) > 325.f
                && g_local->position.dist_to( pred.position ) < 950.f ) {
                cast_position = g_local->position.extend(
                    pred.position,
                    g_local->position.dist_to( pred.position ) + 175.f
                );
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_olaf_W( ) -> bool{
            if ( !g_config->olaf.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time <= 0.4f ||
                !g_features->orbwalker->should_reset_aa( ) ||
                !m_slot_w->is_ready( true )
            )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_olaf_E( ) -> bool{
            if ( !g_config->olaf.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_e_range ) return false;


            if ( g_config->olaf.e_weave_in_aa->get< bool >( ) && g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.25f )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto get_spell_damage_olaf( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ),
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_olaf( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1600.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1600.f;
            }
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto Orianna( ) -> bool{
            m_q_range     = 825.f;
            m_q_cast_time = 0.f;
            m_q_speed     = 1400.f;
            m_q_radius    = 175.f / 2;
            m_w_range     = 0.f;
            m_w_radius    = 225.f / 2;
            m_w_cast_time = 0.f;
            m_e_range     = 1120.f;
            m_e_speed     = 1850.f;
            m_e_cast_time = 0.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_orianna_Q( );
                spell_orianna_W( );
                spell_orianna_E( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_orianna_Q( ) -> bool{
            if ( !g_config->orianna.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_q->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !( target->dist_to_local( ) < 825.f ) ) return false;

            const auto ball_pos = ball_tracking( );


            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                m_q_speed,
                m_q_radius,
                m_q_cast_time,
                g_local->position,
                false
            );

            if ( target->dist_to_local( ) < ball_pos.dist_to( target->position ) && !m_slot_e->is_ready( ) ) {
                pred =
                    g_features->prediction->predict(
                        target->index,
                        m_q_range,
                        m_q_speed,
                        m_q_radius,
                        m_q_cast_time,
                        g_local->position,
                        false
                    );
            } else {
                pred = g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    m_q_speed,
                    m_q_radius,
                    m_q_cast_time,
                    ball_pos,
                    false
                );
            }

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->orianna.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position + 10 ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_orianna_W( ) -> bool{
            const auto ball_pos = ball_tracking( );
            if ( !g_config->orianna.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            if ( ball_is_minion ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( target->position.dist_to( ball_pos ) > 225 ) return false;

                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            return false;
        }

        auto spell_orianna_E( ) -> bool{
            const auto ball_pos = ball_tracking( );
            if ( !g_config->orianna.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.05f || !m_slot_e->is_ready( ) || ball_is_on_local )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( ball_pos, ball_pos.dist_to( g_local->position ) ),
                80.f
            );
            auto polygon = rect.to_polygon( );

            if ( polygon.is_inside( target->position ) ) {
                if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                    m_last_e_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            } else {
                if ( target->dist_to_local( ) < 150 || ball_pos.dist_to( target->position ) > target->
                    dist_to_local( ) ) {
                    if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                        m_last_e_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            return false;
        }

        auto ball_tracking( ) -> Vec3{
            for ( const auto minion : g_entity_list->get_ally_minions( ) ) {
                if ( minion->get_name( ) == "OriannaBall" ) {
                    ball_is_minion   = true;
                    ball_is_on_champ = false;
                    ball_is_on_local = false;
                    return minion->position;
                }
            }

            for ( const auto ally : g_entity_list->get_allies( ) ) {
                //not local
                if ( ally->dist_to_local( ) > 1355.f || !ally->is_alive( ) ) continue;
                for ( const auto buff : g_features->buff_cache->get_all_buffs( ally->index ) ) {
                    if ( buff->name == "orianaghost" ) {
                        ball_is_minion   = false;
                        ball_is_on_champ = true;
                        ball_is_on_local = false;
                        return ally->position;
                    } else if ( buff->name == "orianaghostself" ) {
                        ball_is_minion   = false;
                        ball_is_on_champ = false;
                        ball_is_on_local = true;
                        return g_local->position;
                    }
                }
            }
            return g_local->position;
        }

        auto Pyke( ) -> bool{
            m_q_damage     = { 0.f, 100.f, 150.f, 200.f, 250.f, 300.f };
            m_e_damage     = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };
            m_q_range      = 1100.f;
            m_e_range      = 550.f;
            m_q_base_range = 400.f;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( sci && sci->slot == 0 ) m_last_q_channel_time = *g_time;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "PykeQ" ) ) ) {
                    if ( g_config->pyke.r_interrupt_q->get< bool >( ) ) spell_r( );
                    spell_pyke_Q( );
                    return true;
                }

                spell_pyke_E( );

                if ( g_features->orbwalker->in_attack( ) ) return false;

                spell_pyke_Q( );
                break;
            default:
                break;
            }

            return true;
        }

        auto spell_pyke_Q( ) -> bool{
            if ( !g_config->pyke.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 1.f ) return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "PykeQ" ) );

            if ( !buff ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_spell_specific_target( m_q_range );
                if ( !target ) return false;

                if ( cast_spell( ESpellSlot::q ) ) {
                    g_features->orbwalker->set_cast_time( 0.025f );
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                }
            } else {
                m_q_cast_begin = 0.f;
                m_channeling_q = true;

                auto bonus_range = 0;
                if ( *g_time - buff->buff_data->start_time >= 0.5f ) {
                    bonus_range = static_cast< int32_t >( std::floor(
                        ( *g_time - buff->buff_data->start_time - 0.4f ) / 0.1f
                    ) * 116.67f );
                }

                if ( bonus_range >= 700 ) bonus_range = 700;
                //if ( bonus_range < 400 && bonus_range > 150 ) bonus_range -= 100;

                auto hitchance = *g_time - buff->buff_data->start_time > 2.f && g_config->pyke.q_hitchance->get<
                                     int >( )
                                 > 0
                                     ? 0
                                     : g_config->pyke.q_hitchance->get< int >( );

                const auto target = g_features->target_selector->get_spell_specific_target(
                    m_q_range,
                    [this]( Object* unit ) -> float{ return get_q_ranged_traveltime_pyke( unit ); },
                    [this]( Object* unit ) -> float{ return get_spell_damage_pyke( ESpellSlot::q, unit ); }
                );

                if ( !target ) return false;

                //const auto damage = get_spell_damage( e_spell_slot::q, target.get( ) ) * 1.1f;
                //const bool will_kill = damage > target->health + target->total_health_regen;

                const auto is_melee = bonus_range <= 0;

                const auto pred = g_features->prediction->predict(
                    target->index,
                    m_q_base_range + bonus_range,
                    is_melee ? 0.f : 2000.f,
                    is_melee ? 100.f : 70.f,
                    is_melee ? 0.25f : 0.2f,
                    { },
                    true
                );
                if ( !pred.valid ||
                    pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ||
                    !g_config->pyke.q_melee->get< bool >( ) &&
                    is_melee ||
                    g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) && bonus_range != 0
                )
                    return false;

                if ( release_chargeable( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->set_cast_time( 0.25f );
                    m_last_q_time  = *g_time;
                    m_channeling_q = false;
                    return true;
                }
            }

            return false;
        }

        auto spell_pyke_E( ) -> bool{
            if ( !g_config->pyke.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->is_ready(
                    true
                ) ||
                *g_time - m_last_q_channel_time >= 1.f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );

            auto aimgr = target->get_ai_manager( );
            if ( !aimgr || !aimgr->is_moving || !aimgr->is_dashing ) return false;

            auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) return false;

            Vec3 cast_direction = path[ path.size( ) - 1 ];
            if ( cast_direction.dist_to( g_local->position ) > 500.f ) return false;

            if ( g_config->pyke.e_mode->get< int >( ) != 2 ) {
                auto        current_pos = g_local->position;
                const auto  v1          = cast_direction - current_pos;
                auto        v2          = path[ 0 ] - current_pos;
                const auto  dot         = v1.normalize( ).dot_product( v2.normalize( ) );
                const float angle       = acos( dot ) * 180.f / 3.14159265358979323846f;

                if ( g_config->pyke.e_mode->get< int >( ) == 0 && angle < 80.f ||
                    g_config->pyke.e_mode->get< int >( ) == 1 && angle > 20.f ) {
                    //std::cout << "bad angle: " << angle << std::endl;
                    return false;
                }
            }

            if ( g_config->pyke.e_safe_position_check->get< bool >( ) ) {
                Vec3 dash_end_position = g_local->position.extend( cast_direction, 550.f );

                if ( !is_position_in_turret_range( g_local->position ) && is_position_in_turret_range(
                    dash_end_position
                ) )
                    return false;

                int enemy_count{ };
                for ( auto enemy : g_entity_list->get_enemies( ) ) {
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

        auto get_spell_damage_pyke( const ESpellSlot slot, Object* target ) const -> float{
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

        static auto get_q_melee_traveltime_pyke( const Object* unit ) -> float{ return 0.25f; }

        static auto get_q_ranged_traveltime_pyke( const Object* unit ) -> float{
            const auto tt   = 0.2f + g_local->position.dist_to( unit->position ) / 2000.f;
            const auto pred = g_features->prediction->predict_default( unit->index, tt );
            if ( !pred ) return 0.f;

            return 0.2f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
        }

        auto Riven( ) -> bool{
            m_q_range = 225.f;
            m_w_range = 250.f;
            m_e_range = 250.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return true;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( m_combo_active ) {
                    ew_combo_riven( );
                    return true;
                }

                spell_riven_E( );
                spell_riven_Q( );
                spell_riven_W( );
                spell_r( );
                break;
            default:
                return true;
            }
        }

        auto spell_riven_Q( ) -> bool{
            if ( !g_config->riven.q_enabled->get< bool >( ) || !m_slot_q->is_ready( ) || *g_time - m_last_q_time <
                0.25f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 175.f ) )
                return false;

            if ( m_slot_q->charges != 0 && !g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->
                is_attackable( target->index, g_local->attack_range + 50.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.2f );
                return true;
            }

            return false;
        }

        auto spell_riven_W( ) -> bool{
            if ( !g_config->riven.w_enabled->get< bool >( ) || !m_slot_w->is_ready( ) || *g_time - m_last_w_time < 0.4f
                || *g_time - m_last_q_time < 0.4f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( m_slot_q->charges != 0 && g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->riven.w_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_riven_E( ) -> bool{
            if ( !g_config->riven.e_combo_w->get< bool >( ) || m_combo_active || !m_slot_w->is_ready( ) || !m_slot_e->
                is_ready( )
                || *g_time - m_last_w_time < 0.4f || *g_time - m_last_e_time < 0.4f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 450.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || pred.position.dist_to( g_local->position ) < 250.f ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->riven.w_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time  = *g_time;
                m_combo_active = true;
            }

            return false;
        }

        auto ew_combo_riven( ) -> void{
            if ( !m_combo_active ) return;

            if ( !m_slot_w->is_ready( ) || *g_time - m_last_e_time >= 0.4f ) m_combo_active = false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr || !aimgr->is_dashing ) return;

            auto path     = aimgr->get_path( );
            auto path_end = path[ path.size( ) - 1 ];
            if ( target->position.dist_to( path_end ) > 250.f ) return;

            const auto pred = g_features->prediction->predict_default(
                g_local->index,
                g_features->orbwalker->get_ping( )
            );
            if ( !pred || pred.value( ).dist_to( target->position ) > 250.f ) return;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time  = *g_time;
                m_combo_active = false;
            }
        }

        auto Ryze( ) -> bool{
            m_q_range  = 1000.f;
            m_q_damage = { 0.f, 75.f, 100.f, 125.f, 150.f, 175.f };
            m_w_range  = 550.f;
            m_e_range  = 550.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_ryze_E( );
                spell_ryze_W( );
                spell_ryze_Q( );
                spell_r( );
                break;
            default: ;
            }
        }

        auto spell_ryze_Q( ) -> bool{
            if ( !g_config->ryze.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time_ryze( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage_ryze( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1700.f, 55.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ryze.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 55.f ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_ryze_W( ) -> bool{
            if ( !g_config->ryze.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) || m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_w_range + 90.f );
            if ( !target ) return false;

            if ( !g_features->buff_cache->get_buff( target->index, ct_hash( "RyzeE" ) ) && g_config->ryze.w_only_root->
                get< bool >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_ryze_E( ) -> bool{
            if ( !g_config->ryze.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) || m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_e_range + 90.f );
            if ( !target ) return false;

            if ( g_features->buff_cache->get_buff( target->index, ct_hash( "RyzeE" ) ) ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto get_spell_damage_ryze( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_ryze( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1700.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1700.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

        auto Samira( ) -> bool{
            m_q_range       = 950.f;
            m_w_range       = 400.f;
            m_e_range       = 600.f;
            m_melee_q_range = 340.f;
            m_q_damage      = { 0.f, 0.f, 5.f, 10.f, 15.f, 20.f };
            m_q_ad_ratio    = { 0.f, 0.85f, 0.95f, 0.105f, 0.115f, 0.125f };
            m_e_damage      = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };

            initialize_spell_slots( );

            m_grade = m_slot_r->get_usable_state( ) - 1;

            if ( g_config->samira.e_weave_aa->get< bool >( ) ) {
                if ( m_should_weave && *g_time - m_last_q_time <= 0.4f ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( target && g_features->orbwalker->is_attackable( target->index ) ) {
                        auto sci = g_local->spell_book.get_spell_cast_info( );
                        if ( sci && sci->slot == 0 && sci->server_cast_time > *g_time ) {
                            m_weave_end_time = g_features->orbwalker->get_next_possible_aa_time( );
                            m_should_weave   = false;
                        }
                    }
                } else m_should_weave = false;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            update_enemies_samira( );
            update_dash_samira( );

            m_action_blocked = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraR" ) ) || !!
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "SamiraW" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_samira_E( );
                spell_samira_Q( );
                spell_samira_W( );
                spell_r( );
                break;
            default:
                break;
            }
        }

        auto spell_samira_Q( ) -> bool{
            if ( !g_config->samira.q_enabled->get< bool >( ) || m_is_dash || m_action_blocked || *g_time - m_last_q_time
                <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto cast_position{ target->position };

            const auto ac_pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !ac_pred ) return false;

            if ( target->dist_to_local( ) > m_melee_q_range || g_local->position.dist_to( *ac_pred ) >
                m_melee_q_range ) {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2600.f, 60.f, 0.25f, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->samira.q_hitchance->get<
                    int >( ) )
                    return false;

                if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) ) return false;

                cast_position = pred.position;
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_should_weave = g_config->samira.e_weave_aa->get< bool >( ) && m_grade <= 1;
                return true;
            }

            return false;
        }

        auto spell_samira_W( ) -> bool{
            if ( !g_config->samira.w_enabled->get< bool >( ) || m_action_blocked || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( true )
                || g_config->samira.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) || m_grade
                < 4
                || m_grade == 6 )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_w_range ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_samira_E( ) -> bool{
            if ( !g_config->samira.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                    true
                )
                || g_config->samira.e_weave_aa->get< bool >( ) && should_delay_spell( ) )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto cursor = hud->cursor_position_unclipped;

            bool    target_found{ };
            Vec3    cast_position{ };
            Object* target{ };
            auto    lowest_distance{ std::numeric_limits< float >::max( ) };

            for ( auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > m_e_range ||
                    enemy->position.dist_to( cursor ) > g_config->samira.e_force_dash_range->get< int >( )
                    || g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                auto distance = enemy->position.dist_to( cursor );

                if ( !target_found || distance < lowest_distance ) {
                    target          = enemy;
                    cast_position   = enemy->position;
                    lowest_distance = distance;
                    target_found    = true;
                }
            }

            if ( !target_found || !target ) return false;

            if ( g_config->samira.e_mode->get< int >( ) > 0 ) {
                const auto predict = g_features->prediction->predict_default( target->index, 0.15f );
                if ( !predict ) return false;

                const auto after_dash_position = g_local->position.extend( cast_position, 650.f );

                if ( g_local->position.dist_to( *predict ) < after_dash_position.dist_to( *predict ) ) return false;
            }

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;

                m_last_dash_time = *g_time;
                m_is_dash        = true;
                return true;
            }

            return false;
        }

        auto should_delay_spell( ) const -> bool{
            return m_grade <= 2 && *g_time <= m_weave_end_time + g_features->orbwalker->get_ping( );
        }

        auto update_enemies_samira( ) -> void{
            auto enemies = g_entity_list->get_enemies( );
            if ( enemies.size( ) == m_handles.size( ) ) {
                //std::cout << "all enemies saved: " << m_handles.size() << std::endl;
                m_enemies_saved = true;
                return;
            }

            for ( auto enemy : enemies ) {
                if ( !enemy || is_enemy_saved( enemy->index ) ) continue;

                m_handles.push_back( enemy->index );
            }
        }

        auto update_dash_samira( ) -> void{
            if ( !m_is_dash ) return;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            if ( !m_dash_verified ) {
                if ( *g_time - m_last_dash_time > 0.25f ) {
                    debug_log( "no dash detected, reset" );
                    m_is_dash = false;
                    return;
                }

                if ( !aimgr->is_moving || !aimgr->is_dashing ) return;

                m_dash_verified = true;
                debug_log( "dash found!" );
                return;
            }

            if ( !aimgr->is_moving || !aimgr->is_dashing ) {
                debug_log( "dash ended" );

                m_is_dash = false;
            }
        }

        auto Senna( ) -> bool{
            m_q_range     = 600.f;
            m_q_max_range = 1300.f;
            m_q_damage    = { 0.f, 40.f, 80.f, 100.f, 130.f, 160.f };
            m_w_range     = 1300.f;
            m_w_damage    = { 0.f, 70.f, 115.f, 160.f, 205.f, 250.f };

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return false;

            if ( g_features->orbwalker->in_attack( ) ) return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_senna_W( );
                spell_senna_Q( );
                spell_r( );
                break;
            default:
                break;
            }

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "SennaPassiveStacks" ) );
            if ( buff ) m_q_range = 600.f + 25.f * static_cast< float >( std::floor( buff->stacks( ) / 20 ) );
            return false;
        }

        auto spell_senna_Q( ) -> bool{
            if ( !g_config->senna.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = get_target_q_senna( );
            if ( !target ) return false;

            if ( g_config->senna.q_aa_reset->get< bool >( ) && target->is_hero( ) && target->is_enemy( ) && !g_features
                ->orbwalker->should_reset_aa( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( g_features->orbwalker->get_attack_cast_delay( ) * 0.8f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_senna_W( ) -> bool{
            if ( !g_config->senna.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto damage    = get_spell_damage_senna( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->senna.w_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f )
            )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_senna( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 0.4f,
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->bonus_attack_damage( ) * 0.7f,
                    target->index,
                    true
                );
            }
            return 0.f;
        }

        auto get_target_q_senna( ) -> Object*{
            auto target{ g_features->target_selector->get_default_target( ) };
            if ( !target ) return { };
            if ( g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return target;

            auto cast_time = g_features->orbwalker->get_attack_cast_delay( ) * 0.8f;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            auto pred = g_features->prediction->predict( target->index, 1250.f, 0.f, 0.f, cast_time );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->senna.q_hitchance->
                get< int >( ) ) && !will_kill )
                return { };

            for ( auto hero : g_entity_list->get_enemies( ) ) {
                if ( !hero ||
                    target->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index, m_q_range ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( 55 );

                if ( polygon.is_inside( pred.position ) ) return hero;
            }


            for ( auto hero : g_entity_list->get_allies( ) ) {
                if ( !hero ||
                    target->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index, m_q_range ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( 55 );

                if ( polygon.is_inside( pred.position ) ) return hero;
            }

            for ( auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) &&
                    !minion->is_senna_minion( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index, m_q_range )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( 55 );

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            for ( auto minion : g_entity_list->get_ally_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index, m_q_range )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    50.f
                );
                auto polygon = rect.to_polygon( 55 );

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            return { };
        }

        auto Seraphine( ) -> bool{
            m_q_range    = 900.f;
            m_q_damage   = { 0.f, 0.f, 5.f, 10.f, 15.f, 20.f };
            m_q_ad_ratio = { 0.f, 0.85f, 0.95f, 0.105f, 0.115f, 0.125f };
            m_w_range    = 800.f;
            m_e_range    = 1300.f;
            m_e_damage   = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            m_double_cast_sera = !!g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "SeraphinePassiveEchoStage2" )
            );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_sera_E( );
                spell_sera_Q( );
                spell_r( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_sera_Q( ) -> bool{
            if ( !g_config->seraphine.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range + 50.f ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                m_double_cast_sera ? 30.f : 60.f,
                m_double_cast_sera ? 0.5f : 0.25f
            );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->seraphine.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        static auto spell_sera_W( ) -> bool{ return false; }

        auto spell_sera_E( ) -> bool{
            if ( !g_config->seraphine.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1200.f,
                70.f,
                0.25f,
                { },
                m_double_cast_sera ? false : true
            );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->seraphine.e_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto get_spell_damage_sera( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * m_q_ad_ratio[ get_slot_q( )->
                        level ],
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_sera( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto Sett( ) -> bool{
            m_q_range = 0.f;
            m_w_range = 725.f;
            m_e_range = 450.f;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            if ( m_slot_q->is_ready( true ) ) m_q_range = g_local->attack_range + 65.f + 50.f;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_sett_W( );
                spell_sett_E( );
                spell_sett_Q( );
                spell_r( );
                break;
            default: ;
            }
            return false;
        }

        auto spell_sett_Q( ) -> bool{
            if ( !g_config->sett.q_enabled->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( ) ||
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_sett_W( ) -> bool{
            if ( !g_config->sett.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( )
                ||
                g_local->mana / g_local->max_mana < g_config->sett.w_min_grit_percent->get< int >( ) / 100.f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 60.f, 0.75f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sett.w_hitchance->get<
                int >( ) )
                return false;


            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_sett_E( ) -> bool{
            if ( !g_config->sett.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->sett.e_disable_aa_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range + 55.f, 0.f, 20.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sett.e_hitchance->get<
                int >( ) )
                return false;

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( pred.position, -475.f ),
                140.f
            );
            auto polygon = rect.to_polygon( );

            bool stun_found{ };

            for ( auto minion : g_entity_list->get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_e_range +
                    300.f
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                if ( polygon.is_inside( minion->position ) ) {
                    stun_found = true;
                    break;
                }
            }

            if ( !stun_found ) {
                for ( auto enemy : g_entity_list->get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > m_e_range + 500.f || g_features->target_selector->
                        is_bad_target( enemy->index ) )
                        continue;

                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !pred ) continue;

                    if ( polygon.is_inside( *pred ) ) {
                        stun_found = true;
                        break;
                    }
                }
            }

            if ( !stun_found && g_config->sett.e_only_stun->get< bool >( ) ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Sion( ) -> bool{
            m_q_base_range = 500.f;
            m_q_range      = 0.f;
            m_q_damage     = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };
            m_w_range      = 0.f;
            m_e_range      = 800.f;
            m_e_damage     = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) || g_features->orbwalker->
                in_action( ) )
                return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );

            if ( sci && sci->slot == 0 ) {
                spell_sion_Q( );
                return false;
            }

            if ( *g_time - m_q_cast_begin > 0.2f ) m_channeling_q = false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_sion_Q( );
                spell_sion_W( );
                spell_sion_E( );
                spell_r( );
                break;
            default:
                break;
            }
        }

        auto spell_sion_Q( ) -> bool{
            if ( !g_config->sion.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            //auto buff = g_features->buff_cache->get_buff(g_local->index, ct_hash("SionQ"));

            auto sci = g_local->spell_book.get_spell_cast_info( );

            if ( !sci ) {
                if ( *g_time - m_q_cast_begin <= 0.2f || !m_slot_q->is_ready( true ) ) return false;

                const auto pred = g_features->prediction->predict( target->index, 600.f, 0.f, 0.f, 0.5f );

                if ( !pred.valid ) {
                    m_channeling_q = false;
                    return false;
                }

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    m_q_cast_begin = *g_time;
                    m_channeling_q = true;
                    return false;
                }
            } else {
                if ( sci->slot != 0 ) {
                    m_q_cast_begin = 0.f;
                    m_channeling_q = false;
                    return false;
                }

                m_q_cast_begin = 0.f;
                m_channeling_q = true;

                auto bonus_range{ 0.f };
                int  charge_amount{ };
                if ( *g_time - sci->start_time > 0.25f ) {
                    charge_amount = static_cast< int32_t >( std::floor(
                        ( *g_time - sci->start_time ) / 0.25f
                    ) );
                }

                if ( charge_amount == 1 ) bonus_range = 100.f;
                else if ( charge_amount == 2 ) bonus_range = 175.f;
                else if ( charge_amount >= 3 ) bonus_range = 275.f;

                m_q_range = m_q_base_range + bonus_range;

                const auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( sci->end_position, m_q_range ),
                    160.f
                );
                auto polygon = rect.to_polygon( );

                auto prediction_amount{ 0.025f };

                auto mgr = target->get_ai_manager( );
                if ( !mgr ) return false;

                if ( mgr->is_moving && mgr->is_dashing ) prediction_amount = 1.f;

                const auto pred = g_features->prediction->predict_default( target->index, prediction_amount );
                if ( !pred ) return false;

                if ( polygon.is_inside( target->position ) && !polygon.is_inside( *pred ) ) {
                    if ( release_chargeable( ESpellSlot::q, target->position ) ) {
                        m_last_q_time  = *g_time;
                        m_q_range      = 0.f;
                        m_channeling_q = false;
                    }
                }
            }

            return false;
        }

        auto spell_sion_W( ) -> bool{
            if ( !g_config->sion.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            auto info = m_slot_w->get_spell_info( );
            if ( !info ) return false;

            auto data = info->get_spell_data( );
            if ( !data || rt_hash( data->get_name().c_str() ) == ct_hash( "SionWDetonate" ) ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto sci = target->spell_book.get_spell_cast_info( );
            if ( !sci || sci->get_target_index( ) != g_local->index ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_sion_E( ) -> bool{
            if ( !g_config->sion.e_enabled->get< bool >( ) || m_channeling_q || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1800.f,
                80.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->sion.e_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto Sivir( ) -> bool{
            m_q_range  = 1250.f;
            m_q_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };
            m_w_range  = 0;
            m_e_range  = 0;
            m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };

            spell_sivir_E( );


            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_sivir_W( );
                spell_sivir_Q( );
                spell_r( );
                break;
            default:
                break;
            }
            return false;
        }

        auto spell_sivir_Q( ) -> bool{
            if ( !g_config->sivir.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1450.f, 90.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sivir.q_hitchance->get<
                int >( ) )
                return false;

            if ( g_config->sivir.q_max_minions->get< int >( ) < 4 &&
                g_features->prediction->count_minions_in_line( g_local->position, pred.position, 90.f ) > g_config->
                sivir.q_max_minions->get< int >( )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_sivir_W( ) -> bool{
            if ( !g_config->sivir.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                g_config->sivir.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_sivir_E( ) -> bool{
            if ( !g_config->sivir.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto allow_e{ should_spellshield_ally( g_local.get( ) ) };

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto get_spell_damage_sivir( const ESpellSlot slot, Object* target ) const -> float{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ),
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        static auto get_spell_travel_time_sivir( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1600.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1600.f;
            }
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto Swain( ) -> bool{
            m_q_range  = 725.f;
            m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
            m_w_range  = 5500.f;
            m_e_range  = 850.f;
            m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        }

        auto spell_swain_Q( ) -> bool{
            if ( !g_config->swain.q_enabled->get< bool >( ) || m_e_delay_swain || *g_time - m_last_q_time <= 0.5f || *
                g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, 700.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->swain.q_hitchance->get< int >( ) ) return false;


            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_swain_W( ) -> bool{
            if ( !g_config->swain.w_enabled->get< bool >( ) || m_e_delay_swain || *g_time - m_last_w_time <= 0.5f || *
                g_time - m_last_cast_time <= 0.15f || !m_slot_w->is_ready( true ) )
                return false;

            Object* target{ };
            auto    target_priority{ -1 };
            Vec3    cast_position{ };

            for ( auto enemy : g_entity_list->get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_w_range || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                auto       hitchance = g_config->swain.w_hitchance->get< int >( );
                auto       health    = g_features->prediction->predict_health( enemy, 1.f, false, true, true );
                const bool allow_lower_hitchance{
                    g_config->swain.w_allow_lower_hitchance->get< bool >( ) && enemy->dist_to_local( ) > 1000.f &&
                    health < enemy->health
                };

                if ( allow_lower_hitchance ) hitchance = 2;

                auto pred = g_features->prediction->predict( enemy->index, m_w_range, 0.f, 325.f, 1.5f );
                if ( !pred.valid || ( int )pred.hitchance < hitchance ) return false;

                int priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );

                if ( priority < target_priority || target && priority == target_priority && enemy->health > target->
                    health )
                    continue;

                target          = enemy;
                target_priority = priority;
                cast_position   = pred.position;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_swain_E( ) -> bool{
            if ( !g_config->swain.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f )
                return false;

            if ( rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "SwainE" ) ) {
                if ( !m_slot_e->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                auto pred = g_features->prediction->predict( target->index, m_e_range, 935.f, 0.f, 0.25f );
                if ( !pred.valid || ( int )pred.hitchance < g_config->swain.e_hitchance->get< int >( ) ) return false;

                const auto return_position = g_local->position.extend( pred.position, m_e_range );
                if ( g_features->prediction->minion_in_line( return_position, pred.position, 85.f ) ) return false;


                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            } else {
                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) return false;

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto get_spell_damage_swain( const ESpellSlot slot, Object* target ) const -> float{
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
            }

            return 0.f;
        }

        static auto get_spell_travel_time_swain( const ESpellSlot slot, Object* target ) -> float{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    protected:

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 40.f, 65.f, 90.f, 115.f, 140.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        float m_q_range{ 625.f };
        float m_w_range{ 725.f };
        float m_e_range{ 1000.f };
        float m_r_range{ 500.f };

        // q cast tracking for aatrpx
        bool    m_cast_active{ };
        bool    m_cast_detected{ };
        bool    m_fixed_direction{ };
        Vec3    m_cast_direction{ };
        int     m_cast_hitbox{ };
        int16_t m_cast_target_index{ };

        // passive magnet
        float m_last_move_time{ };
        bool  m_magnet_active{ };

        // passive tracking
        bool     m_ring_found{ };
        int16_t  m_ring_index{ };
        unsigned m_ring_nid{ };
        Vec3     m_ring_position{ };
        float    m_ring_start_time{ };
        bool     m_passive_active{ };

        std::vector< unsigned > m_ignored_rings{ };
        std::vector< Vec3 >     m_magnet_points{ };

        // *** Belveth ***
        bool                 m_e_active_belveth{ };
        int                  m_flag_belveth{ };
        std::vector< float > m_q_speed_belveth = { 0.f, 800.f, 850.f, 900.f, 950.f, 1000.f };

        // *** Cait ***
        float m_last_antimelee_time_cait{ };
        Vec3  m_last_w_position_cait{ };
        float m_cast_time_cait{ };
        bool  m_active_cait{ };
        float m_last_antimelee_time{ };

        // *** darius ***
        bool m_q_active_darius{ };
        // q magnet
        int16_t m_magnet_target{ };

        // *** draven ***
        // auto catching
        Vec3 m_last_axe_position{ };
        // attack tracking
        bool     m_attack_active{ };
        bool     m_missile_found{ };
        int16_t  m_target_index{ };
        unsigned m_missile_nid{ };
        float    m_attack_time{ };
        bool     m_will_impact{ };
        bool     m_moved_for_impact{ };
        float    m_server_cast_time{ };
        float    m_time_to_impact{ };

        int                    m_axe_count{ };
        std::vector< int16_t > m_axes{ };

        // *** ezreal ***
        Vec3                m_target_current_position{ };
        std::vector< Vec3 > m_cast_path{ };
        int                 m_next_path_node{ };
        Vec3                m_cast_pos{ };
        float               m_cast_time{ };

        // *** gnar ***
        bool m_big_gnar{ };

        // *** irelia ***
        bool    m_underturret_q{ };
        int32_t m_e_index{ };

        // *** jax ***
        bool m_e_active{ };
        bool m_w_active{ };

        // *** jhin ***
        std::array< float, 6 > m_w_damage = { 0.f, 60.f, 95.f, 130.f, 165.f, 200.f };

        // *** jinx ***
        float                  m_rocket_range{ };
        float                  m_minigun_range{ };
        float                  m_is_rocket_aa{ };
        std::array< float, 6 > m_extra_q_range =
        {
            0.f,
            80.f,
            110.f,
            140.f,
            180.f,
            200.f
        };
        float m_base_attack_range{ 525.f };

        // *** kalista ***
        std::vector< float > m_e_base_damage   = { 0.f, 20.f, 30.f, 40.f, 50.f, 60.f };
        std::vector< float > m_e_bonus_damage  = { 0.f, 10.f, 16.f, 22.f, 28.f, 34.f };
        std::vector< float > m_e_ad_multiplier = { 0.f, 0.232f, 0.2755f, 0.319f, 0.3625f, 0.406f };

        std::vector< float > m_q_damage_kalista = { 0.f, 20.f, 85.f, 150.f, 215.f, 280.f };

        std::vector< minion_buff >    m_buffed_minions{ };
        std::vector< enemy_damage_t > m_enemy_damage{ };

        // *** karthus ***
        float m_q_radius{ };
        float m_q_cast_time{ };
        float m_q_dam_delay{ };
        float m_q_pred_max_offset{ 65.f }; // increase to change how far off the original predicted position we can fire
        float m_q_offset_step
            { m_q_pred_max_offset / 5.f }; // increase the 5.f for better performance, decrease to find better position
        float m_q_degree_sections{ 16.f };

        // *** katarina ***
        std::mutex                       m_daggers_mutex;
        std::vector< Vec3 >              m_daggers{ };
        std::vector< dagger_instance_t > m_logged_daggers{ };
        bool                             m_allow_spells{ true };
        float                            m_dagger_range{ 340.f };
        float                            m_pickup_range{ 200.f };

        // *** kayle ***
        float m_q_speed{ };
        float m_w_cast_time{ 0.25f };

        // *** kindred ***
        float m_q_dash_range{ 300.f };

        // *** leesin ***
        float m_last_q_start_time{ };
        float m_last_w_start_time{ };
        float m_last_e_start_time{ };

        // *** lucian ***
        float                  m_q_max_range{ 1000.f };
        std::array< float, 6 > m_q_bonus_ad_modifier = { 0.f, .6f, .75f, .9f, 1.05f, 1.2f };
        bool                   m_passive_logged{ };

        // *** lux ***
        float m_last_e_recast_time{ };

        // *** nilah ***
        std::vector< float > m_q_ad_ratio = { 0.f, 0.9f, 1.f, 1.1f, 1.2f, 1.3f };
        bool                 m_action_blocked{ };

        std::vector< int32_t > m_handles{ };
        bool                   m_enemies_saved{ };
        float                  m_e_dash_range{ 450.f };

        // *** orianna ***
        bool  ball_is_minion{ };
        bool  ball_is_on_champ{ };
        bool  ball_is_on_local{ };
        float m_w_radius{ 225.f / 2 };
        float m_e_speed{ 1850.f };
        float m_e_cast_time{ 0.f };

        // *** pyke ***
        float m_last_q_channel_time{ };
        float m_q_base_range{ 400.f };
        bool  m_channeling_q{ };
        float m_q_cast_begin{ };

        // *** riven ***
        bool m_combo_active{ };

        // *** samira ***
        int m_grade{ };
        // e tracking
        bool  m_is_dash{ };
        bool  m_dash_verified{ };
        float m_last_dash_time{ };
        // aa weaving
        bool  m_should_weave{ };
        float m_weave_end_time{ };
        float m_melee_q_range{ 340.f };

        // *** seraphine ***
        bool m_double_cast_sera{ };

        // *** sivir ***
        bool found_buff_sivir{ };

        // *** swain ***
        bool m_e_delay_swain{ };
    };
}
