#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class xayah_module final : public IModule {
    public:
        virtual ~xayah_module( ) = default;

        struct feather_instance_t {
            int16_t  index{ };
            unsigned network_id{ };

            Object* object{ };

            float start_time{ };
            float end_time{ };

            Vec3 position{ };
        };

        struct feather_collision_instance_t {
            int16_t  target_index{ };
            unsigned network_id{ };

            float bounding_radius{ };

            std::vector< Vec3 > feathers{ };

            int   count{ };
            float last_update_time{ };
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "xayah_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Xayah" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "xayah" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto orbwalk    = navigation->add_section( _( "orbwalking" ) );

            q_settings->checkbox( _( "enable" ), g_config->xayah.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->xayah.q_harass );
            q_settings->checkbox( _( "killsteal q" ), g_config->xayah.q_killsteal );
            q_settings->checkbox( _( "weave between aa" ), g_config->xayah.q_aa_reset );
            q_settings->select(
                _( "hitchance" ),
                g_config->xayah.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->checkbox( _( "q on immobile" ), g_config->xayah.q_immobile );

            w_settings->checkbox( _( "enable" ), g_config->xayah.w_enabled );
            w_settings->checkbox( _( "turretclear w" ), g_config->xayah.w_turretclear );

            e_settings->checkbox( _( "enable" ), g_config->xayah.e_enabled );
            e_settings->checkbox( _( "killsteal e" ), g_config->xayah.e_killsteal );
            e_settings->slider_int(
                _( "min feathers on single target" ),
                g_config->xayah.e_single_target_min_feathers,
                2,
                8,
                1
            );
            e_settings->slider_int(
                _( "min feathers on multihit target" ),
                g_config->xayah.e_multihit_min_feathers,
                2,
                5,
                1
            );
            e_settings->slider_int( _( "min multihit targets" ), g_config->xayah.e_multihit_min_targets, 2, 5, 1 );
            e_settings->checkbox( _( "force root in full combo" ), g_config->xayah.e_full_combo_force_root );

            orbwalk->checkbox( _( "harass with passive" ), g_config->xayah.passive_harass );

            drawings->checkbox( _( "draw q range" ), g_config->xayah.q_draw_range );
            drawings->checkbox( _( "draw w duration" ), g_config->xayah.w_draw_duration );
            drawings->checkbox( _( "draw e hitbox" ), g_config->xayah.draw_feathers );
            drawings->checkbox( _( "draw e damage" ), g_config->xayah.e_draw_damage );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->xayah.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->xayah.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->xayah.dont_draw_on_cooldown->get< bool >( ) ) ) {
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

            Vec2 sp{ };
            if ( g_config->xayah.w_draw_duration->get< bool >( ) && m_w_active && m_w_end_time > *g_time &&
                world_to_screen( g_local->position, sp ) ) {
                Vec2 base_position = { sp.x + 50.f, sp.y - 35.f };

                auto modifier = 1.f - std::max(
                    1.f - ( m_w_end_time - *g_time ) / ( m_w_end_time - m_w_start_time ),
                    0.f
                );
                if ( modifier > 1.f ) modifier = 1.f;
                else if ( modifier < 0.f ) modifier = 0.f;

                std::string text = " ";
                auto        data = std::to_string( m_w_end_time - *g_time );
                data.resize( 3 );

                text += data + "s";

                auto size       = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );
                auto dummy_size = g_render->get_text_size( _( " 3.5s" ), g_fonts->get_zabel_16px( ), 16 );

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

            if ( g_config->xayah.e_draw_damage->get< bool >( ) ) {
                for ( const auto inst : m_collisions ) {
                    auto enemy = g_entity_list.get_by_index( inst.target_index );
                    if ( !enemy ) continue;

                    enemy.update( );

                    if ( enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 1500.f ) continue;

                    Vec2 sp{ };
                    if ( !world_to_screen( enemy->position, sp ) ) continue;

                    auto base_x = 1920.f;
                    auto base_y = 1080.f;

                    auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
                    auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

                    auto width_offset  = width_ratio * 0.055f;
                    auto height_offset = height_ratio * 0.0222f;

                    auto bar_length = width_offset * static_cast< float >( g_render_manager->get_width( ) );
                    auto bar_height = height_offset * static_cast< float >( g_render_manager->get_height( ) );

                    auto base_position = enemy->get_hpbar_position( );

                    auto buffer = 1.f;

                    base_position.x -= bar_length * 0.43f;
                    base_position.y -= bar_height;

                    auto damage      = get_e_damage( inst.target_index, inst.count );
                    auto modifier    = enemy->health / enemy->max_health;
                    auto damage_mod  = damage / enemy->max_health;
                    auto is_killable = damage > enemy->health;

                    Vec2 box_start{ base_position.x + bar_length * modifier + buffer, base_position.y - buffer };
                    Vec2 box_size{
                        damage_mod * bar_length > box_start.x - base_position.x
                            ? base_position.x - box_start.x - buffer * 1.f
                            : -( bar_length * damage_mod ) - buffer * 1.f,
                        bar_height * 0.5f + buffer * 2.f
                    };

                    g_render->filled_box(
                        box_start,
                        box_size,
                        is_killable
                            ? g_features->orbwalker->animate_color( Color( 255, 50, 75 ), EAnimationType::pulse, 10 ).
                                          alpha( 175 )
                            : Color( 40, 150, 255, 180 )
                    );
                }
            }

            if ( !g_config->xayah.draw_feathers->get< bool >( ) ) return;

            std::vector< sdk::math::Polygon > poly_list{ };

            for ( const auto inst : m_feathers ) {
                if ( !inst.object ) continue;

                auto obj = g_entity_list.get_by_index( inst.index );
                if ( !obj ) continue;

                obj.update( );

                auto object_position = obj->position;

                poly_list.push_back(
                    sdk::math::Polygon{
                        sdk::math::Rectangle(
                            object_position,
                            object_position.extend(
                                g_local->position,
                                object_position.dist_to( g_local->position ) - 100.f
                            ),
                            80.f
                        )
                        .to_polygon( )
                    }
                );
            }

            auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );
            for ( const auto poly : draw_poly ) {
                g_render->polygon_3d(
                    poly,
                    g_features->orbwalker->animate_color( { 94, 3, 252 }, EAnimationType::pulse, 3 ),
                    Renderer::outline,
                    2.f
                );
            }

            for ( auto inst : m_collisions ) {
                auto object = g_entity_list.get_by_index( inst.target_index );
                if ( !object ) continue;

                object.update( );

                if ( !world_to_screen( object->position, sp ) ) continue;

                Vec2 base_position = { sp.x + 20.f, sp.y };

                auto text = std::to_string( inst.count );

                Vec2 text_position = { sp.x + 20.f, sp.y };

                g_render->text_shadow(
                    text_position,
                    g_features->orbwalker->animate_color( { 255, 50, 50 }, EAnimationType::pulse, 10 ),
                    g_fonts->get_zabel_16px( ),
                    text.data( ),
                    16.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_feathers( );
            update_collisions( );

            if ( g_features->orbwalker->in_action( ) ||
                g_features->evade->is_active( ) )
                return;

            if ( !m_w_active ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "XayahW" ) );
                if ( buff ) {
                    m_w_active     = true;
                    m_w_start_time = buff->buff_data->start_time;
                    m_w_end_time   = buff->buff_data->end_time;
                }
            } else {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "XayahW" ) );
                if ( buff && buff->buff_data->end_time > m_w_end_time ) {
                    m_w_start_time = buff->buff_data->start_time;
                    m_w_end_time   = buff->buff_data->end_time;
                }


                if ( *g_time > m_w_end_time ) m_w_active = false;
            }

            // XayahPassiveActive - feather aa buff

            spell_w( );
            killsteal_e( );
            spell_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            killsteal_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->xayah.q_harass->get< bool >( ) && spell_q( ) ) break;

                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->xayah.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f || *g_time -
                m_last_q_time <= 0.4f
                || !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) || immobile_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target || g_config->xayah.q_aa_reset->get< bool >( ) && ( !g_features->orbwalker->should_reset_aa( )
                || g_features->orbwalker->get_last_target( ) != ETargetType::hero ) )
                return false;

            const auto delay       = std::max( 0.25f - g_local->bonus_attack_speed * 0.07f, 0.1f );
            const auto extra_delay = 0.5f * ( 1.f - std::min( g_local->bonus_attack_speed / 2.6f, 1.f ) );

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    4000.f,
                    40.f,
                    delay + extra_delay,
                    { }
                );

            auto hitchance = g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->get_last_target( ) ==
                             ETargetType::hero
                                 ? 0
                                 : g_config->xayah.q_hitchance->get< int >( );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto immobile_q( Object* target ) -> bool{
            if ( !target || !g_config->xayah.q_immobile->get< bool >( ) ) return false;

            const auto delay       = std::max( 0.25f - g_local->bonus_attack_speed * 0.07f, 0.1f );
            const auto extra_delay = 0.5f * ( 1.f - std::min( g_local->bonus_attack_speed / 2.6f, 1.f ) );

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 4000.f, 40.f, delay + extra_delay );

            if ( !pred.valid || ( int )pred.hitchance < 4 ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->xayah.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.5f || m_w_active || !m_slot_w->is_ready( true ) )
                return false;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time < *g_time ) {
                return
                    false;
            }

            const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target || !target->is_hero( ) && ( !g_config->xayah.w_turretclear->get< bool >( ) || !target->
                is_turret_object( ) ) )
                return false;

            if ( target->is_hero( ) && g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->xayah.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_e_time <= 0.5f || !m_slot_e->is_ready( true ) )
                return false;

            if ( multihit_e( ) || combo_e( ) ) return true;

            return false;
        }

        auto multihit_e( ) -> bool{
            int hitcount{ };

            for ( const auto inst : m_collisions ) {
                if ( inst.count < g_config->xayah.e_multihit_min_feathers->get< int >( ) || *g_time - inst.
                    last_update_time > 0.1f )
                    continue;

                ++hitcount;
            }

            if ( hitcount < g_config->xayah.e_multihit_min_targets->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Xayah: E Multihit ] count: " << hitcount << std::endl;
                return true;
            }

            return false;
        }

        auto combo_e( ) -> bool{
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto cast_delay = m_e_delay;
            auto target_radius = g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) );

            int count{ };

            const auto should_force_root = g_config->xayah.e_full_combo_force_root->get< bool >( ) &&
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                g_input->is_key_pressed( utils::EKey::control );

            const auto threshold = should_force_root ? 3 : g_config->xayah.e_single_target_min_feathers->get< int >( );

            for ( auto feather : m_feathers ) {
                const auto travel_time = cast_delay + feather.object->position.dist_to( target->position ) / 4000.f;
                const auto pred        = g_features->prediction->predict_default( target->index, travel_time );
                if ( !pred ) continue;

                auto local_position = g_local->position;
                auto local_pred     = g_features->prediction->predict_default( g_local->index, travel_time, false );
                if ( local_pred.has_value( ) ) local_position = *local_pred;

                auto hitbox =
                    sdk::math::Rectangle(
                        feather.position,
                        feather.position.extend( local_position, feather.position.dist_to( local_position ) ),
                        80.f + g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) )
                    )
                    .to_polygon( );

                if ( hitbox.is_outside( target->position ) ) continue;

                ++count;
            }

            if ( count < threshold ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Xayah: E Solo ] feathers: " << count << " | force root: " << should_force_root <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->xayah.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                1
            );
            if ( !target ) return false;

            const auto delay       = std::max( 0.25f - g_local->bonus_attack_speed * 0.07f, 0.1f );
            const auto extra_delay = 0.5f * ( 1.f - std::min( g_local->bonus_attack_speed / 2.6f, 1.f ) );

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 4000.f, 40.f, delay + extra_delay, { } );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Xayah: KS Q ] cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_e( ) -> bool{
            if ( !g_config->xayah.e_killsteal->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            bool allow_e{ };

            for ( const auto collision : m_collisions ) {
                if ( collision.count == 0 ) continue;

                const auto damage = get_e_damage( collision.target_index, collision.count );
                const auto health = helper::get_real_health(
                    collision.target_index,
                    EDamageType::physical_damage,
                    0.5f,
                    true
                );

                if ( damage < health ) continue;

                allow_e = true;
                break;
            }

            if ( !allow_e ) return false;


            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Xayah: KS E ] Casted" << std::endl;

                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto update_collisions( ) -> void{
            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > 2000.f ||
                    g_features->target_selector->is_bad_target( enemy->index ) ||
                    is_saved_target( enemy->network_id ) )
                    continue;

                auto cast_delay    = m_e_delay;
                auto target_radius = g_features->prediction->get_champion_radius(
                    rt_hash( enemy->champion_name.text )
                );

                int                 count{ };
                std::vector< Vec3 > points{ };

                for ( auto feather : m_feathers ) {
                    auto travel_time = cast_delay + feather.object->position.dist_to( enemy->position ) / 4000.f;
                    auto pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                    if ( !pred ) continue;

                    auto local_position = g_local->position;
                    auto local_pred     = g_features->prediction->predict_default( g_local->index, travel_time );
                    if ( local_pred.has_value( ) ) local_position = *local_pred;

                    auto hitbox =
                        sdk::math::Rectangle(
                            feather.position,
                            local_position,
                            80.f
                        )
                        .to_polygon( static_cast< int >( target_radius ) );

                    if ( hitbox.is_outside( enemy->position ) || hitbox.is_outside( *pred ) ) continue;

                    ++count;
                    points.push_back( feather.position );
                }

                if ( count == 0 ) continue;

                feather_collision_instance_t inst{
                    enemy->index,
                    enemy->network_id,
                    target_radius,
                    points,
                    count,
                    *g_time
                };

                m_collisions.push_back( inst );
            }

            for ( auto& inst : m_collisions ) {
                auto hero = g_entity_list.get_by_index( inst.target_index );
                if ( !hero ) {
                    remove_collision( inst.network_id );
                    continue;
                }

                hero.update( );

                if ( g_features->target_selector->is_bad_target( inst.target_index ) ) {
                    remove_collision( inst.network_id );
                    continue;
                }

                if ( *g_time - inst.last_update_time < 0.025f ) continue;

                auto cast_delay = m_e_delay;

                int                 count{ };
                std::vector< Vec3 > points{ };

                for ( auto feather : m_feathers ) {
                    auto travel_time = cast_delay + feather.object->position.dist_to( hero->position ) / 4000.f;
                    auto pred        = g_features->prediction->predict_default( hero->index, travel_time );
                    if ( !pred ) continue;

                    auto local_position = g_local->position;
                    auto local_pred     = g_features->prediction->predict_default( g_local->index, travel_time );
                    if ( local_pred.has_value( ) ) local_position = *local_pred;

                    auto hitbox =
                        sdk::math::Rectangle(
                            feather.position,
                            local_position,
                            80.f
                        )
                        .to_polygon( static_cast< int >( inst.bounding_radius ) );

                    if ( hitbox.is_outside( hero->position ) || hitbox.is_outside( *pred ) ) continue;

                    ++count;
                    points.push_back( feather.position );
                }

                if ( count == 0 ) {
                    remove_collision( inst.network_id );
                    continue;
                }

                inst.count            = count;
                inst.feathers         = points;
                inst.last_update_time = *g_time;
            }
        }

        auto update_feathers( ) -> void{
            for ( const auto unit : g_entity_list.get_ally_minions( ) ) {
                if ( !unit || unit->is_dead( ) || !unit->is_feather( ) || is_tracked( unit->network_id ) ) continue;


                feather_instance_t inst{ unit->index, unit->network_id, unit, *g_time, *g_time + 6.f, unit->position };


                m_feathers.push_back( inst );
            }

            for ( auto& inst : m_feathers ) {
                if ( !inst.object || inst.object->is_dead( ) || !inst.object->is_feather( ) ) {
                    remove_feather( inst.network_id );
                    continue;
                }

                inst.position = inst.object->position;
            }
        }

        auto remove_collision( const unsigned network_id ) -> void{
            if ( m_collisions.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_collisions,
                    [ & ]( const feather_collision_instance_t& instance ) -> bool{
                        return instance.network_id == network_id;
                    }
                );

            if ( to_remove.empty( ) ) return;

            m_collisions.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_saved_target( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_collisions ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto remove_feather( const unsigned network_id ) -> void{
            if ( m_feathers.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_feathers,
                    [ & ]( const feather_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_feathers.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_feathers ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto get_e_damage( const int16_t target_index, const int blade_count ) -> float{
            if ( blade_count == 0 ) return 0.f;

            const auto raw_damage = ( m_e_damage[ m_slot_e->level ] + g_local->bonus_attack_damage( ) * 0.6f ) * ( 1.f +
                0.75f * g_local->crit_chance );
            float total_damage{ };

            for ( auto i = 0; i < blade_count; i++ ) total_damage += raw_damage * ( 1.f - 0.05f * i );

            return helper::calculate_damage( total_damage, target_index, true );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto raw_damage = m_q_damage[ m_slot_q->level ] + g_local->bonus_attack_damage( ) * 0.5f;

                return helper::calculate_damage( raw_damage * 1.5f, target->index, true );
            }
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto delay       = std::max( 0.25f - g_local->bonus_attack_speed * 0.07f, 0.1f );
                const auto extra_delay = 0.5f * ( 1.f - std::min( g_local->bonus_attack_speed / 2.6f, 1.f ) );

                const auto tt   = delay + extra_delay + g_local->position.dist_to( target->position ) / 4000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return delay + extra_delay + g_local->position.dist_to( pred.value( ) ) / 4000.f;
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

        bool  m_w_active{ };
        float m_w_start_time{ };
        float m_w_end_time{ };

        // feather tracking
        std::vector< feather_instance_t >           m_feathers{ };
        std::vector< feather_collision_instance_t > m_collisions{ };

        std::vector< float > m_q_damage = { 0.f, 45.f, 60.f, 75.f, 90.f, 105.f };
        std::vector< float > m_e_damage = { 0.f, 55.f, 65.f, 75.f, 85.f, 95.f };

        float m_q_range{ 1100.f };
        float m_w_range{ 0.f };
        float m_e_range{ 0.f };
        float m_r_range{ 1000.f };

        float m_e_delay{ 0.f };
    };
} // namespace features::champion_modules
