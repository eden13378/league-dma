#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class kogmaw_module final : public IModule {
    public:
        virtual ~kogmaw_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "kogmaw_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "KogMaw" ); }

        auto initialize( ) -> void override{ m_priority_list = { r_spell, q_spell, e_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kogmaw" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto misc       = navigation->add_section( _( "misc" ) );

            q_settings->checkbox( _( "enable" ), g_config->kogmaw.q_enabled );
            q_settings->checkbox( _( "killsteal q" ), g_config->kogmaw.q_killsteal );
            q_settings->select(
                _( "hitchance" ),
                g_config->kogmaw.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->kogmaw.w_enabled );
            //w_settings->slider_int( _( "max r stacks" ), g_config->kogmaw.w_extend_range, 1, 9, 1 );
            w_settings->checkbox( _( "use to extend attackrange" ), g_config->kogmaw.w_extend_range );
            w_settings->checkbox(_("disable QER when target in range"), g_config->kogmaw.w_disable_spells_in_combo);

            e_settings->checkbox( _( "enable" ), g_config->kogmaw.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->kogmaw.e_flee );
            e_settings->select(
                _( "hitchance" ),
                g_config->kogmaw.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->kogmaw.r_enabled );
            r_settings->checkbox( _( "killsteal r" ), g_config->kogmaw.r_killsteal );
            r_settings->select(
                _( "hitchance" ),
                g_config->kogmaw.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "enemy min health %" ), g_config->kogmaw.r_min_health_percent, 10, 100, 1 );
            r_settings->slider_int( _( "max r stacks" ), g_config->kogmaw.r_max_stacks, 1, 9, 1 );

            misc->checkbox( _( "always save mana for w" ), g_config->kogmaw.w_save_mana );

            drawings->checkbox( _( "draw q range" ), g_config->kogmaw.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->kogmaw.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->kogmaw.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kogmaw.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw w buff duration" ), g_config->kogmaw.w_draw_duration );
            drawings->checkbox( _( "draw r damage" ), g_config->kogmaw.r_draw_damage );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->kogmaw.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kogmaw.dont_draw_on_cooldown->get
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

            if ( g_config->kogmaw.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && slot->is_ready( true ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            Vec2 sp{ };
            if ( g_config->kogmaw.w_draw_duration->get< bool >( ) && m_w_active && m_w_end_time > *g_time &&
                world_to_screen( g_local->position, sp ) ) {
                const Vec2 base_position = { sp.x + 50.f, sp.y - 35.f };

                auto modifier =
                    1.f - std::max( 1.f - ( m_w_end_time - *g_time ) / ( m_w_end_time - m_w_start_time ), 0.f );
                if ( modifier > 1.f ) modifier = 1.f;
                else if ( modifier < 0.f ) modifier = 0.f;

                std::string text = " ";
                auto        data = std::to_string( m_w_end_time - *g_time );
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

                const float bar_width  = dummy_size.x + 5.f;
                const auto  bar_height = 6.f;

                const Vec2 bar_position = { base_position.x, base_position.y + size.y };
                Vec2       bar_size     = { bar_width, bar_height };

                const Vec2 background_position = { bar_position.x - 1.f, bar_position.y - 1.f };
                const Vec2 background_size     = { bar_size.x + 2.f, bar_size.y + 2.f };

                g_render->filled_box( background_position, background_size, Color( 10, 10, 10, 155 ) );


                const Vec2 progress_size = Vec2{ bar_size.x * modifier, bar_size.y };

                g_render->filled_box( bar_position, progress_size, Color( 255, 255, 25 ) );
            }

            if ( g_config->kogmaw.r_draw_range->get< bool >( ) || g_config->kogmaw.r_draw_damage->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && g_config->kogmaw.r_draw_range->get< bool >( ) && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->kogmaw.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_ult_range[ slot->level ],
                        Renderer::outline,
                        70,
                        2.f
                    );
                }


                if ( g_config->kogmaw.r_draw_damage->get< bool >( ) && slot->level > 0 ) {
                    for ( const auto index : g_features->tracker->get_enemies( ) ) {
                        auto enemy = g_entity_list.get_by_index( index );
                        if ( !enemy ) continue;

                        enemy.update( );

                        if ( enemy->is_dead( ) || enemy->is_invisible( ) ||
                            enemy->dist_to_local( ) > m_ult_range[ m_slot_r->level ] + 150.f )
                            continue;

                        Vec2 sp{ };
                        if ( !world_to_screen( enemy->position, sp ) ) continue;

                        const auto base_x = 1920.f;
                        const auto base_y = 1080.f;

                        const auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
                        const auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

                        const auto width_offset  = width_ratio * 0.055f;
                        const auto height_offset = height_ratio * 0.0222f;

                        const auto bar_length = width_offset * static_cast< float >( g_render_manager->get_width( ) );
                        const auto bar_height = height_offset * static_cast< float >( g_render_manager->get_height( ) );

                        auto base_position = enemy->get_hpbar_position( );

                        const auto buffer = 1.f;

                        base_position.x -= bar_length * 0.43f;
                        base_position.y -= bar_height;

                        const auto damage      = get_ult_damage( enemy->index );
                        const auto modifier    = enemy->health / enemy->max_health;
                        const auto damage_mod  = damage / enemy->max_health;
                        const auto is_killable = damage > enemy->health;

                        const Vec2 box_start{
                            base_position.x + bar_length * modifier + buffer,
                            base_position.y - buffer
                        };
                        const Vec2 box_size{
                            damage_mod * bar_length > box_start.x - base_position.x
                                ? base_position.x - box_start.x - buffer * 1.f
                                : -( bar_length * damage_mod ) - buffer * 1.f,
                            bar_height * 0.5f + buffer * 2.f
                        };

                        g_render->filled_box(
                            box_start,
                            box_size,
                            is_killable
                                ? g_features->orbwalker->get_pulsing_color( ).alpha( 175 )
                                : Color( 40, 150, 255, 180 )
                        );
                    }
                }
            }
        }

        bool can_cast_spell( float manacost ){ return m_slot_w->level <= 0 || g_local->mana - manacost >= 35.f; }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( !m_w_active ) {
                const auto buff = g_features->buff_cache->get_buff(
                    g_local->index,
                    ct_hash( "KogMawBioArcaneBarrage" )
                );
                if ( buff ) {
                    m_w_active     = true;
                    m_w_start_time = buff->buff_data->start_time;
                    m_w_end_time   = buff->buff_data->end_time;
                }
            } else {
                const auto buff = g_features->buff_cache->get_buff(
                    g_local->index,
                    ct_hash( "KogMawBioArcaneBarrage" )
                );
                if ( buff && buff->buff_data->end_time > m_w_end_time ) {
                    m_w_start_time = buff->buff_data->start_time;
                    m_w_end_time   = buff->buff_data->end_time;
                }

                if ( *g_time > m_w_end_time || !buff ) m_w_active = false;
            }

            auto target = g_features->target_selector->get_default_target();
            disable_spells_during_w = target && g_features->orbwalker->is_attackable(target->index) &&
                g_features->orbwalker->get_mode() == Orbwalker::EOrbwalkerMode::combo;

            on_attack_w();

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            killsteal_r( );
            killsteal_q( );

            m_w_range = g_local->attack_range + ( 110 + 20 * m_slot_w->level ) + 65.f;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );

            //if ( g_config->kogmaw.disable_combo_in_aa_range->get< bool >( ) ) return;

                spell_r( );
                spell_q( );
                spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->kogmaw.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || disable_spells_during_w || !m_slot_q->is_ready(true) ||
                !can_cast_spell( m_slot_q->get_manacost( ) ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) && !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.3f )
                return false;


            const auto pred = g_features->prediction->predict( target->index, 1175.f, 1650.f, 70.f, 0.25f, { }, true );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kogmaw.q_hitchance->
                    get< int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->kogmaw.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || m_w_active || !g_config->kogmaw.w_extend_range->get<bool>() ||
                !m_slot_w->is_ready(true))
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
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto on_attack_w() -> bool{

            if (!g_config->kogmaw.w_enabled->get<bool>() || g_features->evade->is_active() || !g_features->orbwalker->in_attack() || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || m_w_active || !m_slot_w->is_ready(true))
                return false;
            
            auto sci = g_local->get_spell_book().get_spell_cast_info();
            if (!sci || !sci->is_autoattack && !sci->is_special_attack || sci->get_target_index() == 0)// || sci->server_cast_time > *g_time + g_features->orbwalker->get_ping() / 2.f + 0.033f)
                return false;

            auto target = g_entity_list.get_by_index(sci->get_target_index());
            if (!target || !target->is_hero()) return false;

            if (cast_spell(ESpellSlot::w)) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->disable_movement_until(sci->server_cast_time + g_features->orbwalker->get_ping() * 2.f  );

                std::cout << "[ Kog: W ] Cast OnAttack | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->kogmaw.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || disable_spells_during_w || !m_slot_e->is_ready(true) ||
                !can_cast_spell( m_slot_e->get_manacost( ) ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_e_range );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) &&
                !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.3f
            )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 120.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kogmaw.e_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if (!g_config->kogmaw.r_enabled->get<bool>() || disable_spells_during_w ||
                *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( ) || get_ult_manacost( ) > g_local->mana ||
                !can_cast_spell( get_ult_manacost( ) ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) &&
                !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.25f )
                return false;

            if ( target->health > target->max_health / 100.f * g_config->kogmaw.r_min_health_percent->get<
                int >( ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "kogmawlivingartillerycost" )
            );
            if ( buff && buff->stacks( ) > g_config->kogmaw.r_max_stacks->get< int >( ) ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_ult_range[ m_slot_r->level ],
                0.f,
                240.f,
                1.25f
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kogmaw.r_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Kog: R ] Combo ult, target: " << target->champion_name.text << std::endl;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->kogmaw.q_killsteal->get< bool >( ) || disable_spells_during_w || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) || !can_cast_spell(
                    m_slot_q->get_manacost( )
                ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                1200.f,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, 1200.f, 1650.f, 70.f, 0.25f, { }, true );

            if ( !predicted.valid ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    predicted.position,
                    70.f,
                    0.25f,
                    1650.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Kogmaw: Q Killsteal ] Cast target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_r( ) -> bool{
            if (!g_config->kogmaw.r_killsteal->get<bool>() || disable_spells_during_w ||
                *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_r->is_ready( ) || get_ult_manacost( ) > g_local->mana ||
                !can_cast_spell( get_ult_manacost( ) ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_ult_range[ m_slot_r->level ],
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_ult_range[ m_slot_r->level ],
                0.f,
                240.f,
                1.25f
            );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Kogmaw: R Killsteal ] Cast target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto get_ult_damage( const int16_t target_index ) -> float{
            const auto target = g_entity_list.get_by_index( target_index );
            if ( !target || target->is_invisible( ) || target->is_dead( ) ) return 0.f;

            const auto missing_health_percent = 1.f - target->health / target->max_health;
            const auto multiplier             = 1.f + std::clamp( missing_health_percent * 0.0833f / 0.05f, 0.f, 1.f );

            const auto raw_damage = m_r_damage[ m_slot_r->level ] + g_local->bonus_attack_damage( ) * 0.65f +
                g_local->ability_power( ) * 0.35f;

            return helper::calculate_damage( raw_damage * multiplier, target->index, false );
        }

        static auto get_ult_manacost( ) -> float{
            const auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "kogmawlivingartillerycost" )
            );
            if ( !buff ) return 40.f;

            return 40.f + 40.f * buff->stacks( );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                const auto missing_health_percent = 1.f - target->health / target->max_health;
                const auto multiplier = 1.f + std::clamp( missing_health_percent * 0.0833f / 0.05f, 0.f, 1.f );

                const auto raw_damage = m_r_damage[ m_slot_r->level ] + g_local->bonus_attack_damage( ) * 0.65f +
                    g_local->ability_power( ) * 0.35f;

                return helper::calculate_damage( raw_damage * multiplier, target->index, false );
            }
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.25f + g_local->position.dist_to( target->position ) / 1650.f;
            case ESpellSlot::r:
                return 1.3f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        std::vector< float > m_r_damage  = { 0.f, 100.f, 140.f, 180.f };
        std::vector< float > m_ult_range = { 0.f, 1300.f, 1500.f, 1800.f };

        std::vector< float > m_q_damage = { 0.f, 90.f, 140.f, 190.f, 240.f, 290.f };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        bool disable_spells_during_w{};

        // w tracking
        bool  m_w_active{ };
        float m_w_start_time{ };
        float m_w_end_time{ };

        float m_target_health{ };

        float m_last_r_cast{ };

        float m_q_range{ 1175.f };
        float m_w_range{ 100.f };
        float m_e_range{ 1300.f };
    };
}
