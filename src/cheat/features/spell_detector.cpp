#include "pch.hpp"

#include "spell_detector.hpp"

#include "entity_list.hpp"
#include "evade.hpp"
#include "prediction.hpp"
#include "tracker.hpp"
#include "../sdk/game/ai_manager.hpp"
#include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../sdk/game/spell_data.hpp"
#include "../sdk/game/spell_details.hpp"

#include "buff_cache.hpp"

namespace features {
    auto SpellDetector::run( ) -> void{
        if ( m_ignored_spells.size( ) > 20 ) m_ignored_spells.clear( );

        update_spells( );

        auto active_spells   = m_active_spells;
        auto active_missiles = m_active_missiles;
        auto active_objects  = m_active_objects;
        std::thread(
            [this, active_spells, active_missiles, active_objects]( ) -> void{
                std::unique_lock lock( m_render_mutex );

                m_active_missiles_render = active_missiles;
                m_active_spells_render   = active_spells;
                m_active_objects_render  = active_objects;
            }
        ).detach( );
    }

    auto SpellDetector::on_draw( ) -> void{
    }

    auto SpellDetector::update_spells( ) -> void{
        //enable_linear_debug_spell();
        //enable_circle_debug_spell();

        //m_detect_ally_missiles = true;
        //m_detect_ally_spells   = true;
        //m_detect_ally_particles = true;

        simulate_spells( );

        if ( m_first_run_time <= 0.f ) m_first_run_time = *g_time;

        // optimization
        if ( *g_time - m_last_removal_time > 5.f ) {
            remove_old_ignored_missiles( );
            remove_old_ignored_spells( );
            remove_old_ignored_objects( );
            m_last_removal_time = *g_time;
        }

        // add new spells cast
        for ( auto obj : m_detect_ally_spells ? g_entity_list.get_allies( ) : g_entity_list.get_enemies( ) ) {
            if ( !obj || obj->is_dead( ) || obj->is_invisible( ) ) continue;

            auto sci = obj->spell_book.get_spell_cast_info( );

            if ( !sci /* || sci->slot < 0 || sci->slot >= static_cast< int32_t >( ESpellSlot::max )*/ ) continue;

            if ( sci->server_cast_time <= 0.f || is_spell_active( obj->index, sci->server_cast_time ) ||
                is_spell_ignored( obj->index, sci->server_cast_time, sci->end_position ) )
                continue;


            auto sci_info = sci->get_spell_info( );
            if ( !sci_info ) continue;
            auto sci_data = sci_info->get_spell_data( );
            if ( !sci_data ) continue;

            auto spell_name{ sci_data->get_name( ) };
            auto data = get_data( rt_hash( spell_name.data( ) ) );

           // std::cout << obj->get_name() << " | " << spell_name << std::endl;

            if ( data.type == ESpellType::none ) {
                auto spell_slot = obj->spell_book.get_spell_slot( static_cast< ESpellSlot >( sci->slot ) );
                if ( !spell_slot ) continue;

                data = get_data( rt_hash( spell_slot->get_name( ).data( ) ) );
                if ( data.type == ESpellType::none ) continue;

                spell_name = spell_slot->get_name( );
            }

            data.start_pos   = sci->start_position;
            data.current_pos = sci->start_position;
            data.end_pos     = sci->end_position;
            data.raw_end_pos = sci->end_position;
            data.base_speed  = data.speed;

            data.start_time       = sci->start_time;
            data.server_cast_time = sci->server_cast_time;
            data.end_time         = sci->end_time;
            data.direction        = ( sci->end_position - sci->start_position ).normalize( );

            if ( get_manual_cast_time( spell_name ).has_value( ) )
                data.server_cast_time += get_manual_cast_time(
                    spell_name
                ).value( );
            if ( get_dynamic_danger_level( spell_name ).has_value( ) )
                data.danger = get_dynamic_danger_level(
                    spell_name
                ).value( );

            auto spell_config = get_spell_config_value( rt_hash( spell_name.c_str( ) ) );
            if ( spell_config.valid ) {
                data.allow_dodge             = spell_config.enabled;
                data.danger                  = spell_config.danger_level;
                data.ignore_health_threshold = static_cast< float >( spell_config.max_health_to_ignore );
            }

            const auto name_hash = rt_hash( spell_name.data( ) );

            switch ( name_hash ) {
            case ct_hash( "AnnieW" ):
            {
                data.angle = 60.f;
                break;
            }
            case ct_hash( "CassiopeiaR" ):
            {
                data.angle = 80.f;
                break;
            }
            case ct_hash( "VeigarEventHorizon" ):
            {
                if ( m_veigar_cage.end_time > *g_time ) return;

                m_veigar_cage.position   = sci->end_position;
                m_veigar_cage.start_time = sci->server_cast_time + 0.75f;
                m_veigar_cage.end_time   = m_veigar_cage.start_time + 3.f;

                return;
            }
            case ct_hash( "DariusAxeGrabCone" ):
            {
                data.angle = 50.f;
                break;
            }
            case ct_hash( "EnchantedCrystalArrow" ):
            {
                data.special_type = ESpecialSpell::ashe_r;
                break;
            }
            case ct_hash( "FeralScream" ):
            {
                data.angle = 60.f;
                break;
            }
            case ct_hash( "JinxR" ):
            {
                data.special_type = ESpecialSpell::jinx_r;
                break;
            }
            case ct_hash( "KarmaQ" ):
            {
                auto spell_slot = obj->spell_book.get_spell_slot( static_cast< ESpellSlot >( sci->slot ) );
                if ( !spell_slot || spell_slot->get_usable_state( ) == 0 ) break;

                data.radius = 80.f;

                auto instance{ data };

                instance.start_time       = sci->start_time;
                instance.server_cast_time = sci->server_cast_time;
                instance.end_time         = sci->server_cast_time + 950.f / data.speed;

                instance.speed           = 0.f;
                instance.base_speed      = 0.f;
                instance.type            = ESpellType::circle;
                instance.danger          = data.danger;
                instance.radius          = 280.f;
                instance.has_edge_radius = false;
                instance.start_pos       = sci->start_position;
                instance.end_pos         = sci->start_position.extend( sci->end_position, 950.f );
                instance.raw_end_pos     = instance.end_pos;
                instance.current_pos     = instance.start_pos;

                instance.windup_time  = instance.server_cast_time - instance.start_time;
                instance.source_index = obj->index;
                instance.spell_name   = _( "KarmaQMantraCircle" );

                m_active_spells.push_back( instance );
                break;
            }
            case ct_hash( "KSanteQ3" ):
            {
                data.range -= g_features->evade->get_bounding_radius( );
                break;
            }
            case ct_hash( "SyndraE" ):
            {
                data.angle = 65.f;

                auto sect = sdk::math::Sector( data.start_pos, data.end_pos, data.angle, 800.f );
                auto poly = sect.to_polygon_new( 40.f );

                for ( auto missile : m_active_missiles ) {
                    if ( rt_hash( missile.name.data( ) ) != ct_hash( "SyndraQSpell" ) ||
                        poly.is_outside( missile.end_position ) )
                        continue;

                    auto instance{ get_data( ct_hash( "SyndraESphereMissile" ) ) };

                    instance.start_pos   = missile.end_position;
                    instance.current_pos = missile.end_position;
                    instance.raw_end_pos = missile.end_position;
                    instance.end_pos     = sci->start_position.extend( missile.end_position, 1250.f );

                    auto base_sct =
                        sci->server_cast_time + sci->start_position.dist_to( missile.end_position ) / 2500.f;

                    instance.start_time       = missile.start_time;
                    instance.server_cast_time = base_sct < missile.end_time ? missile.end_time : base_sct;
                    instance.end_time         =
                        instance.server_cast_time + instance.start_pos.dist_to( instance.end_pos ) / instance.speed;

                    instance.base_speed   = instance.speed;
                    instance.windup_time  = instance.server_cast_time - instance.start_time;
                    instance.source_index = obj->index;
                    instance.spell_name   = _( "SyndraESphereMissile" );

                    m_active_spells.push_back( instance );
                }
                for ( auto unit : g_entity_list.get_enemy_missiles( ) ) {
                    if ( !unit || unit->is_dead( ) || rt_hash( unit->name.text ) != ct_hash( "Seed" ) ||
                        poly.is_outside( unit->position ) )
                        continue;

                    auto instance{ get_data( ct_hash( "SyndraESphereMissile" ) ) };

                    instance.start_pos   = unit->position;
                    instance.current_pos = unit->position;
                    instance.raw_end_pos = unit->position;
                    instance.end_pos     = sci->start_position.extend( unit->position, 1250.f );

                    instance.start_time       = sci->start_time;
                    instance.server_cast_time =
                        sci->server_cast_time + sci->start_position.dist_to( unit->position ) / 2500.f;
                    instance.end_time =
                        instance.server_cast_time + instance.start_pos.dist_to( instance.end_pos ) / instance.speed;

                    instance.base_speed   = instance.speed;
                    instance.windup_time  = instance.server_cast_time - instance.start_time;
                    instance.source_index = obj->index;
                    instance.spell_name   = _( "SyndraESphereMissile" );

                    m_active_spells.push_back( instance );
                }

                break;
            }
            case ct_hash( "SyndraE5" ):
            {
                data.angle = 90.f;

                auto sect = sdk::math::Sector( data.start_pos, data.end_pos, data.angle, 800.f );
                auto poly = sect.to_polygon_new( 40.f );

                for ( auto missile : m_active_missiles ) {
                    if ( rt_hash( missile.name.data( ) ) != ct_hash( "SyndraQSpell" ) ||
                        poly.is_outside( missile.end_position ) )
                        continue;

                    auto instance{ get_data( ct_hash( "SyndraESphereMissile" ) ) };

                    instance.start_pos   = missile.end_position;
                    instance.current_pos = missile.end_position;
                    instance.raw_end_pos = missile.end_position;
                    instance.end_pos     = sci->start_position.extend( missile.end_position, 1250.f );

                    instance.end_pos.y = g_navgrid->get_height( instance.end_pos );

                    auto base_sct =
                        sci->server_cast_time + sci->start_position.dist_to( missile.end_position ) / 2500.f;

                    instance.start_time       = missile.start_time;
                    instance.server_cast_time = base_sct < missile.end_time ? missile.end_time : base_sct;
                    instance.end_time         =
                        instance.server_cast_time + instance.start_pos.dist_to( instance.end_pos ) / instance.speed;

                    instance.base_speed   = instance.speed;
                    instance.windup_time  = instance.server_cast_time - instance.start_time;
                    instance.source_index = obj->index;
                    instance.spell_name   = _( "SyndraESphereMissile" );

                    m_active_spells.push_back( instance );
                }

                for ( auto unit : g_entity_list.get_enemy_minions( ) ) {
                    if ( !unit || unit->is_dead( ) || rt_hash( unit->name.text ) != ct_hash( "Seed" ) ||
                        poly.is_outside( unit->position ) )
                        continue;

                    auto instance{ get_data( ct_hash( "SyndraESphereMissile" ) ) };

                    instance.start_pos   = unit->position;
                    instance.current_pos = unit->position;
                    instance.raw_end_pos = unit->position;
                    instance.end_pos     = sci->start_position.extend( unit->position, 1250.f );
                    instance.end_pos.y   = g_navgrid->get_height( instance.end_pos );

                    instance.start_time       = sci->start_time;
                    instance.server_cast_time =
                        sci->server_cast_time + sci->start_position.dist_to( unit->position ) / 2500.f;
                    instance.end_time =
                        instance.server_cast_time + instance.start_pos.dist_to( instance.end_pos ) / instance.speed;

                    instance.base_speed   = instance.speed;
                    instance.windup_time  = instance.server_cast_time - instance.start_time;
                    instance.source_index = obj->index;
                    instance.spell_name   = _( "SyndraESphereMissile" );

                    m_active_spells.push_back( instance );
                }

                break;
            }
            case ct_hash( "UrgotE" ):
            {
                data.speed = 1200.f + obj->movement_speed;
                break;
            }
            case ct_hash( "ViegoQ" ):
            {
                if ( sci->is_special_attack ) continue;
                break;
            }
            case ct_hash( "ViegoR" ):
            {
                data.start_time = sci->server_cast_time - 0.5f;

                data.radius -= g_features->evade->get_bounding_radius( ) / 2.f;
                break;
            }
            case ct_hash( "VelkozE" ):
            {
                data.has_edge_radius = false;
                break;
            }
            case ct_hash( "VexR" ):
            {
                auto spell_level = obj->spell_book.get_spell_slot( ESpellSlot::r )->level;

                if ( spell_level == 3 ) data.range = 3000.f;
                else if ( spell_level == 2 ) data.range = 2500.f;
                else data.range                         = 2000.f;
                break;
            }
            case ct_hash( "VexQ" ):
            {
                auto instance{ data };

                instance.end_pos     = sci->start_position.extend( sci->end_position, 500.f );
                instance.raw_end_pos = instance.end_pos;

                instance.radius     = 180.f;
                instance.speed      = 600.f;
                instance.base_speed = 600.f;
                instance.range      = 500.f;

                instance.windup_time = sci->windup_time;
                instance.end_time    =
                    instance.server_cast_time + instance.start_pos.dist_to( instance.end_pos ) / instance.speed;
                instance.source_index = obj->index;

                m_active_spells.push_back( instance );
                data.start_pos        = sci->start_position.extend( sci->end_position, 500.f );
                data.end_pos          = sci->start_position.extend( sci->end_position, 1200.f - data.radius );
                data.server_cast_time = sci->server_cast_time + 0.8f;
                data.current_pos      = data.start_pos;
                data.windup_time      = sci->windup_time + 0.8f;

                break;
            }
            case ct_hash( "WarwickR" ):
            {
                if ( sci->windup_time <= 0.01f ) continue;

                data.range = obj->movement_speed * 2.f;
                break;
            }
            case ct_hash( "YasuoQ3" ):
            {
                data.raw_start_pos    = obj->position;
                data.update_direction = true;
                break;
            }
            case ct_hash( "JarvanIVDragonStrike" ):
            {
                data.should_run_check = true;
                data.check_type       = ECheckType::jarvan_iv_q;

                int16_t beacon_index{ };
                float   furthest_beacon{ };
                for ( auto unit : g_entity_list.get_enemy_minions( ) ) {
                    if ( !unit || unit->is_dead( ) || unit->position.dist_to( obj->position ) > 1000.f ||
                        unit->get_name( ).find( _( "JarvanIVStandard" ) ) == std::string::npos )
                        continue;

                    auto hitbox =
                        sdk::math::Rectangle( obj->position, obj->position.extend( sci->end_position, 915.f ), 150.f );

                    auto poly = hitbox.to_polygon( 65 );

                    auto closest_point = g_features->evade->get_closest_line_point(
                        obj->position,
                        obj->position.extend( sci->end_position, 915.f ),
                        unit->position
                    );
                    auto dist = closest_point.dist_to( obj->position );

                    if ( !poly.is_inside( unit->position ) || dist < furthest_beacon || closest_point.dist_to(
                        unit->position
                    ) > 216.f )
                        continue;

                    beacon_index    = unit->index;
                    furthest_beacon = dist;
                }

                if ( beacon_index == 0 ) break;

                auto& unit = g_entity_list.get_by_index( beacon_index );
                if ( !unit ) break;

                SpellInstance instance{ };

                data.should_run_check = false;

                instance.start_pos   = obj->position;
                instance.current_pos = obj->position;
                instance.raw_end_pos = unit->position;
                instance.end_pos     = unit->position;

                instance.speed           = 2500.f;
                instance.base_speed      = 2500.f;
                instance.danger          = 4;
                instance.cc              = true;
                instance.collision       = false;
                instance.radius          = 180.f;
                instance.type            = ESpellType::line;
                instance.range           = 900.f;
                instance.has_edge_radius = false;

                instance.spell_name = _( "JarvanIVBeaconDash" );
                instance.start_time = *g_time;
                instance.server_cast_time = sci->server_cast_time;
                instance.end_time = sci->server_cast_time + obj->position.dist_to( unit->position ) / instance.speed;

                instance.source_index = obj->index;

                instance.windup_time     = sci->windup_time;
                instance.total_cast_time = sci->total_cast_time;
                m_active_spells.push_back( instance );

                auto circle_instance{ instance };

                circle_instance.type        = ESpellType::circle;
                circle_instance.end_pos     = unit->position;
                circle_instance.speed       = 0.f;
                circle_instance.base_speed  = 0.f;
                circle_instance.raw_end_pos = unit->position + sdk::math::Vec3( 1, 1, 1 );

                m_active_spells.push_back( circle_instance );

                break;
            }
            case ct_hash( "XerathArcanopulse2" ):
            {
                if ( data.start_time > *g_time + 0.1f ) {
                    // std::cout << "retarded start time: " << sci->start_time << " | GAMETIME: " << *g_time <<
                    // std::endl;

                    data.start_time       = *g_time;
                    data.server_cast_time = data.start_time + 0.5f;
                    data.end_time         = data.server_cast_time;
                } else if ( sci->start_time == 0.f ) {
                    // std::cout << "null start time: " << sci->start_time << " | GAMETIME: " << *g_time << std::endl;

                    data.start_time = sci->server_cast_time - sci->windup_time;
                }

                break;
            }
            case ct_hash( "SylasQ" ):
                if ( sci->start_position.dist_to( sci->end_position ) < 175.f ) {
                    data.end_pos         = data.start_pos.extend( data.end_pos, 175.f );
                    data.has_edge_radius = true;
                }

                break;
            case ct_hash( "ThreshQInternal" ):
            {
                auto slot = obj->spell_book.get_spell_slot( ESpellSlot::q );
                if ( !slot ) continue;

                auto details = slot->get_details( );
                if ( !details ) continue;

                data.start_pos   = details->last_start_position;
                data.end_pos     = details->last_end_position;
                data.current_pos = data.start_pos;
                break;
            }
            default:
                break;
            }

            data.has_edge_radius = has_edge_range( name_hash );

            //JarvanIVStandard

            if ( get_manual_endtime( spell_name ).has_value( ) )
                data.end_time = data.server_cast_time +
                    get_manual_endtime( spell_name ).value( );
            else if ( name_hash == ct_hash( "VexQ" ) ) data.end_time = data.server_cast_time + 700.f / data.speed;
            else if ( data.type == ESpellType::line && data.speed > 0 )
                data.end_time = data.server_cast_time + data.
                    range / data.speed;
            else if ( data.type == ESpellType::circle && data.speed > 0 )
                data.end_time = data.server_cast_time + data
                                                        .start_pos.dist_to( data.end_pos ) / data.speed;

            if ( data.end_time <= *g_time ) continue;

            data.windup_time     = sci->windup_time;
            data.total_cast_time = sci->total_cast_time;
            data.spell_name      = spell_name;
            data.missile_nid     = sci->missile_nid;

            data.source       = std::make_shared< Object >( *obj );
            data.source_index = obj->index;

            if ( name_hash == ct_hash( "VexQ" ) ) data.windup_time = sci->windup_time + 0.8f;

            if ( is_ignored_missile( sci->missile_nid ) ) continue;

            if ( data.type == ESpellType::line ) {
                if ( should_fix_range( spell_name ) ) {
                    data.end_pos = data.start_pos.extend( data.end_pos, data.range + data.radius );
                } else data.end_pos = data.start_pos.extend( data.end_pos, data.start_pos.dist_to( data.end_pos ) );
            } else if ( data.type == ESpellType::circle && sci->start_position.dist_to( sci->end_position ) > data.
                range ) {
                data.end_pos = sci->start_position.extend( sci->end_position, data.range );
                if ( data.speed > 0.f )
                    data.end_time = data.server_cast_time + data.start_pos.dist_to( data.end_pos ) /
                        data.speed;
            }

            switch ( name_hash ) {
            case ct_hash( "AatroxQ2" ):
            {
                auto direction = obj->position + obj->get_direction( );

                data.base_position = obj->position;
                data.direction     = obj->get_direction( );

                data.start_pos        = obj->position.extend( direction, 475.f );
                data.current_pos      = data.start_pos;
                data.end_pos          = data.start_pos.extend( direction, -180.f );
                data.windup_time      = 0.3f;
                data.server_cast_time = data.start_time + data.windup_time;

                data.base_distance_to_start = obj->position.dist_to( data.start_pos );
                data.base_distance_to_end   = obj->position.dist_to( data.end_pos );

                data.stick_full_duration = true;
                data.is_hitbox_relative  = true;

                data.radius = 120.f;
                break;
            }
            case ct_hash( "AatroxQ3" ):
            {
                auto direction = obj->position + obj->get_direction( );

                data.base_position = obj->position;
                data.direction     = obj->get_direction( );

                data.start_pos   = data.base_position.extend( direction, 325.f );
                data.current_pos = data.start_pos;
                data.end_pos     = data.start_pos.extend( direction, -200.f );

                data.windup_time      = 0.3f;
                data.server_cast_time = data.start_time + data.windup_time;

                data.base_distance_to_start = obj->position.dist_to( data.start_pos );
                data.base_distance_to_end   = obj->position.dist_to( data.end_pos );

                data.stick_full_duration = true;
                data.is_hitbox_relative  = true;

                data.radius = 310.f;
                break;
            }
            case ct_hash( "AatroxQ" ):
            {
                auto direction = obj->position + obj->get_direction( );

                data.base_position = obj->position;
                data.direction     = obj->get_direction( );

                data.start_pos = obj->position;
                data.end_pos   = data.start_pos.extend( direction, 190.f );
                data.radius    = 160.f;

                data.base_distance_to_start = obj->position.dist_to( data.start_pos );
                data.base_distance_to_end   = obj->position.dist_to( data.end_pos );

                data.stick_full_duration = true;
                data.is_hitbox_relative  = true;

                break;
            }
            case ct_hash( "GwenQ" ):
            {
                data.base_position = obj->position;
                data.direction     = ( sci->end_position - sci->start_position ).normalize( );

                auto direction = obj->position + data.direction;

                data.start_pos   = sci->start_position.extend( sci->end_position, -50.f );
                data.current_pos = data.start_pos;

                data.end_pos          = data.start_pos.extend( direction, 525.f );
                data.server_cast_time = sci->server_cast_time;
                data.end_time         = sci->server_cast_time + 0.15f;

                data.base_distance_to_start = -50.f;
                data.base_distance_to_end   = obj->position.dist_to( data.end_pos );

                data.stick_full_duration = true;
                data.is_hitbox_relative  = false;
                break;
            }
            case ct_hash( "HeimerdingerE" ):
            {
                auto instance{ data };

                instance.danger = 4;
                instance.radius = 135;

                m_active_spells.push_back( instance );
                break;
            }
            case ct_hash( "HeimerdingerEUlt" ):
            {
                m_ignored_missiles.push_back( { data.missile_nid, *g_time } );
                m_active_spells.push_back( data );

                auto instance{ data };

                instance.danger = 4;
                instance.radius = 135.f;

                m_active_spells.push_back( instance );

                auto bounce{ data };

                // bounce 1
                bounce.end_pos          = data.end_pos.extend( data.start_pos, -300.f );
                bounce.raw_end_pos      = bounce.end_pos;
                bounce.end_time         = data.end_time + 0.5f;
                bounce.radius           = 250.f;
                bounce.server_cast_time = data.server_cast_time + 0.01f;
                bounce.missile_nid      = 0;
                bounce.danger           = 2;

                m_active_spells.push_back( bounce );

                // bounce 1 stun
                bounce.danger = 4;
                bounce.radius = 135.f;

                m_active_spells.push_back( bounce );

                // bounce 2
                bounce.end_pos     = bounce.end_pos.extend( bounce.start_pos, -300.f );
                bounce.raw_end_pos = bounce.end_pos;
                bounce.end_time += 0.5f;
                bounce.radius = 250.f;
                bounce.server_cast_time += 0.01f;
                bounce.missile_nid = 0;
                bounce.danger      = 2;

                m_active_spells.push_back( bounce );

                // bounce 2 stun
                bounce.danger = 4;
                bounce.radius = 135.f;
                m_active_spells.push_back( bounce );

                continue;
            }
            case ct_hash( "LeonaSolarFlare" ):
            {
                m_ignored_missiles.push_back( { data.missile_nid, *g_time } );
                m_active_spells.push_back( data );

                auto instance{ data };

                instance.danger = 5;
                instance.radius = 125.f;

                m_active_spells.push_back( instance );
                continue;
            }
            case ct_hash( "LilliaW" ):
            {
                m_ignored_missiles.push_back( { data.missile_nid, *g_time } );
                m_active_spells.push_back( data );

                auto instance{ data };

                instance.danger = 3;
                instance.radius = 70.f;

                m_active_spells.push_back( instance );
                continue;
            }
            case ct_hash( "LilliaE" ):
            {
                data.start_pos   = sci->end_position;
                data.current_pos = sci->end_position;
                data.end_pos     = sci->start_position.extend(
                    sci->end_position,
                    sci->start_position.dist_to( sci->end_position ) + data.speed * ( 0.75f + data.radius * 2.f /
                        g_local->movement_speed )
                );

                data.windup_time      = sci->windup_time + sci->start_position.dist_to( sci->end_position ) / 1400.f;
                data.server_cast_time = sci->server_cast_time + sci->start_position.dist_to( sci->end_position ) /
                    1400.f;
                data.end_time = data.server_cast_time + data.start_pos.dist_to( data.end_pos ) / 1400.f + 0.15f;
                break;
            }
            case ct_hash( "RakanW" ):
            {
                auto aimgr = obj->get_ai_manager( );
                if ( !aimgr || !aimgr->is_dashing ) continue;

                auto start = aimgr->path_start;
                auto end   = aimgr->path_end;

                data.end_time = sci->start_time + start.dist_to( end ) / 1700.f + 0.35f;

                auto cast_delta = start.dist_to( end );
                if ( cast_delta > data.range ) {
                    data.end_pos = start.extend( end, data.range );
                    std::cout << "MODIFIED | cast distance: " << cast_delta << std::endl;
                } else {
                    data.end_pos = end;
                    std::cout << "NORMAL | cast distance: " << cast_delta << std::endl;
                }

                break;
            }
            case ct_hash( "ThreshE" ):
            {
                data.start_pos   = sci->start_position.extend( sci->end_position, -500.f );
                data.current_pos = data.start_pos;
                data.end_pos     = sci->start_position.extend( sci->end_position, 525.f );
                data.speed       = 2000.f;
                data.base_speed  = 2000.f;

                data.server_cast_time = sci->start_time + 0.05f;
                data.end_time         = sci->start_time + 1050.f / 2000.f;
                break;
            }
            case ct_hash( "UrgotE" ):
            {
                data.is_direction_locked = false;
                data.stick_during_cast   = true;
                break;
            }
            case ct_hash( "ViegoQ" ):
            {
                data.start_pos = data.end_pos.extend(
                    data.current_pos,
                    data.current_pos.dist_to( data.end_pos ) + g_features->evade->get_bounding_radius( )
                );
                data.current_pos = data.start_pos;
                break;
            }
            case ct_hash( "VexE" ):
            {
                data.radius = 180.f + data.start_pos.dist_to( data.end_pos ) / 800.f * 100.f;
                break;
            }
            case ct_hash("HweiQQ"): {

                auto q_circle = SpellInstance{ data };
                q_circle.type = ESpellType::circle;
                q_circle.radius = 175.f;
                q_circle.collision = false;
                q_circle.has_edge_radius = true;

                  m_active_spells.push_back(q_circle);
                break;
                
            }
            case ct_hash( "ZiggsQ" ):
            {
                data.has_edge_radius = false;

                auto q_bounce{ data };

                q_bounce.end_pos = data.start_pos.extend(
                    data.end_pos,
                    data.start_pos.dist_to( data.end_pos ) * 1.38f
                );
                q_bounce.raw_end_pos      = q_bounce.end_pos;
                q_bounce.end_time         = data.end_time + 0.6f;
                q_bounce.radius           = 180.f;
                q_bounce.server_cast_time = data.server_cast_time + 0.01f;
                q_bounce.missile_nid      = 0;

                m_active_spells.push_back( q_bounce );

                q_bounce.end_pos = data.start_pos.extend(
                    data.end_pos,
                    data.start_pos.dist_to( data.end_pos ) * 1.65f
                );
                q_bounce.raw_end_pos = q_bounce.end_pos;
                q_bounce.end_time += 0.55f;
                q_bounce.radius = 240.f;
                q_bounce.server_cast_time += 0.01f;
                q_bounce.missile_nid = 0;

                m_active_spells.push_back( q_bounce );
                break;
            }
            case ct_hash( "ZedQ" ):
            {
                for ( const auto obj : g_entity_list.get_enemy_minions( ) ) { // neeed to change to get enemy minions
                    if ( !obj || obj->dist_to_local( ) > 1500.f || obj->is_dead( )
                        || rt_hash( obj->get_name().data() ) != ct_hash( "ZedShadow" ) )
                        continue;

                    auto instance{ data };

                    instance.start_pos   = obj->position;
                    instance.current_pos = obj->position;
                    instance.end_pos     = obj->position.extend( sci->end_position, instance.range );
                    instance.raw_end_pos = instance.end_pos;
                    instance.missile_nid = 0;

                    m_active_spells.push_back( instance );
                }

                break;
            }
            case ct_hash( "OrianaDetonateCommand" ):
            {
                bool found_ball{ };
                for ( auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) || rt_hash( minion->name.text ) !=
                        ct_hash( "TheDoomBall" ) )
                        continue;


                    data.end_pos = minion->position;
                    found_ball   = true;
                    break;
                }

                if ( found_ball ) break;

                auto buff = g_features->buff_cache->get_buff( obj->index, ct_hash( "orianaghostself" ) );
                if ( buff ) {
                    data.end_pos          = obj->position;
                    data.should_run_check = true;
                    data.check_type       = ECheckType::orianna_self_r;
                    found_ball            = true;
                    break;
                }

                for ( auto hero : g_entity_list.get_enemies( ) ) {
                    if ( !hero || hero->is_dead( ) || hero->network_id == obj->network_id ) continue;

                    auto ball_buff = g_features->buff_cache->get_buff( hero->index, ct_hash( "orianaghost" ) );
                    if ( !ball_buff ) continue;


                    data.end_pos            = hero->position;
                    data.should_run_check   = true;
                    data.check_type         = ECheckType::orianna_ally_r;
                    data.check_target_index = hero->index;
                    found_ball              = true;
                    break;
                }

                if ( found_ball ) break;

                continue;
            }
            case ct_hash( "SylasQ" ):
            {
                auto chain{ data };

                chain.type = ESpellType::line;

                auto temp = data.start_pos.extend( data.end_pos, 125.f );

                chain.raw_end_pos = Vec3( 1, 1, 1 );
                chain.danger      = 1;

                chain.start_pos   = data.start_pos + ( temp - data.start_pos ).rotated( -1.58f );
                chain.end_pos     = chain.start_pos.extend( data.end_pos, 775.f );
                chain.current_pos = chain.start_pos;

                chain.has_edge_radius  = true;
                chain.radius           = 20.f;
                chain.end_time         = sci->server_cast_time;
                chain.server_cast_time = sci->server_cast_time;
                chain.range            = 775.f;

                m_active_spells.push_back( chain );

                chain.start_pos   = data.start_pos + ( temp - data.start_pos ).rotated( 1.58f );
                chain.end_pos     = chain.start_pos.extend( data.end_pos, 775.f );
                chain.current_pos = chain.start_pos;

                m_active_spells.push_back( chain );
                break;
            }
            case ct_hash( "MalzaharQ" ):
            {
                auto base_position = sci->end_position;
                auto delta         = ( base_position - sci->start_position ).normalize( );
                auto start_point   = base_position.extend( base_position + delta.rotated( 1.58f ), 350.f );
                auto end_point     = start_point.extend( sci->end_position, 700.f );


                auto instance{ data };

                instance.start_pos       = start_point;
                instance.current_pos     = start_point;
                instance.end_pos         = end_point;
                instance.raw_end_pos     = Vec3( 10, 10, 01 );
                instance.has_edge_radius = true;


                instance.type             = ESpellType::line;
                instance.server_cast_time = sci->server_cast_time + 0.4f;
                instance.windup_time      = sci->windup_time + 0.4f;
                instance.end_time         = instance.server_cast_time + 700.f / 1600.f;

                m_active_spells.push_back( instance );

                base_position = sci->end_position;
                delta         = ( base_position - sci->start_position ).normalize( );
                start_point   = base_position.extend( base_position + delta.rotated( -1.58f ), 350.f );
                end_point     = start_point.extend( sci->end_position, 700.f );

                instance.start_pos   = start_point;
                instance.current_pos = start_point;
                instance.end_pos     = end_point;

                m_active_spells.push_back( instance );

                data.allow_dodge = false;
                data.radius      = 0;
                data.type        = ESpellType::none;
                break;
            }
            case ct_hash( "YorickE" ):
            {
                auto start_position = sci->start_position.dist_to( sci->end_position ) > data.range
                                          ? sci->start_position.extend( sci->end_position, 700.f )
                                          : sci->end_position;

                auto end_position = start_position.extend( sci->start_position, -150.f );

                data.start_pos       = start_position.extend( sci->start_position, 125.f );
                data.current_pos     = start_position.extend( sci->start_position, 125.f );
                data.end_pos         = end_position;
                data.has_edge_radius = false;

                data.windup_time = sci->windup_time + sci->start_position.dist_to( start_position ) / 1800.f;

                data.server_cast_time = sci->start_time + data.windup_time;
                data.end_time         = data.server_cast_time;

                data.radius = 100.f;

                data.speed      = 0.f;
                data.base_speed = 0.f;

                auto hitbox = data;

                hitbox.start_pos   = data.end_pos;
                hitbox.current_pos = data.end_pos;
                hitbox.end_pos     = data.end_pos.extend( data.start_pos, -300.f );

                hitbox.radius = 125.f;

                m_active_spells.push_back( hitbox );
                break;
            }
            case ct_hash( "WildCards" ):
            {
                data.direction = ( sci->end_position - sci->start_position );

                data.end_pos = sci->start_position.extend( sci->end_position, data.range );

                auto hitbox = data;

                auto direction =
                    ( data.end_pos - data.start_pos )
                    .rotated( 27.5f * ( 3.14159265358979323846264338327950288419716939937510 / 180.f ) );
                hitbox.end_pos = data.start_pos.extend( data.start_pos + direction, data.range );

                hitbox.direction = direction;

                m_active_spells.push_back( hitbox );

                direction = ( data.end_pos - data.start_pos )
                    .rotated( -27.5f * ( 3.14159265358979323846264338327950288419716939937510 / 180.f ) );
                hitbox.end_pos   = data.start_pos.extend( data.start_pos + direction, data.range );
                hitbox.direction = direction;

                m_active_spells.push_back( hitbox );
                break;
            }
            default:
                break;
            }

            //VeigarEventHorizon
            //std::cout << "\n[ SPELL ]: Detected " << data.spell_name << " | start height: " << data.start_pos.y
            //         << " | end height: " << data.end_pos.y << " | local height: " << g_local->position.y << std::endl;

            m_ignored_missiles.push_back( { data.missile_nid, *g_time } );
            m_active_spells.push_back( data );

            switch ( name_hash ) {
            case ct_hash( "RakanWCast" ):
            {
                for ( auto& spell : m_active_spells ) {
                    if ( rt_hash( spell.spell_name.c_str( ) ) == ct_hash( "RakanW" ) ) {
                        remove_spell( spell.source_index, spell.raw_end_pos );
                        break;
                    }
                }

                break;
            }
            default:
                break;
            }
        }

        // update spells
        for (auto &spell : m_active_spells)
        {
            if (spell.is_particle_spell)
            {
                auto particle = g_entity_list.get_by_index(spell.particle_index);
                if (!particle || spell.particle_type != EParticleSpell::zac_e && spell.end_time < *g_time)
                {
                m_ignored_particles.push_back(spell.particle_nid);

                if (!spell.related_particles.empty())
                {
                    for (auto nid : spell.related_particles) m_ignored_particles.push_back(nid);
                }

                remove_spell(spell.particle_index, spell.raw_end_pos);
                }

                continue;
            }

            if (spell.end_time < *g_time)
            {
                switch (rt_hash(spell.spell_name.data()))
                {
                case ct_hash("VelkozQ"):
                {
                    auto split_missile{ spell };

                    split_missile.start_time       = *g_time;
                    split_missile.server_cast_time = *g_time + 0.25f;
                    split_missile.end_time         = *g_time + 0.25f + 1100.f / 2100.f;

                    split_missile.spell_name = "VelkozQMissile";

                    split_missile.speed         = 2100.f;
                    split_missile.base_speed    = 2100.f;
                    split_missile.range         = 1100.f;
                    split_missile.radius        = 45.f;
                    split_missile.missile_nid   = 1;
                    split_missile.missile_found = false;

                    split_missile.start_pos   = spell.end_pos.extend(spell.start_pos, -65.f);
                    split_missile.current_pos = spell.start_pos;

                    auto base_position = split_missile.start_pos;
                    auto delta         = (base_position - spell.start_pos).normalize();
                    auto split         = base_position.extend(base_position + delta.rotated(1.58f), 1100.f);
                    auto alt_split     = base_position.extend(base_position + delta.rotated(-1.58f), 1100.f);

                    split_missile.raw_end_pos     = split;
                    split_missile.end_pos         = split;
                    split_missile.has_edge_radius = true;

                    if (is_spell_active(split_missile.source_index, split_missile.server_cast_time)) break;

                    m_active_spells.push_back(split_missile);

                    split_missile.raw_end_pos = alt_split;
                    split_missile.end_pos     = alt_split;

                    m_active_spells.push_back(split_missile);

                    std::cout << "Velkoz Q: Added split missiles to evade\n";
                    break;
                }
                default:
                break;
                }

                m_ignored_missiles.push_back({ spell.missile_nid, *g_time });
                remove_spell(spell.source_index, spell.raw_end_pos);

                continue;
            }

            spell.start_pos.y   = g_navgrid->get_height(spell.start_pos);
            spell.current_pos.y = g_navgrid->get_height(spell.current_pos);
            spell.end_pos.y     = g_navgrid->get_height(spell.end_pos);

            float evade_length = 5.f + static_cast<float>(g_config->evade.extra_distance->get<int>());

            spell.update_hitbox();

            if (spell.type == ESpellType::line)
            {
                spell.hitbox_area = helper::get_missile_hitbox_polygon(
                    spell.get_current_position(), spell.get_dynamic_line_endpos(1000.f, 1000.f),
                    spell.radius + (spell.has_edge_radius ? g_features->evade->get_bounding_radius() : 0.f));

                spell.tether_area = helper::get_missile_hitbox_polygon(
                    spell.get_current_position(), spell.get_dynamic_line_endpos(1000.f, 1000.f),
                    spell.radius + (spell.has_edge_radius ? g_features->evade->get_bounding_radius() : 0.f) +
                        g_config->evade.tether_distance->get<int>() + evade_length);
            }

            spell.dodge_points = spell.type == ESpellType::circle
                ? g_render->get_3d_circle_points(
                      spell.end_pos,
                      spell.radius + (spell.has_edge_radius ? g_features->evade->get_bounding_radius() : 0.f) +
                          evade_length,
                      32)
                : helper::get_missile_hitbox_polygon(
                      spell.get_current_position(), spell.end_pos,
                      spell.radius + (spell.has_edge_radius ? g_features->evade->get_bounding_radius() : 0.f) +
                          evade_length)
                      .points;

            spell.tether_points = spell.type == ESpellType::circle
                ? spell.tether_area.points
                : helper::get_missile_hitbox_polygon(
                      spell.get_current_position(), spell.end_pos,
                      spell.radius + (spell.has_edge_radius ? g_features->evade->get_bounding_radius() : 0.f) +
                          evade_length + g_config->evade.tether_distance->get<int>())
                      .points;


            if (spell.server_cast_time < *g_time && spell.type == ESpellType::line)
            {
                auto time_traveled     = *g_time - spell.server_cast_time;
                auto distance_traveled = time_traveled * spell.base_speed;

                switch (spell.special_type)
                {
                case ESpecialSpell::ashe_r:
                {
                    auto seconds_traveled = std::min(static_cast<int>(std::floor(*g_time - spell.server_cast_time)), 3);
                    auto speed_increase   = seconds_traveled < 3 ? 200.f * seconds_traveled : 500.f;

                    auto extra_distance_traveled =
                        (time_traveled - seconds_traveled) * (spell.base_speed + speed_increase);

                    switch (seconds_traveled)
                    {
                    case 0:
                        break;
                    case 1:
                        distance_traveled = 1600.f + extra_distance_traveled;
                        spell.speed       = 1800.f;
                        break;
                    case 2:
                        distance_traveled = 1600.f + 1800.f + extra_distance_traveled;
                        spell.speed       = 2000.f;
                        break;
                    default:
                        distance_traveled = 1600.f + 1800.f + 2000.f + extra_distance_traveled;
                        spell.speed       = 2100.f;
                        break;
                    }

                    break;
                }
                case ESpecialSpell::jinx_r:
                {
                    if (distance_traveled > 1300.f)
                    {
                        if (spell.speed != 2200.f) spell.speed = 2200.f;
                        distance_traveled = 1300.f + (time_traveled - 0.765f) * 2200.f;
                    }
                    break;
                }
                default:
                break;
                }

                if (rt_hash(spell.spell_name.c_str()) == ct_hash("JavelinToss") ||
                    rt_hash(spell.spell_name.c_str()) == ct_hash("KaisaW"))
                {
                distance_traveled -= distance_traveled > 100.f ? 100.f : distance_traveled;
                }

                auto dynamic_end = spell.get_dynamic_line_endpos();

                auto current_pos = spell.start_pos.extend(spell.end_pos, distance_traveled);

                if (current_pos.dist_to(spell.start_pos) > spell.start_pos.dist_to(dynamic_end))
                current_pos = dynamic_end;

                spell.current_pos = current_pos;
            }
            else if (spell.server_cast_time >= *g_time)
            {
                auto &object = g_entity_list.get_by_index(spell.source_index);
                if (!object)
                {
                m_ignored_missiles.push_back({ spell.missile_nid, *g_time });
                remove_spell(spell.source_index, spell.raw_end_pos);
                continue;
                }

                object.update();

                if (!object || object->is_dead())
                {
                m_ignored_missiles.push_back({ spell.missile_nid, *g_time });
                remove_spell(spell.source_index, spell.raw_end_pos);
                continue;
                }

                if (spell.stick_during_cast)
                {
                auto distance   = spell.start_pos.dist_to(spell.end_pos);
                spell.start_pos = { object->position.x, g_navgrid->get_height(object->position), object->position.z };

                if (spell.is_direction_locked)
                    spell.end_pos = spell.start_pos.extend(spell.start_pos + spell.direction, distance);
                else
                {
                    if (should_fix_range(spell.spell_name))
                        spell.end_pos = spell.start_pos.extend(spell.raw_end_pos, spell.range + spell.radius);
                    else
                        spell.end_pos = spell.start_pos.extend(spell.end_pos, spell.start_pos.dist_to(spell.end_pos));
                }
                }

                if (spell.update_direction)
                {
                if (spell.raw_start_pos.dist_to(object->position) <= 10.f)
                {
                    spell.direction = object->get_direction();
                }


                auto dir = object->position + spell.direction;

                spell.start_pos   = object->position;
                spell.current_pos = object->position;
                spell.end_pos     = object->position.extend(dir, spell.range + spell.radius);
                }

                if (spell.should_run_check)
                {
                switch (spell.check_type)
                {
                case ECheckType::jarvan_iv_q:
                    {
                        int16_t beacon_index{};
                        float   furthest_beacon{};
                        for (auto unit : g_entity_list.get_enemy_minions())
                        {
                            if (!unit || unit->is_dead() || unit->position.dist_to(object->position) > 1000.f ||
                                unit->get_name().find(_("JarvanIVStandard")) == std::string::npos)
                                continue;

                            auto hitbox = sdk::math::Rectangle(object->position,
                                                               object->position.extend(spell.end_pos, 915.f), 150.f);

                            auto poly = hitbox.to_polygon(65);

                            if (!poly.is_inside(unit->position)) continue;

                            auto closest_point = g_features->evade->get_closest_line_point(
                                object->position, spell.end_pos, unit->position);
                            auto dist = closest_point.dist_to(object->position);

                            if (dist < furthest_beacon || closest_point.dist_to(unit->position) > 215.f) continue;

                            beacon_index    = unit->index;
                            furthest_beacon = dist;
                        }

                        if (beacon_index == 0) break;

                        auto &unit = g_entity_list.get_by_index(beacon_index);
                        if (!unit) break;

                        SpellInstance instance{};

                        spell.should_run_check = false;

                        instance.start_pos   = object->position;
                        instance.current_pos = object->position;
                        instance.raw_end_pos = unit->position;
                        instance.end_pos     = unit->position;

                        instance.speed           = 2500.f;
                        instance.base_speed      = 2500.f;
                        instance.danger          = 4;
                        instance.cc              = true;
                        instance.collision       = false;
                        instance.radius          = 180.f;
                        instance.type            = ESpellType::line;
                        instance.range           = 900.f;
                        instance.has_edge_radius = false;

                        instance.spell_name       = _("JarvanIVBeaconDash");
                        instance.start_time       = *g_time;
                        instance.server_cast_time = spell.server_cast_time;
                        instance.end_time =
                            spell.server_cast_time + object->position.dist_to(unit->position) / instance.speed;

                        instance.source_index = object->index;

                        instance.windup_time     = spell.windup_time;
                        instance.total_cast_time = spell.total_cast_time;
                        m_active_spells.push_back(instance);


                        auto circle_instance{ instance };

                        circle_instance.type        = ESpellType::circle;
                        circle_instance.end_pos     = unit->position;
                        circle_instance.speed       = 0.f;
                        circle_instance.base_speed  = 0.f;
                        circle_instance.raw_end_pos = unit->position + sdk::math::Vec3(1, 1, 1);

                        m_active_spells.push_back(circle_instance);
                        break;
                    }
                case ECheckType::orianna_self_r:
                    {
                        spell.end_pos = object->position;
                        break;
                    }
                case ECheckType::orianna_ally_r:
                    {
                        auto unit = g_entity_list.get_by_index(spell.check_target_index);
                        if (!unit || !unit->is_hero()) break;

                        unit.update();

                        auto predict_amount =
                            spell.server_cast_time > *g_time + 0.1f ? 0.1f : spell.server_cast_time - *g_time;

                        auto pred = g_features->prediction->predict_default(unit->index, predict_amount);
                        if (!pred)
                        {
                            spell.end_pos = unit->position;
                            break;
                        }

                        spell.end_pos = pred.value();
                        break;
                    }
                default:
                    break;
                }
                }
            }

            if (spell.stick_full_duration)
            {
                auto object = g_entity_list.get_by_index(spell.source_index);
                if (!object) continue;

                object.update();

                if (spell.is_hitbox_relative || *g_time < spell.server_cast_time)
                {
                spell.base_position = { object->position.x, g_navgrid->get_height(object->position),
                                        object->position.z };

                auto relative_pos = spell.base_position;

                const auto end_time = spell.is_hitbox_relative ? spell.end_time : spell.server_cast_time;

                auto pred = g_features->prediction->predict_default(object->index, end_time - *g_time);
                if (pred) relative_pos = *pred;

                spell.start_pos   = relative_pos.extend(relative_pos + spell.direction, spell.base_distance_to_start);
                spell.current_pos = relative_pos.extend(relative_pos + spell.direction, spell.base_distance_to_start);
                spell.end_pos     = relative_pos.extend(relative_pos + spell.direction, spell.base_distance_to_end);
                }
            }

            if (!spell.missile_found)
            {
                if (spell.missile_nid == 0) continue;

                for (auto missile :
                     m_detect_ally_spells ? g_entity_list.get_ally_missiles() : g_entity_list.get_enemy_missiles())
                {
                if (!missile) continue;
                if (missile->network_id != spell.missile_nid)
                {
                    if (spell.spell_name.find("VelkozQ") == std::string::npos ||
                        spell.end_pos.dist_to(missile->missile_end_position) > 250.f)
                        continue;

                    auto info = missile->missile_spell_info();
                    if (!info) continue;

                    auto data = info->get_spell_data();
                    if (!data) continue;

                    if (data->get_name().find(spell.spell_name) == std::string::npos) continue;
                }

                spell.missile_found = true;
                spell.mis_index     = missile->index;

                m_ignored_missiles.push_back({ missile->network_id, *g_time });
                std::cout << "found missile for: " << spell.spell_name << std::endl;

                break;
                }
            }
            else
            {
                auto obj = g_entity_list.get_by_index(spell.mis_index);
                if (!obj)
                {
                switch (rt_hash(spell.spell_name.data()))
                {
                case ct_hash("VelkozQ"):
                    {
                        auto split_missile{ spell };

                        split_missile.start_time       = *g_time;
                        split_missile.server_cast_time = *g_time + 0.25f;
                        split_missile.end_time         = *g_time + 0.25f + 1100.f / 2100.f;

                        split_missile.spell_name = "VelkozQMissile";

                        split_missile.speed         = 2100.f;
                        split_missile.base_speed    = 2100.f;
                        split_missile.range         = 1100.f;
                        split_missile.radius        = 45.f;
                        split_missile.missile_nid   = 1;
                        split_missile.missile_found = false;

                        split_missile.start_pos   = spell.mis_last_position;
                        split_missile.current_pos = spell.mis_last_position;

                        auto base_position = spell.mis_last_position;
                        auto delta         = (base_position - spell.start_pos).normalize();
                        auto split         = base_position.extend(base_position + delta.rotated(1.58f), 1100.f);
                        auto alt_split     = base_position.extend(base_position + delta.rotated(-1.58f), 1100.f);

                        split_missile.raw_end_pos     = split;
                        split_missile.end_pos         = split;
                        split_missile.has_edge_radius = true;

                        if (is_spell_active(split_missile.source_index, split_missile.server_cast_time)) break;

                        m_active_spells.push_back(split_missile);

                        split_missile.raw_end_pos = alt_split;
                        split_missile.end_pos     = alt_split;

                        m_active_spells.push_back(split_missile);

                        std::cout << "Velkoz Q: Added split missiles to evade\n";
                        break;
                    }
                default:
                    break;
                }


                m_ignored_missiles.push_back({ spell.missile_nid, *g_time });
                remove_spell(spell.source_index, spell.raw_end_pos);
                continue;
                }

                obj.update();

                if (spell.mis_last_position.length() > 0.f)
                {
                if (obj->position.dist_to(spell.mis_last_position) > 500.f)
                {
                    m_ignored_missiles.push_back({ spell.missile_nid, *g_time });
                    remove_spell(spell.source_index, spell.raw_end_pos);
                    continue;
                }
                }

                spell.mis_last_position = obj->position;
            }
        }


        // add new missiles
        for ( auto missile_obj : m_detect_ally_missiles
                                     ? g_entity_list.get_ally_missiles( )
                                     : g_entity_list.get_enemy_missiles( ) ) {
            if ( !missile_obj || is_ignored_missile( missile_obj->network_id ) || is_missile_active(
                    missile_obj->network_id
                )
                || is_missile_from_spell( missile_obj->network_id ) )
                continue;

            const auto spellinfo = missile_obj->missile_spell_info( );
            if ( !spellinfo ) continue;

            const auto spelldata = spellinfo->get_spell_data( );
            if ( !spelldata ) continue;

            const auto spell_name = spelldata->get_name( );
            auto       data       = get_data( rt_hash( spell_name.data() ) );
            if ( data.type != ESpellType::line && data.type != ESpellType::circle ) {
                m_ignored_missiles.push_back( { missile_obj->network_id, *g_time } );
                continue;
            }

            bool is_lillia_e{ };

            auto missile = g_entity_list.get_by_index( missile_obj->index );
            if ( !missile ) continue;

            missile.update( );

            if ( std::isinf( missile->position.length( ) ) ) continue;

            for ( const auto& spell : m_active_spells ) {
                if ( rt_hash( spell.spell_name.c_str() ) == ct_hash( "LilliaE" ) && rt_hash( spell_name.c_str() ) ==
                    ct_hash( "LilliaERollingMissile" )
                    && spell.raw_end_pos.dist_to( missile->missile_start_position ) <= 75.f ) {
                    is_lillia_e = true;
                    break;
                }

                if ( rt_hash( spell.spell_name.c_str( ) ) == ct_hash( "YoneQ3" ) &&
                    rt_hash( spell_name.c_str( ) ) == ct_hash( "YoneQ3Missile" ) ) {
                    m_ignored_missiles.push_back( { missile->network_id, *g_time } );
                    break;
                }

                if ( spell.start_pos.dist_to( missile->missile_start_position ) <= 10.f ) {
                    m_ignored_missiles.push_back( { missile->network_id, *g_time } );
                    break;
                }
            }

            if ( is_ignored_missile( missile->network_id ) ) continue;

            MissileInstance inst{ };

            inst.index          = missile->index;
            inst.network_id     = missile->network_id;
            inst.position       = missile->position;
            inst.start_position = missile->missile_start_position;
            inst.end_position   = missile->missile_end_position;

            inst.speed       = data.speed;
            inst.range       = data.range;
            inst.radius      = data.radius;
            inst.danger      = data.danger;
            inst.windup_time = data.windup_time;
            inst.type        = data.type;
            inst.name        = spell_name;
            inst.cc          = data.cc;
            inst.collision   = data.collision;

            const auto name_hash = rt_hash( spell_name.data( ) );

            inst.has_edge_radius = has_edge_range( name_hash );

            auto include_radius{ true };
            switch ( name_hash ) {
            case ct_hash( "ViktorDeathRayMissile2" ):
            {
                for ( const auto instance : m_active_missiles ) {
                    if ( rt_hash( instance.name.data() ) == ct_hash( "ViktorEAugParticle" ) ) {
                        m_ignored_missiles.push_back( { instance.network_id, *g_time } );
                        remove_missile( instance.network_id );
                        break;
                    }
                }
                break;
            }
            case ct_hash( "HowlingGaleSpell" ):
            {
                inst.speed = missile->missile_start_position.dist_to( missile->missile_end_position ) / 1.25f;
                inst.range = missile->missile_start_position.dist_to( missile->missile_end_position );
                break;
            }
            case ct_hash( "EnchantedCrystalArrow" ):
            {
                inst.special_type = ESpecialSpell::ashe_r;
                break;
            }
            case ct_hash( "JinxR" ):
            {
                inst.special_type = ESpecialSpell::jinx_r;
                break;
            }
            case ct_hash( "ViegoWMis" ):
                include_radius = false;
                break;
            case ct_hash( "VexE" ):
                inst.radius = 175.f + inst.start_position.dist_to( inst.end_position ) / 800.f * 100.f;
                break;
            case ct_hash( "VexR" ):
                inst.range = inst.start_position.dist_to( inst.end_position );
                break;
            case ct_hash( "SyndraESphereMissile" ):
            {
                for ( const auto spell : m_active_spells ) {
                    if ( rt_hash( spell.spell_name.data() ) != ct_hash( "SyndraESphereMissile" ) ) continue;

                    if ( spell.start_pos.dist_to( missile->missile_start_position ) <= 80.f ) {
                        m_ignored_missiles.push_back( { missile->network_id, *g_time } );
                        break;
                    }
                }
                break;
            }
            default:
                break;
            }

            if ( is_ignored_missile( missile->network_id ) ) continue;

            if ( inst.type == ESpellType::line ) {
                if ( should_fix_range( inst.name ) ) {
                    inst.end_position = inst.start_position.extend( inst.end_position, inst.range + inst.radius );
                } else {
                    inst.end_position = inst.start_position.extend(
                        inst.end_position,
                        inst.start_position.dist_to( inst.end_position ) +
                        ( include_radius ? inst.radius : 0.f )
                    );
                }
            }

            auto modifier = missile->position.dist_to( inst.start_position ) / inst.start_position.dist_to(
                inst.end_position
            );
            inst.position = inst.start_position.extend(
                inst.end_position,
                inst.start_position.dist_to( inst.end_position ) * modifier
            );

            inst.start_time = *g_time;

            if ( inst.speed >
                0 ) { inst.end_time = *g_time + inst.position.dist_to( inst.end_position ) / inst.speed; } else {
                if ( get_manual_endtime( spelldata->get_name( ) ).has_value( ) )
                    inst.end_time = missile->missile_spawn_time( ) + get_manual_endtime( spelldata->get_name( ) ).value( );
                else inst.end_time = *g_time + 2.5f;
            }

            auto spell_config = get_spell_config_value( rt_hash( spell_name.c_str() ) );
            if ( spell_config.valid ) {
                inst.allow_dodge             = spell_config.enabled;
                inst.danger                  = spell_config.danger_level;
                inst.ignore_health_threshold = static_cast< float >( spell_config.max_health_to_ignore );
            }

            switch ( rt_hash( spell_name.data() ) ) {
            case ct_hash( "IreliaEMissile" ):
            {
                bool     found_start{ };
                Vec3     start_position{ };
                float    second_dist_to_end{ };
                unsigned second_nid{ };
                bool     second_is_minion{ };

                for ( auto unit : g_entity_list.get_enemy_missiles( ) ) {
                    if ( !unit || unit->position.dist_to( missile->position ) > 2000.f || unit->network_id == missile->
                        network_id )
                        continue;

                    auto sinfo = unit->missile_spell_info( );
                    if ( !sinfo ) continue;

                    auto sdata = sinfo->get_spell_data( );
                    if ( !sdata ) continue;

                    auto name = sdata->get_name( );
                    if ( name.find( "IreliaEMissile" ) == std::string::npos ) continue;


                    auto& missile_object = g_entity_list.get_by_index( unit->index );
                    if ( !missile_object ) continue;

                    missile_object.update( );

                    start_position     = missile_object->missile_end_position;
                    second_dist_to_end = missile_object->position.dist_to( missile_object->missile_end_position );
                    found_start        = true;
                    second_nid         = missile_object->network_id;
                    break;
                }

                if ( !found_start ) {
                    for ( auto unit : g_entity_list.get_enemy_minions( ) ) {
                        if ( !unit || unit->is_dead( ) || rt_hash( unit->name.text ) != ct_hash( "Blade" ) ) continue;

                        second_dist_to_end = 0.f;
                        start_position     = unit->position;
                        found_start        = true;
                        second_is_minion   = true;
                        break;
                    }

                    if ( !found_start ) continue;
                }

                auto is_primary_missile = second_is_minion || missile->position.dist_to( missile->missile_end_position )
                                          > second_dist_to_end
                                              ? true
                                              : false;
                auto dist_to_end = is_primary_missile
                                       ? missile->position.dist_to( missile->missile_end_position )
                                       : second_dist_to_end;

                inst.start_position = is_primary_missile ? start_position : missile->missile_end_position;
                inst.position       = inst.start_position;

                inst.end_position = is_primary_missile ? missile->missile_end_position : start_position;

                inst.start_time     = *g_time;
                inst.end_time       = *g_time + ( dist_to_end / 2000.f + 0.265f );
                inst.ignore_missile = true;

                if ( std::isinf( inst.end_time ) || std::isnan( inst.end_time ) ) {
                    // debug_log( "\n[ SPELL DETECTOR ] IRELIA E INVALID MISSILE DETECTED! ----" );
                    // debug_log(
                    //     "1. POSITION x: {} y: {} z: {}",
                    //     missile->position.x,
                    //     missile->position.y,
                    //     missile->position.z
                    // );
                    // debug_log( "2. LENGTH: {} 2D: {}", missile->position.length( ), missile->position.length2d( ) );

                    auto& upd_missile = g_entity_list.get_by_index( missile->index );
                    if ( !upd_missile ) continue;

                    upd_missile.update( );

                    dist_to_end   = upd_missile->position.dist_to( upd_missile->missile_end_position );
                    inst.end_time = *g_time + ( dist_to_end / 2000.f + 0.265f );

                    if ( std::isinf( inst.end_time ) || std::isnan( inst.end_time ) ) {
                        debug_log( "[ SPELL DETECTOR ] IRELIA E: missile update failed" );
                        continue;
                    }
                }

                m_ignored_missiles.push_back( { missile->network_id, *g_time } );
                m_ignored_missiles.push_back( { second_nid, *g_time } );

                inst.radius = g_features->evade->get_bounding_radius( );
                inst.range  = 0.f;
                inst.speed  = 0.f;

                inst.end_position = inst.start_position.extend(
                    inst.end_position,
                    inst.start_position.dist_to( inst.end_position ) + 80.f
                );
                inst.start_position = inst.end_position.extend(
                    inst.start_position,
                    inst.start_position.dist_to( inst.end_position ) + 80.f
                );
                inst.position = inst.start_position;

                break;
            }
            case ct_hash( "ZoeEMisAudio" ):
            {
                for ( const auto instance : m_active_missiles ) {
                    if ( rt_hash( instance.name.data( ) ) == ct_hash( "ZoeE" ) ) {
                        m_ignored_missiles.push_back( { instance.network_id, *g_time } );
                        remove_missile( instance.network_id );
                        break;
                    }
                }

                inst.start_position = missile->missile_start_position.extend( inst.end_position, -300.f );
                inst.position       = missile->position.extend( inst.end_position, -300.f );
                inst.end_position   = missile->missile_end_position.extend( missile->missile_start_position, 400.f );

                inst.end_position.y = g_navgrid->get_height( inst.end_position );
                inst.position.y     = g_navgrid->get_height( inst.position );

                inst.speed = 1900.f;

                inst.end_time = *g_time + inst.position.dist_to( inst.end_position ) / inst.speed;

                inst.manual_update = true;
            }
            case ct_hash( "ZoeQMis2Warning" ):
            {
                inst.start_position = missile->missile_start_position.extend( inst.end_position, -450.f );
                inst.position       = missile->position.extend( inst.end_position, -450.f );
                //inst.end_position   = missile->missile_end_position.extend( missile->missile_start_position, 400.f );

                inst.end_position.y = g_navgrid->get_height( inst.end_position );
                inst.position.y     = g_navgrid->get_height( inst.position );

                inst.end_time = *g_time + inst.position.dist_to( inst.end_position ) / inst.speed;

                inst.manual_update = true;
            }
            default:
                inst.end_position.y = g_navgrid->get_height( inst.end_position );
                break;
            }

            //debug_log( "[ MISSILE ] {} | END IN: {}", inst.name, inst.end_time - *g_time );

            m_active_missiles.push_back( inst );

            if ( is_lillia_e ) {
                for ( auto& spell : m_active_spells ) {
                    if ( rt_hash( spell.spell_name.c_str() ) == ct_hash( "LilliaE" ) ) {
                        remove_spell( spell.source_index, spell.raw_end_pos );
                        break;
                    }
                }
            }
        }

        // enemy dash detection
        for ( auto enemy : m_detect_ally_spells ? g_entity_list.get_allies( ) : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

            auto aimgr = enemy->get_ai_manager( );
            if ( !aimgr ) continue;

            aimgr.update( );
            if ( !aimgr->is_moving || !aimgr->is_dashing ) continue;

            auto dash_spell = get_dash_spell( enemy->index );
            if ( dash_spell.type == ESpellType::none ||
                is_spell_active( enemy->index, dash_spell.server_cast_time ) )
                continue;

            auto inst{ dash_spell };

            inst.start_pos   = aimgr->path_start;
            inst.current_pos = enemy->position + aimgr->velocity;
            inst.end_pos     = aimgr->path_end;
            inst.raw_end_pos = inst.end_pos;

            inst.start_time       = dash_spell.server_cast_time;
            inst.server_cast_time = dash_spell.server_cast_time;
            inst.end_time         = dash_spell.server_cast_time + enemy->position.dist_to( aimgr->path_end ) / aimgr->
                dash_speed;

            inst.range = aimgr->path_start.dist_to( aimgr->path_end );

            inst.speed      = aimgr->dash_speed;
            inst.base_speed = inst.speed;

            switch ( rt_hash( inst.spell_name.data( ) ) ) {
            case ct_hash( "GragasE" ):
            {
                inst.end_pos = aimgr->path_start.extend( aimgr->path_end, 825.f );
                break;
            }
            case ct_hash( "ViQ" ):
            {
                inst.end_pos = aimgr->path_start.extend(
                    aimgr->path_end,
                    aimgr->path_start.dist_to( aimgr->path_end ) + inst.radius + g_features->evade->
                    get_bounding_radius( )
                );
                break;
            }
            case ct_hash( "KaynQ" ):
            {
                auto dash_inst{ inst };

                dash_inst.type            = ESpellType::line;
                dash_inst.speed           = aimgr->dash_speed;
                dash_inst.radius          = 60.f;
                dash_inst.has_edge_radius = true;
                dash_inst.base_speed      = dash_inst.speed;
                dash_inst.raw_end_pos.x += 100.f;

                m_active_spells.push_back( dash_inst );


                inst.radius          = 300.f;
                inst.has_edge_radius = true;
                inst.end_time += 0.15f;
                break;
            }
            case ct_hash( "UFSlash" ):
            {
                //m_malphite_r_time = inst.server_cast_time;

                inst.danger          = get_spell_config_value( ct_hash( "UFSlash" ) ).danger_level;
                inst.speed           = aimgr->dash_speed;
                inst.base_speed      = inst.speed;
                inst.range           = 1000.f;
                inst.cc              = true;
                inst.collision       = false;
                inst.radius          = 325.f;
                inst.type            = ESpellType::circle;
                inst.has_edge_radius = false;
                break;
            }
            default:
                break;
            }


            inst.source_index = enemy->index;

            std::cout << "[ OnProcess ] Added dash spell " << inst.spell_name << " | " << *g_time << std::endl;

            m_active_spells.push_back( inst );
        }

        // add new particles
        // if ( false ) {
        //     for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
        //         if ( !particle || is_ignored_particle( particle->network_id ) ) continue;
        //
        //         if ( *g_time - m_first_run_time <= 1.f ) {
        //             m_ignored_particles.push_back( particle->network_id );
        //             continue;
        //         }
        //
        //         auto data = get_particle_spell( particle->get_alternative_name( ), particle->position );
        //
        //         if ( data.type == ESpellType::none || data.particle_type == EParticleSpell::none ||
        //             is_spell_active( data.particle_index, 0.f, particle->position, true ) )
        //             continue;
        //
        //         data.is_particle_spell = true;
        //         data.particle_index    = particle->index;
        //         data.particle_nid      = particle->network_id;
        //
        //         std::cout << "[ PARTICLE ] Added " << data.spell_name << std::endl;
        //         m_active_spells.push_back( data );
        //         m_ignored_particles.push_back( particle->network_id );
        //     }
        // }

        // add new objects/traps
        for ( auto inst : g_features->tracker->get_wards( ) ) {
            //if  continue;
            switch ( inst.type ) {
            case Object::EWardType::caitlyn_trap:
            case Object::EWardType::jhin_trap:
            case Object::EWardType::teemo_shroom:
            case Object::EWardType::nidalee_trap:
                break;
            default:
                continue;
            }

            auto object = g_entity_list.get_by_index( inst.index );
            if ( !object || is_object_active( object->network_id ) || object->is_dead( ) ) continue;


            auto data = get_object_data( rt_hash( object->get_name().c_str() ) );
            if ( data.type == ESpellType::none ) {
                m_ignored_objects.push_back( { object->network_id, *g_time } );
                continue;
            }

            if ( inst.type == Object::EWardType::caitlyn_trap )
                data.radius += g_features->orbwalker->
                                           get_bounding_radius( );


            data.index      = object->index;
            data.network_id = object->network_id;
            data.position   = object->position;
            data.start_time = *g_time;

            m_active_objects.push_back( data );
        }

        // update objects/traps
        for ( auto& inst : m_active_objects ) {
            auto object = g_entity_list.get_by_index( inst.index );
            if ( !object || object->is_dead( ) || object->team == 300 || object->team == g_local->team ) {
                m_ignored_objects.push_back( { inst.network_id, *g_time } );
                remove_object( inst.network_id );
                continue;
            }

            object.update( );

            if ( object->position.dist_to( inst.position ) >= 1.f ) inst.position = object->position;
        }

        // update missiles
        for ( auto& inst : m_active_missiles ) {
            if ( inst.ignore_missile ) {
                if ( inst.end_time <= *g_time ) {
                    m_ignored_missiles.push_back( { inst.network_id, *g_time } );
                    remove_missile( inst.network_id );
                }

                continue;
            }

            auto& obj = g_entity_list.get_by_index( inst.index );
            if ( !obj ) {
                m_ignored_missiles.push_back( { inst.network_id, *g_time } );
                remove_missile( inst.network_id );
                continue;
            }

            obj.update( );

            if ( !m_detect_ally_missiles && obj->team == g_local->team || inst.end_time <= *g_time ||
                is_ignored_missile( obj->network_id ) ) {
                m_ignored_missiles.push_back( { inst.network_id, *g_time } );
                remove_missile( inst.network_id );
                continue;
            }

            if ( inst.type == ESpellType::line ) {
                auto time_traveled     = *g_time - obj->missile_spawn_time( );
                auto distance_traveled = time_traveled * inst.speed;

                switch ( inst.special_type ) {
                case ESpecialSpell::ashe_r:
                {
                    auto seconds_traveled = std::min(
                        static_cast< int >( std::floor( *g_time - inst.start_time ) ),
                        3
                    );
                    auto speed_increase = seconds_traveled < 3 ? 200.f * seconds_traveled : 500.f;

                    auto extra_distance_traveled = ( time_traveled - seconds_traveled ) * ( inst.speed +
                        speed_increase );


                    switch ( seconds_traveled ) {
                    case 0:
                        break;
                    case 1:
                        distance_traveled = 1600.f + extra_distance_traveled;
                        break;
                    case 2:
                        distance_traveled = 1600.f + 1800.f + extra_distance_traveled;
                        break;
                    default:
                        distance_traveled = 1600.f + 1800.f + 2000.f + extra_distance_traveled;
                        break;
                    }

                    break;
                }
                case ESpecialSpell::jinx_r:
                {
                    if ( distance_traveled > 1300.f ) distance_traveled = 1300.f + ( time_traveled - 0.765f ) * 2200.f;
                    break;
                }
                default:
                    break;
                }


                if ( inst.manual_update ) {
                    if ( distance_traveled > 5000.f ) {
                        std::cout << "[ " << inst.name << " ] Invalid length: " << distance_traveled
                            << " | base length: " << inst.start_position.dist_to( inst.end_position )
                            << std::endl;

                        m_ignored_missiles.push_back( { inst.network_id, *g_time } );
                        remove_missile( inst.network_id );
                        continue;
                    }

                    inst.position = inst.start_position.extend( inst.end_position, distance_traveled );

                    continue;
                }


                auto modifier = std::min(
                    obj->position.dist_to( inst.start_position ) / inst.start_position.dist_to( inst.end_position ),
                    1.f
                );
                inst.position = inst.start_position.extend(
                    inst.end_position,
                    inst.start_position.dist_to( inst.end_position ) * modifier
                );

                auto travel_time = obj->missile_start_position.dist_to( obj->missile_end_position ) / inst.speed;

                if ( obj->missile_spawn_time( ) + travel_time != inst.end_time - 0.5f && inst.special_type ==
                    ESpecialSpell::none ) {
                    // std::cout << "[ Missile " << inst.name << " ] Endtime updated: " << obj->missile_spawn_time() + travel_time << " | OLD: " << inst.end_time << std::endl;
                    inst.end_time = obj->missile_spawn_time( ) + travel_time + 0.5f;
                }

                auto dist = obj->position.dist_to( inst.end_position );
                if ( inst.is_initialized && dist > inst.distance_to_end + 100.f ) {
                    //std::cout << "[ MIS: " << inst.name << " ] Removed due to end distance: " << dist << " | " << inst.distance_to_end << std::endl;
                    m_ignored_missiles.push_back( { inst.network_id, *g_time } );
                    remove_missile( inst.network_id );
                    continue;
                } else if ( dist < inst.distance_to_end ) inst.distance_to_end = dist;

                if ( !inst.is_initialized && dist < inst.start_position.dist_to( inst.end_position ) ) {
                    inst.distance_to_end = dist;
                    inst.is_initialized  = true;
                }
            }
        }
    }

    auto SpellDetector::enable_circle_debug_spell( ) -> void{
        if ( *g_time - m_last_debug_time2 <= 5.f ) return;

        SpellInstance spell{ };

        spell.start_time       = *g_time;
        spell.server_cast_time = *g_time + 0.25f;

        spell.start_pos   = Vec3( 1804.f, 0.f, 3102.f );
        spell.current_pos = spell.start_pos;
        spell.end_pos     = spell.start_pos.extend( Vec3( 1920, 0.f, 3152.f ), 575.f );
        spell.raw_end_pos = Vec3( 1920, 0.f, 3155.f );

        spell.danger          = 3;
        spell.speed           = 0.f;
        spell.range           = 1000.f;
        spell.radius          = 350.f;
        spell.missile_nid     = 0;
        spell.windup_time     = 1.f;
        spell.source_index    = g_local->index;
        spell.has_edge_radius = true;

        spell.end_time = spell.server_cast_time + 4.f;
        spell.type     = ESpellType::circle;


        debug_log( "added dummy circle spell {}", *g_time );

        m_last_debug_time2 = *g_time;
        m_active_spells.push_back( spell );
    }

    auto SpellDetector::enable_linear_debug_spell( ) -> void{
        if ( *g_time - m_last_debug_time <= 5.f ) return;

        SpellInstance spell{ };

        spell.start_time       = *g_time;
        spell.server_cast_time = *g_time + 0.25f;

        spell.start_pos   = Vec3( 1804.f, 0.f, 3102.f );
        spell.current_pos = spell.start_pos;
        spell.end_pos     = spell.start_pos.extend( Vec3( 1920, 0.f, 3152.f ), 1150.f );
        spell.raw_end_pos = Vec3( 1920, 0.f, 3152.f );

        spell.danger          = 3;
        spell.speed           = 0.f;
        spell.range           = 1150.f;
        spell.radius          = 60.f;
        spell.missile_nid     = 0;
        spell.windup_time     = 1.f;
        spell.source_index    = g_local->index;
        spell.has_edge_radius = true;

        spell.end_time = spell.server_cast_time + 4.f;
        spell.type     = ESpellType::line;

        debug_log( "added dummy line spell {}", *g_time );

        m_last_debug_time = *g_time;
        m_active_spells.push_back( spell );
    }

    auto SpellDetector::simulate_spells( ) -> void{
        if ( !g_config->evade.simulate_spells->get< bool >( ) || *g_time - m_last_simulation_time <= g_config->evade.
            simulation_delay->get< int >( ) * 0.1f )
            return;

        auto max_index        = 2;
        auto spell_randomized = rand( ) % max_index + 1;

        hash_t      spell_hash{ };
        std::string spell_name{ };

        switch ( spell_randomized ) {
        default:
            spell_hash = ct_hash( "EzrealQ" );
            spell_name = "EzrealQ";
            break;
        case 1:
            spell_hash = ct_hash( "ZyraE" );
            spell_name = "ZyraE";
            break;
        case 2:
            spell_hash = ct_hash( "EzrealR" );
            spell_name = "EzrealR";
            break;
        case 3:
            spell_hash = ct_hash( "NamiQ" );
            spell_name = "NamiQ";
            break;
        case 4:
            std::cout << "[ SIM: Index too high " << spell_randomized << " ]\n";
            return;
        }

        auto spell{ get_data( spell_hash ) };
        if ( spell_hash == ct_hash( "EzrealR" ) ) spell.range = 1500.f;

        spell.has_edge_radius = has_edge_range( spell_hash );

        auto pred_amount = 0.25f;

        switch ( spell.type ) {
        case ESpellType::line:
        {
            auto radius =
                spell.radius + ( spell.has_edge_radius ? g_features->orbwalker->get_bounding_radius( ) : 0.f );
            auto modified_windup = radius / g_local->movement_speed;

            spell.windup_time = modified_windup;

            spell.start_time       = *g_time;
            spell.server_cast_time = *g_time + spell.windup_time;

            spell.base_speed   = spell.speed;
            spell.missile_nid  = 0;
            spell.source_index = g_local->index;

            if ( spell.speed > 0.f ) spell.end_time = spell.server_cast_time + spell.range / spell.speed;
            else if ( get_manual_endtime( spell_name ).has_value( ) )
                spell.end_time = spell.server_cast_time +
                    get_manual_endtime( spell_name ).value( );
            else spell.end_time = spell.server_cast_time;

            auto random               = rand( ) % 360;
            auto distance_random      = rand( ) % 65 + 30;
            auto direction_randomized = rand( ) % 8;

            auto       local_position = g_local->position;
            const auto pred           = g_features->prediction->predict_movement( g_local->index, pred_amount );
            if ( pred.has_value( ) ) local_position = *pred;

            auto direction = local_position.rotated_raw( random );

            auto start_position =
                local_position.extend( local_position + direction, spell.range * ( distance_random / 100.f ) );

            auto points = g_render->get_3d_circle_points( local_position, radius * 0.5f, 8 );

            auto end_position = start_position.extend( points[ direction_randomized ], spell.range );

            spell.start_pos   = start_position;
            spell.current_pos = start_position;
            spell.end_pos     = end_position;
            spell.raw_end_pos = end_position;

            m_active_spells.push_back( spell );

            break;
        }
        case ESpellType::circle:
        {
            auto radius =
                spell.radius + ( spell.has_edge_radius ? g_features->orbwalker->get_bounding_radius( ) : 0.f );
            auto modified_windup = radius / g_local->movement_speed;

            //spell.windup_time = modified_windup;

            spell.start_time       = *g_time;
            spell.server_cast_time = *g_time + spell.windup_time;

            spell.base_speed   = spell.speed;
            spell.missile_nid  = 0;
            spell.source_index = g_local->index;

            if ( spell.speed > 0.f ) spell.end_time = spell.server_cast_time + spell.range / spell.speed;
            else if ( get_manual_endtime( spell_name ).has_value( ) )
                spell.end_time = spell.server_cast_time +
                    get_manual_endtime( spell_name ).value( );
            else spell.end_time = spell.server_cast_time;

            auto random               = rand( ) % 360;
            auto distance_random      = rand( ) % 65 + 30;
            auto direction_randomized = rand( ) % 8;

            auto local_position = g_local->position;
            auto pred           = g_features->prediction->predict_default( g_local->index, pred_amount, false );
            if ( pred.has_value( ) ) local_position = *pred;

            auto direction = local_position.rotated_raw( random );

            auto start_position =
                local_position.extend( local_position + direction, spell.range * ( distance_random / 100.f ) );

            auto points = g_render->get_3d_circle_points( local_position, radius / 2.f, 8 );

            auto end_position = points[ direction_randomized ];

            spell.start_pos   = start_position;
            spell.current_pos = start_position;
            spell.end_pos     = end_position;
            spell.raw_end_pos = end_position;

            m_active_spells.push_back( spell );
            break;
        }
        default:
            return;
        }

        std::cout << "[ EVADE: Simulation ] Added new simulation " << *g_time << " | sim index: " << spell_randomized <<
            std::endl;
        m_last_simulation_time = *g_time;
    }

    auto SpellDetector::is_spell_active(
        const int32_t index,
        const float   server_cast_time,
        const Vec3&   raw_end_pos,
        const bool    is_particle
    ) -> bool{
        return std::ranges::find_if(
            m_active_spells,
            [&]( const SpellInstance& spell ) -> bool{
                return is_particle && spell.particle_index == index || !is_particle && spell.source_index == index && (
                    spell.server_cast_time == server_cast_time || spell.raw_end_pos.dist_to( raw_end_pos ) <= 5.f );
            }
        ) != m_active_spells.end( );
    }

    auto SpellDetector::is_missile_active( unsigned nid ) -> bool{
        return std::ranges::find_if(
            m_active_missiles,
            [nid]( const MissileInstance& missile ) -> bool{ return missile.network_id == nid; }
        ) != m_active_missiles.end( );
    }

    auto SpellDetector::is_object_active( const unsigned nid ) const -> bool{
        for ( const auto inst : m_active_objects ) { if ( inst.network_id == nid ) return true; }

        return false;
    }

    auto SpellDetector::is_ignored_missile( unsigned nid ) -> bool{
        return std::ranges::find_if(
            m_ignored_missiles,
            [nid]( const IgnoredMissile& inst ) -> bool{ return inst.network_id == nid; }
        ) != m_ignored_missiles.end( );
    }

    auto SpellDetector::is_missile_from_spell( unsigned nid ) -> bool{
        return std::ranges::find_if(
            m_active_spells,
            [nid]( const SpellInstance& spell ) -> bool{ return spell.missile_nid == nid; }
        ) != m_active_spells.end( );
    }


    auto SpellDetector::remove_missile( const unsigned nid ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_active_missiles,
            [&]( const MissileInstance& missile ) -> bool{ return missile.network_id == nid; }
        );

        if ( to_remove.empty( ) ) return;

        m_active_missiles.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto SpellDetector::remove_old_ignored_missiles( ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_ignored_missiles,
            [&]( const IgnoredMissile& inst ) -> bool{ return *g_time - inst.start_time > 10.f; }
        );

        if ( to_remove.empty( ) ) return;

        m_ignored_missiles.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto SpellDetector::remove_old_ignored_spells( ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_ignored_spells,
            [&]( const IgnoredSpell& inst ) -> bool{ return *g_time - inst.server_cast_time > 8.f; }
        );

        if ( to_remove.empty( ) ) return;

        m_ignored_spells.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto SpellDetector::remove_old_ignored_objects( ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_ignored_objects,
            [&]( const IgnoredObject& inst ) -> bool{ return *g_time - inst.start_time > 15.f; }
        );

        if ( to_remove.empty( ) ) return;

        m_ignored_objects.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto SpellDetector::remove_object( const unsigned nid ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_active_objects,
            [&]( const ObjectInstance& object ) -> bool{ return object.network_id == nid; }
        );

        if ( to_remove.empty( ) ) return;

        m_active_objects.erase( to_remove.begin( ), to_remove.end( ) );
    }


    auto SpellDetector::remove_spell( const int32_t source_index, const Vec3& raw_end_pos ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_active_spells,
            [&]( const SpellInstance& spell ) -> bool{
                if ( ( spell.source_index == source_index || spell.particle_index == source_index ) && spell.raw_end_pos
                    == raw_end_pos ) {
                    m_ignored_spells.push_back(
                        IgnoredSpell(
                            spell.is_particle_spell ? spell.particle_index : spell.source_index,
                            spell.server_cast_time,
                            spell.raw_end_pos
                        )
                    );

                    return true;
                }

                return false;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_active_spells.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto SpellDetector::is_spell_ignored(
        const int16_t index,
        const float   server_cast_time,
        const Vec3&   raw_end_pos
    ) const -> bool{
        for ( const auto& inst : m_ignored_spells ) {
            if ( inst.source_index != index || inst.server_cast_time != server_cast_time || inst.end_position !=
                raw_end_pos )
                continue;

            return true;
        }

        return false;
    }

    auto SpellDetector::is_object_ignored( const unsigned network_id ) const -> bool{
        for ( const auto& inst : m_ignored_objects ) { if ( inst.network_id == network_id ) return true; }

        return false;
    }

    auto SpellDetector::is_ignored_particle( const unsigned network_id ) const -> bool{
        for ( auto i = 0; i < m_ignored_particles.size( ); i++ )
            if ( m_ignored_particles[ i ] == network_id )
                return
                    true;

        return false;
    }




    auto SpellDetector::get_data( const hash_t spell_name ) -> SpellInstance{ return m_spell_instance[ spell_name ]; }

    auto SpellDetector::get_object_data( const hash_t object_name ) -> ObjectInstance{
        return m_object_instance[ object_name ];
    }


    auto SpellDetector::initialize_object_instances( ) -> void{
        m_object_instance[ rt_hash( _("CaitlynTrap") ) ]   = ObjectInstance( 15.f, 3, ESpellType::circle, true );
        m_object_instance[ rt_hash( _("JhinTrap") ) ]      = ObjectInstance( 180.f, 2, ESpellType::circle, false );
        m_object_instance[ rt_hash( _("JinxMine") ) ]      = ObjectInstance( 115.f, 3, ESpellType::circle, true );
        m_object_instance[ rt_hash( _("TeemoMushroom") ) ] = ObjectInstance( 160.f, 2, ESpellType::circle, false );
    }


    auto SpellDetector::initialize_spell_instances( ) -> void{
        m_spell_instance[ rt_hash( _("AatroxQ") ) ] = SpellInstance(
            0.f,
            650.f,
            130.f,
            0.6f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("AatroxQ2") ) ] = SpellInstance(
            0.f,
            500.f,
            200.f,
            0.6f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("AatroxQ3") ) ] = SpellInstance(
            0.f,
            200.f,
            300.f,
            0.6f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("AatroxW") ) ] = SpellInstance(
            1800.f,
            825.f,
            80.f,
            0.25f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("AhriQ") ) ] = SpellInstance(
            2500.f,
            900.f,
            100.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("AhriQMissile") ) ] = SpellInstance(
            2500.f,
            900.f,
            100.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("AhriE") ) ] = SpellInstance(
            1500.f,
            950.f,
            60.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("AhriEMissile") ) ] = SpellInstance(
            1500.f,
            950.f,
            60.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("AkaliE") ) ] = SpellInstance(
            1800.f,
            825.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("AkshanQ") ) ] = SpellInstance(
            1500.f,
            850.f,
            60.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("Pulverize") ) ] = SpellInstance(
            0.f,
            0.f,
            320.f,
            0.25f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("BandageToss") ) ] = SpellInstance(
            2000.f,
            1020.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("CurseoftheSadMummy") ) ] = SpellInstance(
            0.f,
            0.f,
            550.f,
            0.25f,
            5,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("DrMundoQ") ) ] = SpellInstance(
            2000.f,
            1050.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("DariusAxeGrabCone") ) ] = SpellInstance(
            0.f,
            535.f,
            60.f,
            0.25f,
            4,
            ESpellType::cone,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("FlashFrostSpell") ) ] = SpellInstance(
            850.f,
            1100.f,
            110.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("AnnieW") ) ] = SpellInstance(
            0.f,
            625.f,
            50.f,
            0.25f,
            2,
            ESpellType::cone,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("AnnieR") ) ] = SpellInstance(
            0.f,
            600.f,
            290.f,
            0.25f,
            5,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ApheliosCalibrumQ") ) ] = SpellInstance(
            1850.f,
            1450.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("ApheliosInfernumQ") ) ] = SpellInstance(
            1500.f,
            850.f,
            50.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("ApheliosR") ) ] = SpellInstance(
            2050.f,
            1600.f,
            125.f,
            0.5f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("Volley") ) ] = SpellInstance(
            2000.f,
            1200.f,
            20.f,
            0.25f,
            1,
            ESpellType::none,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("EnchantedCrystalArrow") ) ] = SpellInstance(
            1600.f,
            25000.f,
            130.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        //m_spell_instance[ rt_hash( _("AurelionSolQ") ) ] = spell_instance_t( 850.f, 1075.f, 110.f, 0.f, 3, e_spell_type::line, true, false );
        //m_spell_instance[ rt_hash( _("AurelionSolE") ) ] = spell_instance_t( 4500.f, 1500.f, 120.f, 0.35f, 4, e_spell_type::line, true, false );
        m_spell_instance[ rt_hash( _("AzirR") ) ] = SpellInstance(
            1400.f,
            500.f,
            250.f,
            0.3f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("BardQ") ) ] = SpellInstance(
            1500.f,
            950.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("BardR") ) ] = SpellInstance(
            2100.f,
            3400.f,
            350.f,
            0.5f,
            4,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("BelvethW") ) ] = SpellInstance(
            0.f,
            650.f,
            110.f,
            0.5f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("RocketGrab") ) ] = SpellInstance(
            1800.f,
            1150.f,
            75.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("RocketGrabMissile") ) ] = SpellInstance(
            1800.f,
            1150.f,
            75.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("StaticField") ) ] = SpellInstance(
            0.f,
            0.f,
            600.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("BrandQ") ) ] = SpellInstance(
            1600.f,
            1050.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("BrandW") ) ] = SpellInstance(
            0.f,
            900.f,
            260.f,
            0.85f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("BraumQ") ) ] = SpellInstance(
            1700.f,
            1000.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("BraumR") ) ] = SpellInstance(
            1400.f,
            1250.f,
            115.f,
            0.5f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("CaitlynQ") ) ] = SpellInstance(
            2200.f,
            1250.f,
            90.f,
            0.625f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("CaitlynE") ) ] = SpellInstance(
            1600.f,
            740.f,
            70.f,
            0.15f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("CaitlynEMissile") ) ] = SpellInstance(
            1600.f,
            740.f,
            70.f,
            0.15f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("CaitlynEntrapment") ) ] = SpellInstance(
            1600.f,
            750.f,
            70.f,
            0.15f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("CamilleE") ) ] = SpellInstance(
            1900.f,
            800.f,
            60.f,
            0.f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("CamilleEDash2") ) ] = SpellInstance(
            1900.f,
            400.f,
            60.f,
            0.f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("CassiopeiaQ") ) ] = SpellInstance(
            0.f,
            850.f,
            150.f,
            0.4f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("CassiopeiaR") ) ] = SpellInstance(
            0.f,
            850.f,
            155.f,
            0.5f,
            5,
            ESpellType::cone,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("Rupture") ) ] = SpellInstance(
            0.f,
            950.f,
            240.f,
            1.2f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("FeralScream") ) ] = SpellInstance(
            0.f,
            650.f,
            150.f,
            0.5f,
            1,
            ESpellType::cone,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("PhosphorusBomb") ) ] = SpellInstance(
            1000.f,
            825.f,
            250.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("MissileBarrageMissile") ) ] = SpellInstance(
            2000.f,
            1300.f,
            40.f,
            0.175f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("MissileBarrageMissile2") ) ] = SpellInstance(
            2000.f,
            1500.f,
            40.f,
            0.175f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("DianaQ") ) ] = SpellInstance(
            1900.f,
            900.f,
            185.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("DianaR") ) ] = SpellInstance(
            0.f,
            0.f,
            450.f,
            0.25f,
            5,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("DravenDoubleShot") ) ] = SpellInstance(
            1600.f,
            1050.f,
            130.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("DravenRCast") ) ] = SpellInstance(
            2000.f,
            25000.f,
            160.f,
            0.25f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("DravenDoubleShotMissile") ) ] = SpellInstance(
            2000.f,
            25000.f,
            160.f,
            0.25f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("InfectedCleaverMissile") ) ] = SpellInstance(
            2000.f,
            975.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("EkkoQ") ) ] = SpellInstance(
            1650.f,
            700.f,
            60.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("EkkoW") ) ] = SpellInstance(
            0.f,
            1600.f,
            375.f,
            0.5f,
            0,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("EliseHumanE") ) ] = SpellInstance(
            1600.f,
            1075.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("EvelynnQ") ) ] = SpellInstance(
            2400.f,
            800.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("EvelynnR") ) ] = SpellInstance(
            0.f,
            450.f,
            180.f,
            0.35f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("EzrealQ") ) ] = SpellInstance(
            2000.f,
            1150.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("EzrealW") ) ] = SpellInstance(
            1700.f,
            1150.f,
            80.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("EzrealR") ) ] = SpellInstance(
            2000.f,
            25000.f,
            160.f,
            1.f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("FioraW") ) ] = SpellInstance(
            3200.f,
            750.f,
            70.f,
            0.75f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("FizzR") ) ] = SpellInstance(
            1300.f,
            1300.f,
            150.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("GalioQ") ) ] = SpellInstance(
            1150.f,
            825.f,
            235.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("GalioE") ) ] = SpellInstance(
            2300.f,
            650.f,
            160.f,
            0.4f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("GnarQMissile") ) ] = SpellInstance(
            2500.f,
            1125.f,
            55.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("GnarBigQMissile") ) ] = SpellInstance(
            2100.f,
            1125.f,
            90.f,
            0.5f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("GnarBigW") ) ] = SpellInstance(
            0.f,
            575.f,
            100.f,
            0.6f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("GnarR") ) ] = SpellInstance(
            0.f,
            0.f,
            475.f,
            0.25f,
            4,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("GragasQ") ) ] = SpellInstance(
            1000.f,
            850.f,
            275.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("GragasE") ) ] = SpellInstance(
            900.f,
            600.f,
            170.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("GragasR") ) ] = SpellInstance(
            1800.f,
            1000.f,
            400.f,
            0.25f,
            5,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("GravesQLineSpell") ) ] = SpellInstance(
            0.f,
            800.f,
            80.f,
            1.4f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("GravesSmokeGrenade") ) ] = SpellInstance(
            1500.f,
            950.f,
            250.f,
            0.15f,
            0,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("GwenQ") ) ] = SpellInstance(
            0.f,
            400.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _( "GwenRMis" ) ) ] = SpellInstance(
            1800.f,
            1300.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        //GwenRRecast
        m_spell_instance[ rt_hash( _("HecarimUlt") ) ] = SpellInstance(
            1100.f,
            1650.f,
            280.f,
            0.2f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("HeimerdingerW") ) ] = SpellInstance(
            2050.f,
            1325.f,
            100.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("HeimerdingerE") ) ] = SpellInstance(
            1200.f,
            970.f,
            250.f,
            0.25f,
            2,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("HeimerdingerEUlt") ) ] = SpellInstance(
            1200.f,
            970.f,
            250.f,
            0.25f,
            2,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[rt_hash(_("HweiQQ"))] =
            SpellInstance(2000.f, 800.f, 70.f, 0.25f, 2, ESpellType::line, false, true);

         m_spell_instance[rt_hash(_("HweiQW"))] =
            SpellInstance(0.f, 1900.f, 225.f, 0.5f, 2, ESpellType::circle, false, false);

         m_spell_instance[rt_hash(_("HweiQE"))] =
             SpellInstance(800.f, 1300.f, 80.f, 0.5f, 2, ESpellType::line, false, false);

         m_spell_instance[rt_hash(_("HweiEQ"))] =
             SpellInstance(1300.f, 1100.f, 60.f, 0.25f, 3, ESpellType::line, true, false);

        m_spell_instance[rt_hash(_("HweiEW"))] =
             SpellInstance(1600.f, 900.f, 350.f, 0.25f, 2, ESpellType::circle, true, false);

        m_spell_instance[rt_hash(_("HweiR"))] =
             SpellInstance(1400.f, 1340.f, 80.f, 0.5f, 4, ESpellType::line, false, false);

         //m_spell_instance[rt_hash(_("HweiQE"))] =
         //    SpellInstance(0.f, 1900.f, 200.f, 0.5f, 2, ESpellType::circle, false, false);

        m_spell_instance[ rt_hash( _("IllaoiQ") ) ] = SpellInstance(
            0.f,
            750.f,
            100.f,
            0.75f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("IllaoiE") ) ] = SpellInstance(
            1900.f,
            900.f,
            50.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("IreliaEMissile") ) ] = SpellInstance(
            2000.f,
            0.f,
            0.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("IreliaR") ) ] = SpellInstance(
            2000.f,
            950.f,
            160.f,
            0.4f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("IvernQ") ) ] = SpellInstance(
            1300.f,
            1075.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("HowlingGaleSpell") ) ] = SpellInstance(
            1408.f,
            1750.f,
            100.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("JarvanIVDragonStrike") ) ] = SpellInstance(
            0.f,
            735.f,
            68.f,
            0.4f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JayceShockBlast") ) ] = SpellInstance(
            1450.f,
            1050.f,
            70.f,
            0.214f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JayceShockBlastWallMis") ) ] = SpellInstance(
            2350.f,
            1600.f,
            70.f,
            0.152f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JhinW") ) ] = SpellInstance(
            0.f,
            2550.f,
            40.f,
            0.75f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JhinRShotMis") ) ] = SpellInstance(
            5000.f,
            3500.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JhinRShotMis4") ) ] = SpellInstance(
            5000.f,
            3500.f,
            80.f,
            0.25f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JinxWMissile") ) ] = SpellInstance(
            3300.f,
            1450.f,
            60.f,
            0.6f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("JinxR") ) ] = SpellInstance(
            1700.f,
            25000.f,
            140.f,
            0.6f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KaisaW") ) ] = SpellInstance(
            1750.f,
            3000.f,
            100.f,
            0.4f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KarmaQ") ) ] = SpellInstance(
            1700.f,
            890.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KarmaQMissile") ) ] = SpellInstance(
            1700.f,
            890.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KarmaQMantra") ) ] = SpellInstance(
            1700.f,
            950.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KarmaQMissileMantra") ) ] = SpellInstance(
            1700.f,
            950.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KarthusLayWasteA1") ) ] = SpellInstance(
            0.f,
            875.f,
            200.f,
            0.8f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KarthusLayWasteA2") ) ] = SpellInstance(
            0.f,
            875.f,
            200.f,
            0.8f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KarthusLayWasteA3") ) ] = SpellInstance(
            0.f,
            875.f,
            200.f,
            0.8f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("RiftWalk") ) ] = SpellInstance(
            0.f,
            500.f,
            250.f,
            0.9f,
            3,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KayleQ") ) ] = SpellInstance(
            1600.f,
            900.f,
            60.f,
            0.9f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KaynQ") ) ] = SpellInstance(
            0.f,
            350.f,
            350.f,
            0.15f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KaynW") ) ] = SpellInstance(
            0.f,
            700.f,
            90.f,
            0.6f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("KennenShurikenHurlMissile1") ) ] = SpellInstance(
            1700.f,
            1050.f,
            50.f,
            0.175f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KhazixW") ) ] = SpellInstance(
            1700.f,
            1000.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KledQ") ) ] = SpellInstance(
            1600.f,
            800.f,
            60.f,
            0.25f,
            1,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("KledRiderQ") ) ] = SpellInstance(
            3000.f,
            700.f,
            100.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("KledEDash") ) ] = SpellInstance(
            1100.f,
            550.f,
            90.f,
            0.f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("KogMawQ") ) ] = SpellInstance(
            1650.f,
            1175.f,
            70.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("KogMawVoidOozeMissile") ) ] = SpellInstance(
            1400.f,
            1360.f,
            120.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KogMawLivingArtillery") ) ] = SpellInstance(
            0.f,
            1300.f,
            240.f,
            1.f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KSanteQ") ) ] = SpellInstance(
            0.f,
            465.f,
            75.f,
            0.f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("KSanteQ3") ) ] = SpellInstance(
            1800.f,
            755.f,
            70.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("KSanteQ3Missile") ) ] = SpellInstance(
            1800.f,
            755.f,
            70.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LilliaW") ) ] = SpellInstance(
            0.f,
            500.f,
            200.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LilliaE") ) ] = SpellInstance(
            1400.f,
            700.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("LilliaERollingMissile") ) ] = SpellInstance(
            1400.f,
            99999.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("LeblancE") ) ] = SpellInstance(
            1750.f,
            925.f,
            55.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("LeblancEMissile") ) ] = SpellInstance(
            1750.f,
            925.f,
            55.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("LeblancRE") ) ] = SpellInstance(
            1750.f,
            925.f,
            55.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("LeblancREMissile") ) ] = SpellInstance(
            1750.f,
            925.f,
            55.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("BlindMonkQOne") ) ] = SpellInstance(
            1800.f,
            1100.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("LeonaZenithBlade") ) ] = SpellInstance(
            2000.f,
            875.f,
            70.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LeonaZenithBladeMissile") ) ] = SpellInstance(
            2000.f,
            875.f,
            70.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LeonaSolarFlare") ) ] = SpellInstance(
            0.f,
            1200.f,
            300.f,
            0.85f,
            2,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LissandraQMissile") ) ] = SpellInstance(
            2200.f,
            750.f,
            75.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LissandraEMissile") ) ] = SpellInstance(
            850.f,
            1025.f,
            125.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LucianQ") ) ] = SpellInstance(
            0.f,
            1000.f,
            65.f,
            0.35f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LucianW") ) ] = SpellInstance(
            1600.f,
            900.f,
            80.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("LuluQ") ) ] = SpellInstance(
            1450.f,
            925.f,
            60.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LuxLightBinding") ) ] = SpellInstance(
            1200.f,
            1175.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LuxLightBindingDummy") ) ] = SpellInstance(
            1200.f,
            1175.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LuxLightBindingMis") ) ] = SpellInstance(
            1200.f,
            1175.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("LuxLightStrikeKugel") ) ] = SpellInstance(
            1200.f,
            1100.f,
            300.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("LuxRMis") ) ] = SpellInstance(
            0.f,
            3340.f,
            125.f,
            1.f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("UFSlash") ) ] = SpellInstance(
            1835.f,
            1000.f,
            300.f,
            0.f,
            5,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("MalzaharQ") ) ] = SpellInstance(
            1600.f,
            900.f,
            85.f,
            0.5f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _( "MilioQ" ) ) ] = SpellInstance(
            1200.f,
            1000.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _( "MilioQHit" ) ) ] = SpellInstance(
            0.f,
            600.f,
            250.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _( "MilioQHitMinion" ) ) ] = SpellInstance(
            0.f,
            600.f,
            250.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("MissFortuneBulletTime") ) ] = SpellInstance(
            2000.f,
            1400.f,
            300.f,
            0.25f,
            0,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("MorganaQ") ) ] = SpellInstance(
            1200.f,
            1250.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("MordekaiserQ") ) ] = SpellInstance(
            0.f,
            490.f,
            165.f,
            0.5f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("MordekaiserE") ) ] = SpellInstance(
            0.f,
            850.f,
            125.f,
            0.75f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("NamiQ") ) ] = SpellInstance(
            0.f,
            850.f,
            200.f,
            0.25f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("NamiRMissile") ) ] = SpellInstance(
            850.f,
            2750.f,
            250.f,
            0.5f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("NautilusAnchorDragMissile") ) ] = SpellInstance(
            2000.f,
            1040.f,
            90.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("NeekoE") ) ] = SpellInstance(
            1300.f,
            1000.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("NeekoQ") ) ] = SpellInstance(
            1500.f,
            800.f,
            200.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("JavelinToss") ) ] = SpellInstance(
            1300.f,
            1500.f,
            40.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("NocturneDuskbringer") ) ] = SpellInstance(
            1600.f,
            1200.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("NunuR") ) ] = SpellInstance(
            0.f,
            0.f,
            650.f,
            3.f,
            0,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("OlafAxeThrowCast") ) ] = SpellInstance(
            1600.f,
            1000.f,
            90.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("OlafAxeThrow") ) ] = SpellInstance(
            1600.f,
            1000.f,
            90.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("OrnnRCharge") ) ] = SpellInstance(
            1650.f,
            2500.f,
            200.f,
            0.5f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("OrianaIzuna") ) ] = SpellInstance(
            1400.f,
            825.f,
            80.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("OrianaDetonateCommand" ) ) ] = SpellInstance(
            0.f,
            0.f,
            360.f,
            0.f,
            5,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("PantheonQTap") ) ] = SpellInstance(
            0.f,
            575.f,
            80.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("PantheonQMissile") ) ] = SpellInstance(
            2700.f,
            1200.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("PantheonR") ) ] = SpellInstance(
            2250.f,
            1350.f,
            250.f,
            4.f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("PoppyQSpell") ) ] = SpellInstance(
            0.f,
            325.f,
            100.f,
            0.332f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("PoppyRSpell") ) ] = SpellInstance(
            2500.f,
            1200.f,
            100.f,
            0.33f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("PoppyRSpellInstant") ) ] = SpellInstance(
            0.f,
            450.f,
            100.f,
            0.33f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("PykeQMelee") ) ] = SpellInstance(
            0.f,
            400.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("PykeQRange") ) ] = SpellInstance(
            2000.f,
            1100.f,
            70.f,
            0.2f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("PykeE") ) ] = SpellInstance(
            3000.f,
            550.f,
            110.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("PykeR") ) ] = SpellInstance(
            0.f,
            750.f,
            100.f,
            0.5f,
            5,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("QuinnQ") ) ] = SpellInstance(
            1550.f,
            1025.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("QiyanaQ") ) ] = SpellInstance(
            0.f,
            425.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("QiyanaQ_Rock") ) ] = SpellInstance(
            1600.f,
            850.f,
            125.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("QiyanaQ_Grass") ) ] = SpellInstance(
            1600.f,
            850.f,
            125.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("QiyanaQ_Water") ) ] = SpellInstance(
            1600.f,
            850.f,
            125.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("QiyanaR") ) ] = SpellInstance(
            2000.f,
            850.f,
            140.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("RakanQ") ) ] = SpellInstance(
            1850.f,
            850.f,
            65.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RakanW") ) ] = SpellInstance(
            0.f,
            600.f,
            250.f,
            0.7f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _( "RakanWCast" ) ) ] = SpellInstance(
            0.f,
            0.f,
            250.f,
            0.35f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("RekSaiQBurrowed") ) ] = SpellInstance(
            1950.f,
            1625.f,
            65.f,
            0.125f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("RengarE") ) ] = SpellInstance(
            1500.f,
            1000.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RengarEMis") ) ] = SpellInstance(
            1500.f,
            1000.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RengarEEmp") ) ] = SpellInstance(
            1500.f,
            1000.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RenataQ") ) ] = SpellInstance(
            1450.f,
            850.f,
            75.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("RenataE") ) ] = SpellInstance(
            1450.f,
            875.f,
            110.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RellR") ) ] = SpellInstance(
            0.f,
            0.f,
            400.f,
            0.25f,
            4,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("RenataR") ) ] = SpellInstance(
            650.f,
            1900.f,
            325.f,
            0.75f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("RivenIzunaBlade") ) ] = SpellInstance(
            1600.f,
            900.f,
            300.f,
            0.25f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("RumbleGrenade") ) ] = SpellInstance(
            2000.f,
            850.f,
            60.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("RyzeQWrapper") ) ] = SpellInstance(
            1700.f,
            1000.f,
            55.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("SamiraQGun") ) ] = SpellInstance(
            2600.f,
            890.f,
            60.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("SejuaniR") ) ] = SpellInstance(
            1600.f,
            1300.f,
            120.f,
            0.25f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SennaQCast") ) ] = SpellInstance(
            0.f,
            1400.f,
            80.f,
            0.4f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SennaW") ) ] = SpellInstance(
            1150.f,
            1300.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("SennaR") ) ] = SpellInstance(
            20000.f,
            25000.f,
            180.f,
            1.f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SeraphineQCast") ) ] = SpellInstance(
            1200.f,
            900.f,
            350.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SeraphineECast") ) ] = SpellInstance(
            1200.f,
            1300.f,
            70,
            0.25f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SeraphineEMissile") ) ] = SpellInstance(
            1200.f,
            1300.f,
            70,
            0.25f,
            2,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SeraphineR") ) ] = SpellInstance(
            1600.f,
            1300.f,
            175.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("ShenE") ) ] = SpellInstance(
            1200.f,
            600.f,
            60.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );

         m_spell_instance[rt_hash(_("ShyvanaFireball"))] =
            SpellInstance(1600.f, 925.f, 60.f, 0.25f, 2, ESpellType::line, false, false);

        m_spell_instance[ rt_hash( _("ShyvanaFireballDragon2") ) ] = SpellInstance(
            1575.f,
            975.f,
            100.f,
            0.333f,
            3,
            ESpellType::line,
            false,
            false
        );

        //ShyvanaFireball
        m_spell_instance[ rt_hash( _("SionQ") ) ] = SpellInstance(
            0.f,
            750.f,
            150.f,
            2.f,
            0,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SionE") ) ] = SpellInstance(
            1800.f,
            800.f,
            80.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SionEMissile") ) ] = SpellInstance(
            1800.f,
            800.f,
            80.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SivirQ") ) ] = SpellInstance(
            1350.f,
            1250.f,
            90.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SivirQMissile") ) ] = SpellInstance(
            1350.f,
            1250.f,
            90.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SonaR") ) ] = SpellInstance(
            2400.f,
            860.f,
            140.f,
            0.25f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SorakaQ") ) ] = SpellInstance(
            1150.f,
            810.f,
            235.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SwainW") ) ] = SpellInstance(
            0.f,
            3500.f,
            275.f,
            1.5f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SwainE") ) ] = SpellInstance(
            1800.f,
            875.f,
            90.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SylasQ") ) ] = SpellInstance(
            0.f,
            775.f,
            80.f,
            0.4f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SylasE2") ) ] = SpellInstance(
            1600.f,
            850.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("SyndraQSpell") ) ] = SpellInstance(
            0.f,
            800.f,
            210.f,
            0.625f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SyndraE") ) ] = SpellInstance(
            0.f,
            700.f,
            200.f,
            0.25f,
            2,
            ESpellType::cone,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SyndraE5") ) ] = SpellInstance(
            0.f,
            700.f,
            200.f,
            0.25f,
            2,
            ESpellType::cone,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("SyndraESphereMissile") ) ] = SpellInstance(
            2000.f,
            1250.f,
            100.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("TahmKenchQ") ) ] = SpellInstance(
            2800.f,
            900.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("TahmKenchW") ) ] = SpellInstance(
            0.f,
            1200.f,
            250.f,
            0.25f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("TaliyahQ") ) ] = SpellInstance(
            3600.f,
            1000.f,
            100.f,
            0.f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("TaliyahWVC") ) ] = SpellInstance(
            0.f,
            900.f,
            225.f,
            0.25f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("TalonW") ) ] = SpellInstance(
            2500.f,
            650.f,
            175.f,
            0.25f,
            2,
            ESpellType::none,
            false,
            false
        );
        //m_spell_instance[ rt_hash( _("ThreshQInternal") ) ] = spell_instance_t( 1900.f, 1100.f, 70.f, 0.5f, 3, e_spell_type::line, true, true );
        m_spell_instance[ rt_hash( _("ThreshQMissile") ) ] = SpellInstance(
            1900.f,
            1100.f,
            70.f,
            0.5f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("ThreshE") ) ] = SpellInstance(
            0.f,
            550.f,
            110.f,
            0.389f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("TwitchVenomCask") ) ] = SpellInstance(
            1400.f,
            950.f,
            275.f,
            0.25f,
            0,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("TryndamereE") ) ] = SpellInstance(
            1300.f,
            660.f,
            225.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("WildCards") ) ] = SpellInstance(
            1000.f,
            1450.f,
            40.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("SealFateMissile") ) ] = SpellInstance(
            1000.f,
            1450.f,
            40.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("UrgotQ") ) ] = SpellInstance(
            0.f,
            800.f,
            175.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("UrgotE") ) ] = SpellInstance(
            1500.f,
            450.f,
            100.f,
            0.5f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("UrgotR") ) ] = SpellInstance(
            3200.f,
            2500.f,
            80.f,
            0.5f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VarusQMissile") ) ] = SpellInstance(
            1900.f,
            1525.f,
            70.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VarusE") ) ] = SpellInstance(
            0.f,
            925.f,
            250.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VarusR") ) ] = SpellInstance(
            1950.f,
            1200.f,
            120.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("VarusRMissile") ) ] = SpellInstance(
            1950.f,
            1200.f,
            120.f,
            0.25f,
            4,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("VeigarBalefulStrike") ) ] = SpellInstance(
            2200.f,
            900.f,
            70.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("VeigarDarkMatterCastLockout") ) ] = SpellInstance(
            0.f,
            900.f,
            240.f,
            1.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VeigarEventHorizon") ) ] = SpellInstance(
            0.f,
            725.f,
            400.f,
            0.25f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("VelkozQ") ) ] = SpellInstance(
            1300.f,
            1050.f,
            50.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _( "VelkozQMissile" ) ) ] = SpellInstance(
            1300.f,
            1050.f,
            50.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _( "VelkozQMissileSplit" ) ) ] = SpellInstance(
            2100.f,
            1055.f,
            45.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("VelkozW") ) ] = SpellInstance(
            1700.f,
            1050.f,
            87.5,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _( "VelkozWMissile" ) ) ] = SpellInstance(
            1700.f,
            1050.f,
            87.5,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VelkozE") ) ] = SpellInstance(
            0.f,
            800.f,
            225.f,
            0.8f,
            3,
            ESpellType::circle,
            true,
            true
        );
        m_spell_instance[ rt_hash( _( "VelkozEMissile" ) ) ] = SpellInstance(
            0.f,
            800.f,
            225.f,
            0.8f,
            3,
            ESpellType::circle,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("VexQ") ) ] = SpellInstance(
            3200.f,
            1100.f,
            80.f,
            0.15f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VexE") ) ] = SpellInstance(
            1300.f,
            800.f,
            200.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("VexR") ) ] = SpellInstance(
            1600.f,
            2000.f,
            130.f,
            0.25f,
            4,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViegoQ") ) ] = SpellInstance(
            0.f,
            565.f,
            62.5f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViegoWMis") ) ] = SpellInstance(
            1300.f,
            830.f,
            60.f,
            0.f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("ViegoR") ) ] = SpellInstance(
            0.f,
            500.f,
            300.f,
            0.f,
            3,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViktorW") ) ] = SpellInstance(
            0.f,
            800.f,
            270.f,
            1.75f,
            3,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("ViktorDeathRayMissile") ) ] = SpellInstance(
            1050.f,
            590.f,
            90.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViktorDeathRayMissile2") ) ] = SpellInstance(
            1500.f,
            590.f,
            90.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViktorEAugMissile") ) ] = SpellInstance(
            1050.f,
            590.f,
            90.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ViktorEAugParticle") ) ] = SpellInstance(
            0.f,
            500.f,
            90.f,
            0.f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("WarwickR") ) ] = SpellInstance(
            1800.f,
            3000.f,
            100.f,
            0.1f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("XayahQ") ) ] = SpellInstance(
            2075.f,
            1100.f,
            45.f,
            0.5f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("XerathArcanopulse2") ) ] = SpellInstance(
            0.f,
            1400.f,
            70.f,
            0.5f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("XerathArcaneBarrage2") ) ] = SpellInstance(
            0.f,
            1000.f,
            280.f,
            0.75f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("XerathMageSpear") ) ] = SpellInstance(
            1400.f,
            1060.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("XerathMageSpearMissile") ) ] = SpellInstance(
            1400.f,
            1100.f,
            60.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("XerathLocusPulse") ) ] = SpellInstance(
            0.f,
            5000.f,
            200.f,
            0.75f,
            2,
            ESpellType::circle,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("XinZhaoW") ) ] = SpellInstance(
            5000.f,
            900.f,
            40.f,
            0.5f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("YasuoQ1") ) ] = SpellInstance(
            0.f,
            450.f,
            40.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("YasuoQ2") ) ] = SpellInstance(
            0.f,
            450.f,
            40.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("YasuoQ3") ) ] = SpellInstance(
            1200.f,
            1000.f,
            90.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("YasuoQ3Mis") ) ] = SpellInstance(
            1200.f,
            1000.f,
            90.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("YoneQ") ) ] = SpellInstance(
            0.f,
            450.f,
            40.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("YoneQ3") ) ] = SpellInstance(
            1500.f,
            970.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("YoneQ3Missile") ) ] = SpellInstance(
            1500.f,
            1050.f,
            80.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("YoneR") ) ] = SpellInstance(
            0.f,
            900.f,
            125.f,
            0.75f,
            5,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _( "YorickE" ) ) ] = SpellInstance(
            1800.f,
            700.f,
            100.f,
            0.33f,
            3,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZacQ") ) ] = SpellInstance(
            2800.f,
            870.f,
            80.f,
            0.33f,
            4,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("ZedQ") ) ] = SpellInstance(
            1700.f,
            900.f,
            50.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZedQMissile") ) ] = SpellInstance(
            1700.f,
            900.f,
            50.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZeriQ") ) ] = SpellInstance(
            2600.f,
            825.f,
            40.f,
            0.f,
            0,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("ZeriW") ) ] = SpellInstance(
            2200.f,
            1200.f,
            40.f,
            0.f,
            2,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("ZiggsQ") ) ] = SpellInstance(
            1750.f,
            850.f,
            180.f,
            0.25f,
            1,
            ESpellType::circle,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("ZiggsW") ) ] = SpellInstance(
            1750.f,
            1000.f,
            260.f,
            0.25f,
            2,
            ESpellType::circle,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("ZiggsE") ) ] = SpellInstance(
            1800.f,
            900.f,
            260.f,
            0.25f,
            2,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZiggsR") ) ] = SpellInstance(
            1550.f,
            5000.f,
            480.f,
            0.375f,
            0,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZileanQ") ) ] = SpellInstance(
            0.f,
            900.f,
            140.f,
            0.8f,
            3,
            ESpellType::circle,
            false,
            false
        );
        m_spell_instance[ rt_hash( _("ZoeQMissile") ) ] = SpellInstance(
            1200.f,
            800.f,
            50.f,
            0.25f,
            1,
            ESpellType::line,
            false,
            true
        );
        //m_spell_instance[ rt_hash( _("ZoeQMis2") ) ] = spell_instance_t( 2500.f, 1600.f, 70.f, 0.f, 2, e_spell_type::line, false, true );
        m_spell_instance[ rt_hash( _( "ZoeQMis2Warning" ) ) ] = SpellInstance(
            2500.f,
            800.f,
            70.f,
            0.25f,
            2,
            ESpellType::line,
            false,
            true
        );
        m_spell_instance[ rt_hash( _("ZoeE") ) ] = SpellInstance(
            1700.f,
            800.f,
            50.f,
            0.3f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _( "ZoeEMisAudio" ) ) ] = SpellInstance(
            1700.f,
            800.f,
            50.f,
            0.3f,
            3,
            ESpellType::line,
            true,
            true
        );
        m_spell_instance[ rt_hash( _("ZyraE") ) ] = SpellInstance(
            1150.f,
            1100.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _("ZyraR") ) ] = SpellInstance(
            0.f,
            700.f,
            500.f,
            0.25f,
            4,
            ESpellType::circle,
            true,
            false
        );

        m_spell_instance[rt_hash(_("NaafiriQ"))] =
            SpellInstance(2000.f, 900.f, 60.f, 0.25f, 2, ESpellType::line, false, false);

         m_spell_instance[rt_hash(_("NaafiriQRecast"))] =
            SpellInstance(2000.f, 900.f, 60.f, 0.25f, 2, ESpellType::line, false, false);

        m_spell_instance[ rt_hash( _( "BriarR" ) ) ] = SpellInstance(
            2000.f,
            10000.f,
            160.f,
            1.f,
            4,
            ESpellType::line,
            false,
            false
        );

        // everfrost
        m_spell_instance[ rt_hash( _("6656Cast") ) ] = SpellInstance(
            2000.f,
            800.f,
            70.f,
            0.25f,
            3,
            ESpellType::line,
            true,
            false
        );
        m_spell_instance[ rt_hash( _( "PoroSnowballMissile" ) ) ] = SpellInstance(
            1200.f,
            1600.f,
            80.f,
            0.f,
            3,
            ESpellType::line,
            false,
            true
        );
    }

    //6656Cast

    auto SpellDetector::get_manual_endtime( const std::string& spell_name ) -> std::optional< float >{
        const auto hash = rt_hash( spell_name.data( ) );
        //HweiQW
        switch ( hash ) {
        case ct_hash("HweiQW"):
            return std::make_optional(0.5f);
        case ct_hash("HweiEW"):
            return std::make_optional(0.75f);
        case ct_hash( "XerathArcaneBarrage2" ):
            return std::make_optional( 0.5f );
        case ct_hash( "NamiQ" ):
            return std::make_optional( 0.75f );
        case ct_hash( "ZileanQ" ):
            return std::make_optional( 0.5f );
        case ct_hash( "UrgotQ" ):
            return std::make_optional( 0.45f );
        case ct_hash( "GragasR" ):
            return std::make_optional( 0.575f );
        case ct_hash( "VarusE" ):
            return std::make_optional( 0.5f );
        case ct_hash( "XerathArcanopulse2" ):
            return std::make_optional( 0.0f );
        case ct_hash( "KaynW" ):
        case ct_hash( "QiyanaQ" ):
        case ct_hash( "AnnieW" ):
        case ct_hash( "FeralScream" ):
        case ct_hash( "AatroxQ" ):
        case ct_hash( "AatroxQ2" ):
        case ct_hash( "AatroxQ3" ):
        case ct_hash( "DariusAxeGrabCone" ):
        case ct_hash( "JarvanIVDragonStrike" ):
        case ct_hash( "OrianaDetonateCommand" ):
            return std::make_optional( 0.05f );
        case ct_hash( "MordekaiserE" ):
        case ct_hash( "CassiopeiaQ" ):
            return std::make_optional( 0.6f );
        case ct_hash( "VeigarDarkMatterCastLockout" ):
            return std::make_optional( 1.221f );
        case ct_hash( "KogMawLivingArtillery" ):
            return std::make_optional( 0.95f );
        case ct_hash( "TaliyahWVC" ):
            return std::make_optional( 0.65f );
        case ct_hash( "KarthusLayWasteA1" ):
        case ct_hash( "KarthusLayWasteA2" ):
        case ct_hash( "KarthusLayWasteA3" ):
            return std::make_optional( 0.575f );
        case ct_hash( "MilioQHit" ):
        case ct_hash( "MilioQHitMinion" ):
            return std::make_optional( 0.825f );
        case ct_hash( "AnnieR" ):
        case ct_hash( "IllaoiQ" ):
        case ct_hash( "RiftWalk" ):
        case ct_hash( "ZyraR" ):
        case ct_hash( "JhinW" ):
        case ct_hash( "PoppyRSpellInstant" ):
        case ct_hash( "DianaR" ):
        case ct_hash( "Pulverize" ):
            return std::make_optional( 0.f );
        case ct_hash( "XerathLocusPulse" ):
        case ct_hash( "SyndraQSpell" ):
            return std::make_optional( 0.625f );
        case ct_hash( "ViktorEAugParticle" ):
        case ct_hash( "Rupture" ):
        case ct_hash( "SylasQ" ):
            return std::make_optional( 1.f );
        case ct_hash( "EkkoW" ):
            return std::make_optional( 3.75f );
        case ct_hash( "SyndraE" ):
        case ct_hash( "SyndraE5" ):
            return std::make_optional( 0.2f );
        case ct_hash( "LilliaW" ):
            return std::make_optional( 0.5f );
        case ct_hash( "SwainW" ):
            return std::make_optional( 1.25f );
        case ct_hash( "VelkozE" ):
        case ct_hash( "VelkozEMissile" ):
            return std::make_optional( 0.75f );
        default:
            break;
        }

        return std::nullopt;
    }

    auto SpellDetector::get_manual_cast_time( const std::string& spell_name ) -> std::optional< float >{
        const auto hash = rt_hash( spell_name.data() );

        switch ( hash ) {
        case ct_hash( "6656Cast" ):
            return std::make_optional( 0.15f );
       // case ct_hash("HweiQE"):
         //   return std::make_optional(0.25f);
        default:
            break;
        }

        return std::nullopt;
    }

    auto SpellDetector::get_dynamic_danger_level( const std::string& spell_name ) -> std::optional< int >{
        const auto hash = rt_hash( spell_name.data() );

        switch ( hash ) {
        case ct_hash( "BrandQ" ):
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "BrandAblaze" ) ) )
                return
                    std::make_optional( 3 );
            break;
        case ct_hash( "JhinW" ):
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "jhinespotteddebuff" ) ) )
                return
                    std::make_optional( 3 );
            break;
        case ct_hash( "UrgotR" ):
            if ( g_local->health / g_local->max_health <= g_local->max_health * 0.35f ) return std::make_optional( 5 );
            if ( g_local->health / g_local->max_health <= g_local->max_health * 0.6f ) return std::make_optional( 4 );
            break;
        default:
            break;
        }

        return std::nullopt;
    }


    auto SpellDetector::should_fix_range( const std::string& spell_name ) -> bool{
        const auto hash = rt_hash( spell_name.data() );

        switch ( hash ) {
        case ct_hash( "XerathArcanopulse2" ):
        case ct_hash( "SyndraESphereMissile" ):
        case ct_hash( "VarusQMissile" ):
        case ct_hash( "LilliaERollingMissile" ):
        case ct_hash( "ViegoWMis" ):
        case ct_hash( "OrianaIzuna" ):
        case ct_hash( "ZoeEMisAudio" ):
        case ct_hash( "ZoeQMis2Warning" ):
            return false;
        default:
            return true;
        }
    }

    auto SpellDetector::has_edge_range( const hash_t spell_name ) -> bool{
        switch ( spell_name ) {
        case ct_hash("HweiQW"):
        //case ct_hash("HweiQE"):
        case ct_hash( "BrandW" ):
        case ct_hash( "GragasR" ):
        case ct_hash( "KogMawLivingArtillery" ):
        case ct_hash( "MilioQHit" ):
        case ct_hash( "MilioQHitMinion" ):
        case ct_hash( "NamiQ" ):
        case ct_hash( "VeigarDarkMatterCastLockout" ):
        case ct_hash( "VelkozE" ):
        case ct_hash( "XerathArcaneBarrage2" ):
        case ct_hash( "XerathLocusPulse" ):
        case ct_hash( "KarthusLayWasteA1" ):
        case ct_hash( "KarthusLayWasteA2" ):
        case ct_hash( "KarthusLayWasteA3" ):
            return false;
        default:
            return true;
        }
    }

    auto SpellDetector::get_particle_spell( std::string particle_name, Vec3 particle_position ) -> SpellInstance{
        SpellInstance instance{ };
        //E_LandPositionIndicator
        if ( particle_name.find( "Zac" ) != std::string::npos
            && particle_name.find( "E_LandPositionIndicat" ) != std::string::npos ) {
            instance.range         = 1800.f;
            instance.danger        = 4;
            instance.cc            = true;
            instance.collision     = false;
            instance.radius        = 250.f;
            instance.type          = ESpellType::circle;
            instance.particle_type = EParticleSpell::zac_e;

            if ( !m_detect_ally_particles ) {
                bool ally_found{ };
                for ( auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || rt_hash( ally->champion_name.text ) != ct_hash( "Zac" ) )
                        continue
                            ;

                    ally_found = true;
                }

                if ( ally_found ) return { };
            }

            bool    found_caster{ };
            Object* caster{ };

            const auto enemies = m_detect_ally_particles ? g_entity_list.get_allies( ) : g_entity_list.get_enemies( );
            for ( auto enemy : enemies ) {
                if ( !enemy || enemy->is_dead( ) || rt_hash( enemy->champion_name.text ) != ct_hash( "Zac" ) ) continue;


                found_caster = true;
                caster       = enemy;

                if ( found_caster ) break;
            }

            if ( !found_caster || !caster ) return { };

            auto aimgr = caster->get_ai_manager( );
            if ( !aimgr || !aimgr->is_dashing ) return { };

            auto path = aimgr->get_path( );

            instance.speed      = 1200.f;
            instance.base_speed = 1200.f;

            instance.end_time         = *g_time + caster->position.dist_to( particle_position ) / aimgr->dash_speed;
            instance.start_time       = *g_time;
            instance.server_cast_time = *g_time;

            instance.start_pos = path[ path.size( ) - 1 ];

            instance.end_pos     = particle_position;
            instance.raw_end_pos = particle_position;
            instance.spell_name  = _( "ZacParticleE" );

            return instance;
        }

        if ( particle_name.find( "Lux" ) != std::string::npos && particle_name.find( "R_cas" ) != std::string::npos ) {
            instance.range           = 3400.f;
            instance.danger          = 4;
            instance.cc              = false;
            instance.collision       = false;
            instance.radius          = 100.f;
            instance.type            = ESpellType::line;
            instance.particle_type   = EParticleSpell::lux_r;
            instance.has_edge_radius = true;

            if ( !m_detect_ally_particles ) {
                bool ally_found{ };

                for ( auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || rt_hash( ally->champion_name.text ) != ct_hash( "Lux" ) )
                        continue
                            ;

                    auto sci = ally->spell_book.get_spell_cast_info( );
                    if ( !sci || sci->slot != 3 ) continue;

                    ally_found = true;
                }

                if ( ally_found ) return { };
            }


            instance.speed      = 0.f;
            instance.base_speed = 0.f;

            instance.end_time         = *g_time + 1.f;
            instance.start_time       = *g_time;
            instance.server_cast_time = *g_time;

            instance.start_pos   = particle_position;
            instance.current_pos = particle_position;

            instance.current_pos.y = g_navgrid->get_height( particle_position );
            instance.start_pos.y   = instance.current_pos.y;

            bool     found_particle{ };
            Vec3     particle_pos{ };
            unsigned related_nid{ };

            for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                if ( !particle || is_ignored_particle( particle->network_id ) || particle->position.dist_to(
                    particle_position
                ) <= 100.f )
                    continue;

                const auto name = particle->get_alternative_name( );
                if ( name.find( "Lux" ) == std::string::npos || name.find( "mis_beam_middle" ) ==
                    std::string::npos )
                    continue;

                found_particle = true;
                particle_pos   = particle->position;
                related_nid    = particle->network_id;
                break;
            }

            if ( !found_particle ) return { };

            instance.end_pos     = particle_position.extend( particle_pos, instance.range );
            instance.raw_end_pos = particle_position;
            instance.spell_name  = "LuxRMis";
            instance.related_particles.push_back( related_nid );

            return instance;
        }

        if ( particle_name.find( "Jhin" ) != std::string::npos && particle_name.find( "W_charging" ) !=
            std::string::npos ) {
            instance.range             = 2600.f;
            instance.danger            = 2;
            instance.cc                = false;
            instance.collision         = false;
            instance.radius            = 45.f;
            instance.type              = ESpellType::line;
            instance.particle_type     = EParticleSpell::jhin_w;
            instance.has_edge_radius   = true;
            instance.windup_time       = 0.75f;
            instance.total_cast_time   = 1.f;
            instance.is_particle_spell = true;

            if ( !m_detect_ally_particles ) {
                bool ally_found{ };

                for ( auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || rt_hash( ally->champion_name.text ) !=
                        ct_hash( "Jhin" ) )
                        continue;

                    auto sci = ally->spell_book.get_spell_cast_info( );
                    if ( !sci || sci->slot != 1 ) continue;

                    ally_found = true;
                }

                if ( ally_found ) return { };
            }


            instance.speed      = 0.f;
            instance.base_speed = 0.f;

            bool    found_caster{ };
            Object* caster{ };

            const auto enemies = m_detect_ally_particles ? g_entity_list.get_allies( ) : g_entity_list.get_enemies( );
            for ( auto enemy : enemies ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_visible( ) || rt_hash( enemy->champion_name.text ) !=
                    ct_hash( "Jhin" ) )
                    continue;


                found_caster = true;
                caster       = enemy;

                break;
            }

            if ( !found_caster || !caster ) return { };

            auto data = g_features->tracker->get_last_seen_data( caster->index );


            instance.start_time       = *g_time;
            instance.server_cast_time = data->last_seen_time + 0.75f;
            instance.end_time         = data->last_seen_time + 0.75f;

            if ( *g_time - data->last_seen_time > 0.25f ) {
                instance.server_cast_time = *g_time + 0.75f;
                instance.end_time         = instance.server_cast_time;
            }


            instance.start_pos   = caster->position;
            instance.current_pos = caster->position;

            instance.end_pos = caster->position.extend( caster->position + caster->get_direction( ), instance.range );
            instance.raw_end_pos = instance.end_pos;
            instance.spell_name = "JhinW";

            instance.end_pos.y = g_navgrid->get_height( instance.end_pos );

            auto spell_config = get_spell_config_value( ct_hash( "JhinW" ) );
            if ( spell_config.valid ) {
                instance.allow_dodge             = spell_config.enabled;
                instance.danger                  = spell_config.danger_level;
                instance.ignore_health_threshold = static_cast< float >( spell_config.max_health_to_ignore );
            }

            return instance;
        }

        return { };
    }

    auto SpellDetector::get_dash_spell( const int16_t index ) -> SpellInstance{
        const auto enemy = g_entity_list.get_by_index( index );
        if ( !enemy ) return { };

        SpellInstance instance{ };

        constexpr auto dash_threshold{ 0.35f };

        switch ( rt_hash( enemy->champion_name.text ) ) {
        case ct_hash( "Gragas" ):
        {
            auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell ) return { };

            spell.update( );

            const auto cast_start_time = spell->cooldown_expire - spell->cooldown;
            if ( *g_time - cast_start_time > dash_threshold || *g_time >= spell->cooldown_expire ) return { };;
            instance.spell_name       = _( "GragasE" );
            instance.cc               = true;
            instance.missile_nid      = 0;
            instance.collision        = false;
            instance.type             = ESpellType::line;
            instance.range            = 700.f;
            instance.danger           = 3;
            instance.windup_time      = 0.1f;
            instance.radius           = 100.f;
            instance.source_index     = enemy->index;
            instance.server_cast_time = cast_start_time;
            return instance;
        }
        case ct_hash( "Kayn" ):
        {
            auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell ) return { };

            spell.update( );

            const auto cast_start_time = spell->cooldown_expire - spell->cooldown;
            if ( *g_time - cast_start_time > dash_threshold || *g_time >= spell->cooldown_expire ) return { };;
            instance.spell_name       = _( "KaynQ" );
            instance.cc               = false;
            instance.missile_nid      = 0;
            instance.collision        = false;
            instance.type             = ESpellType::circle;
            instance.range            = 350.f;
            instance.danger           = get_spell_config_value( ct_hash( "KaynQ" ) ).danger_level;
            instance.windup_time      = 0.15f;
            instance.source_index     = enemy->index;
            instance.server_cast_time = cast_start_time;

            return instance;
        }
        case ct_hash( "Khazix" ):
        {
            auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell ) return { };

            spell.update( );

            const auto cast_start_time = spell->cooldown_expire - spell->cooldown;
            if ( *g_time - cast_start_time > dash_threshold || *g_time >= spell->cooldown_expire ) return { };;
            instance.spell_name       = _( "KhazixE" );
            instance.cc               = false;
            instance.missile_nid      = 0;
            instance.collision        = false;
            instance.type             = ESpellType::circle;
            instance.range            = 700.f;
            instance.radius           = 275.f;
            instance.has_edge_radius  = false;
            instance.danger           = 2;
            instance.windup_time      = 0.1f;
            instance.source_index     = enemy->index;
            instance.server_cast_time = cast_start_time;

            return instance;
        }
        case ct_hash( "Malphite" ):
        {
            auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell ) return { };

            spell.update( );

            const auto cast_start_time = spell->cooldown_expire - spell->cooldown;
            if ( *g_time - cast_start_time > dash_threshold || *g_time >= spell->cooldown_expire ) return { };;
            instance.spell_name       = _( "UFSlash" );
            instance.cc               = true;
            instance.missile_nid      = 0;
            instance.collision        = false;
            instance.type             = ESpellType::circle;
            instance.range            = 1000.f;
            instance.radius           = 325.f;
            instance.has_edge_radius  = false;
            instance.danger           = get_spell_config_value( ct_hash( "UFSlash" ) ).danger_level;
            instance.windup_time      = 0.05f;
            instance.source_index     = enemy->index;
            instance.server_cast_time = cast_start_time;

            return instance;
        }
        case ct_hash( "Vi" ):
        {
            auto spell = enemy->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell ) return { };

            spell.update( );

            const auto cast_start_time = spell->cooldown_expire - spell->cooldown;
            if ( *g_time - cast_start_time > dash_threshold || *g_time >= spell->cooldown_expire ) return { };;
            instance.spell_name       = _( "ViQ" );
            instance.cc               = true;
            instance.missile_nid      = 0;
            instance.collision        = false;
            instance.type             = ESpellType::line;
            instance.range            = 700.f;
            instance.danger           = get_spell_config_value( ct_hash( "ViQ" ) ).danger_level;
            instance.windup_time      = 0.1f;
            instance.radius           = 75.f;
            instance.source_index     = enemy->index;
            instance.server_cast_time = cast_start_time;
            return instance;
        }
        default:
            return { };
        }
    }


    auto SpellDetector::get_spell_config_value( hash_t name_hash ) -> SpellConfigValues{
        SpellConfigValues cfg_values{ };

        // TODO: Add SamiraQGun, YorickE, GwenQ, GwenRMis, MilioQ, MilioQHit, MilioQHitMinion, CaitlynE, CaitlynEMissile

        switch ( name_hash ) {
        case ct_hash( "AatroxQ1" ):
        case ct_hash( "AatroxQ2" ):
        case ct_hash( "AatroxQ3" ):
        {
            cfg_values = {
                g_config->evade_spells.aatrox_q_enabled->get< bool >( ),
                g_config->evade_spells.aatrox_q_danger->get< int >( ),
                g_config->evade_spells.aatrox_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AatroxW" ):
        {
            cfg_values = {
                g_config->evade_spells.aatrox_w_enabled->get< bool >( ),
                g_config->evade_spells.aatrox_w_danger->get< int >( ),
                g_config->evade_spells.aatrox_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AhriQ" ):
        case ct_hash( "AhriQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.ahri_q_enabled->get< bool >( ),
                g_config->evade_spells.ahri_q_danger->get< int >( ),
                g_config->evade_spells.ahri_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AhriE" ):
        case ct_hash( "AhriEMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.ahri_e_enabled->get< bool >( ),
                g_config->evade_spells.ahri_e_danger->get< int >( ),
                g_config->evade_spells.ahri_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AkaliE" ):
        {
            cfg_values = {
                g_config->evade_spells.akali_e_enabled->get< bool >( ),
                g_config->evade_spells.akali_e_danger->get< int >( ),
                g_config->evade_spells.akali_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AkshanQ" ):
        {
            cfg_values = {
                g_config->evade_spells.akshan_q_enabled->get< bool >( ),
                g_config->evade_spells.akshan_q_danger->get< int >( ),
                g_config->evade_spells.akshan_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "Pulverize" ):
        {
            cfg_values = {
                g_config->evade_spells.alistar_q_enabled->get< bool >( ),
                g_config->evade_spells.alistar_q_danger->get< int >( ),
                g_config->evade_spells.alistar_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BandageToss" ):
        {
            cfg_values = {
                g_config->evade_spells.amumu_q_enabled->get< bool >( ),
                g_config->evade_spells.amumu_q_danger->get< int >( ),
                g_config->evade_spells.amumu_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CurseoftheSadMummy" ):
        {
            cfg_values = {
                g_config->evade_spells.amumu_r_enabled->get< bool >( ),
                g_config->evade_spells.amumu_r_danger->get< int >( ),
                g_config->evade_spells.amumu_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "FlashFrostSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.anivia_q_enabled->get< bool >( ),
                g_config->evade_spells.anivia_q_danger->get< int >( ),
                g_config->evade_spells.anivia_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AnnieW" ):
        {
            cfg_values = {
                g_config->evade_spells.annie_w_enabled->get< bool >( ),
                g_config->evade_spells.annie_w_danger->get< int >( ),
                g_config->evade_spells.annie_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AnnieR" ):
        {
            cfg_values = {
                g_config->evade_spells.annie_r_enabled->get< bool >( ),
                g_config->evade_spells.annie_r_danger->get< int >( ),
                g_config->evade_spells.annie_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ApheliosCalibrumQ" ):
        case ct_hash( "ApheliosInfernumQ" ):
        {
            cfg_values = {
                g_config->evade_spells.aphelios_q_enabled->get< bool >( ),
                g_config->evade_spells.aphelios_q_danger->get< int >( ),
                g_config->evade_spells.aphelios_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ApheliosR" ):
        {
            cfg_values = {
                g_config->evade_spells.aphelios_r_enabled->get< bool >( ),
                g_config->evade_spells.aphelios_r_danger->get< int >( ),
                g_config->evade_spells.aphelios_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "Volley" ):
        {
            cfg_values = {
                g_config->evade_spells.ashe_w_enabled->get< bool >( ),
                g_config->evade_spells.ashe_w_danger->get< int >( ),
                g_config->evade_spells.ashe_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EnchanceCrystalArrow" ):
        {
            cfg_values = {
                g_config->evade_spells.ashe_r_enabled->get< bool >( ),
                g_config->evade_spells.ashe_r_danger->get< int >( ),
                g_config->evade_spells.ashe_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AurelionSolQ" ):
        { // remove

            cfg_values = {
                g_config->evade_spells.asol_q_enabled->get< bool >( ),
                g_config->evade_spells.asol_q_danger->get< int >( ),
                g_config->evade_spells.asol_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AurelionSolR" ):
        { // remvoe

            cfg_values = {
                g_config->evade_spells.asol_r_enabled->get< bool >( ),
                g_config->evade_spells.asol_r_danger->get< int >( ),
                g_config->evade_spells.asol_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "AzirR" ):
        {
            cfg_values = {
                g_config->evade_spells.azir_r_enabled->get< bool >( ),
                g_config->evade_spells.azir_r_danger->get< int >( ),
                g_config->evade_spells.azir_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BardQ" ):
        {
            cfg_values = {
                g_config->evade_spells.bard_q_enabled->get< bool >( ),
                g_config->evade_spells.bard_q_danger->get< int >( ),
                g_config->evade_spells.bard_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BardR" ):
        {
            cfg_values = {
                g_config->evade_spells.bard_r_enabled->get< bool >( ),
                g_config->evade_spells.bard_r_danger->get< int >( ),
                g_config->evade_spells.bard_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BelvethW" ):
        {
            cfg_values = {
                g_config->evade_spells.belveth_w_enabled->get< bool >( ),
                g_config->evade_spells.belveth_w_danger->get< int >( ),
                g_config->evade_spells.belveth_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RocketGrab" ):
        {
            cfg_values = {
                g_config->evade_spells.blitzcrank_q_enabled->get< bool >( ),
                g_config->evade_spells.blitzcrank_q_danger->get< int >( ),
                g_config->evade_spells.blitzcrank_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BrandQ" ):
        {
            cfg_values = {
                g_config->evade_spells.brand_q_enabled->get< bool >( ),
                g_config->evade_spells.brand_q_danger->get< int >( ),
                g_config->evade_spells.brand_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BrandW" ):
        {
            cfg_values = {
                g_config->evade_spells.brand_w_enabled->get< bool >( ),
                g_config->evade_spells.brand_w_danger->get< int >( ),
                g_config->evade_spells.brand_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BraumQ" ):
        {
            cfg_values = {
                g_config->evade_spells.braum_q_enabled->get< bool >( ),
                g_config->evade_spells.braum_q_danger->get< int >( ),
                g_config->evade_spells.braum_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BraumR" ):
        {
            cfg_values = {
                g_config->evade_spells.braum_r_enabled->get< bool >( ),
                g_config->evade_spells.braum_r_danger->get< int >( ),
                g_config->evade_spells.braum_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CaitlynQ" ):
        {
            cfg_values = {
                g_config->evade_spells.caitlyn_q_enabled->get< bool >( ),
                g_config->evade_spells.caitlyn_q_danger->get< int >( ),
                g_config->evade_spells.caitlyn_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CaitlynEntrapment" ):
        {
            cfg_values = {
                g_config->evade_spells.caitlyn_e_enabled->get< bool >( ),
                g_config->evade_spells.caitlyn_e_danger->get< int >( ),
                g_config->evade_spells.caitlyn_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CamilleE" ):
        case ct_hash( "CamilleEDash2" ):
        {
            cfg_values = {
                g_config->evade_spells.camille_e_enabled->get< bool >( ),
                g_config->evade_spells.camille_e_danger->get< int >( ),
                g_config->evade_spells.camille_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CassiopeiaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.cass_q_enabled->get< bool >( ),
                g_config->evade_spells.cass_q_danger->get< int >( ),
                g_config->evade_spells.cass_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "CassiopeiaR" ):
        {
            cfg_values = {
                g_config->evade_spells.cass_r_enabled->get< bool >( ),
                g_config->evade_spells.cass_r_danger->get< int >( ),
                g_config->evade_spells.cass_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "Rupture" ):
        {
            cfg_values = {
                g_config->evade_spells.chogath_q_enabled->get< bool >( ),
                g_config->evade_spells.chogath_q_danger->get< int >( ),
                g_config->evade_spells.chogath_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "FeralScream" ):
        {
            cfg_values = {
                g_config->evade_spells.chogath_w_enabled->get< bool >( ),
                g_config->evade_spells.chogath_w_danger->get< int >( ),
                g_config->evade_spells.chogath_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PhosphorusBomb" ):
        {
            cfg_values = {
                g_config->evade_spells.corki_q_enabled->get< bool >( ),
                g_config->evade_spells.corki_q_danger->get< int >( ),
                g_config->evade_spells.corki_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MissileBarrageMissile" ):
        case ct_hash( "MissileBarrageMissile2" ):
        {
            cfg_values = {
                g_config->evade_spells.corki_r_enabled->get< bool >( ),
                g_config->evade_spells.corki_r_danger->get< int >( ),
                g_config->evade_spells.corki_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DariusAxeGrabCone" ):
        {
            cfg_values = {
                g_config->evade_spells.darius_e_enabled->get< bool >( ),
                g_config->evade_spells.darius_e_danger->get< int >( ),
                g_config->evade_spells.darius_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DianaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.diana_q_enabled->get< bool >( ),
                g_config->evade_spells.diana_q_danger->get< int >( ),
                g_config->evade_spells.diana_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DianaR" ):
        {
            cfg_values = {
                g_config->evade_spells.diana_r_enabled->get< bool >( ),
                g_config->evade_spells.diana_r_danger->get< int >( ),
                g_config->evade_spells.diana_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DrMundoQ" ):
        case ct_hash( "InfectedCleaverMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.mundo_q_enabled->get< bool >( ),
                g_config->evade_spells.mundo_q_danger->get< int >( ),
                g_config->evade_spells.mundo_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DravenDoubleShot" ):
        {
            cfg_values = {
                g_config->evade_spells.draven_e_enabled->get< bool >( ),
                g_config->evade_spells.draven_e_danger->get< int >( ),
                g_config->evade_spells.draven_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "DravenDoubleShotMissile" ):
        case ct_hash( "DravenRCast" ):
        {
            cfg_values = {
                g_config->evade_spells.draven_r_enabled->get< bool >( ),
                g_config->evade_spells.draven_r_danger->get< int >( ),
                g_config->evade_spells.draven_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EkkoQ" ):
        {
            cfg_values = {
                g_config->evade_spells.ekko_q_enabled->get< bool >( ),
                g_config->evade_spells.ekko_q_danger->get< int >( ),
                g_config->evade_spells.ekko_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EkkoW" ):
        {
            cfg_values = {
                g_config->evade_spells.ekko_w_enabled->get< bool >( ),
                g_config->evade_spells.ekko_w_danger->get< int >( ),
                g_config->evade_spells.ekko_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EliseHumanE" ):
        {
            cfg_values = {
                g_config->evade_spells.elise_e_enabled->get< bool >( ),
                g_config->evade_spells.elise_e_danger->get< int >( ),
                g_config->evade_spells.elise_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EvelynnQ" ):
        {
            cfg_values = {
                g_config->evade_spells.evelynn_q_enabled->get< bool >( ),
                g_config->evade_spells.evelynn_q_danger->get< int >( ),
                g_config->evade_spells.evelynn_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EvelynnR" ):
        {
            cfg_values = {
                g_config->evade_spells.evelynn_r_enabled->get< bool >( ),
                g_config->evade_spells.evelynn_r_danger->get< int >( ),
                g_config->evade_spells.evelynn_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EzrealQ" ):
        {
            cfg_values = {
                g_config->evade_spells.ezreal_q_enabled->get< bool >( ),
                g_config->evade_spells.ezreal_q_danger->get< int >( ),
                g_config->evade_spells.ezreal_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EzrealW" ):
        {
            cfg_values = {
                g_config->evade_spells.ezreal_w_enabled->get< bool >( ),
                g_config->evade_spells.ezreal_w_danger->get< int >( ),
                g_config->evade_spells.ezreal_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "EzrealR" ):
        {
            cfg_values = {
                g_config->evade_spells.ezreal_r_enabled->get< bool >( ),
                g_config->evade_spells.ezreal_r_danger->get< int >( ),
                g_config->evade_spells.ezreal_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "FioraW" ):
        {
            cfg_values = {
                g_config->evade_spells.fiora_w_enabled->get< bool >( ),
                g_config->evade_spells.fiora_w_danger->get< int >( ),
                g_config->evade_spells.fiora_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "FizzR" ):
        {
            cfg_values = {
                g_config->evade_spells.fizz_r_enabled->get< bool >( ),
                g_config->evade_spells.fizz_r_danger->get< int >( ),
                g_config->evade_spells.fizz_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GalioQ" ):
        {
            cfg_values = {
                g_config->evade_spells.galio_q_enabled->get< bool >( ),
                g_config->evade_spells.galio_q_danger->get< int >( ),
                g_config->evade_spells.galio_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GalioE" ):
        {
            cfg_values = {
                g_config->evade_spells.galio_e_enabled->get< bool >( ),
                g_config->evade_spells.galio_e_danger->get< int >( ),
                g_config->evade_spells.galio_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GnarBigQMissile" ):
        case ct_hash( "GnarQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.gnar_q_enabled->get< bool >( ),
                g_config->evade_spells.gnar_q_danger->get< int >( ),
                g_config->evade_spells.gnar_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GnarBigW" ):
        {
            cfg_values = {
                g_config->evade_spells.gnar_w_enabled->get< bool >( ),
                g_config->evade_spells.gnar_w_danger->get< int >( ),
                g_config->evade_spells.gnar_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GnarR" ):
        {
            cfg_values = {
                g_config->evade_spells.gnar_r_enabled->get< bool >( ),
                g_config->evade_spells.gnar_r_danger->get< int >( ),
                g_config->evade_spells.gnar_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GravesQLineSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.graves_q_enabled->get< bool >( ),
                g_config->evade_spells.graves_q_danger->get< int >( ),
                g_config->evade_spells.graves_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "GravesSmokeGrenade" ):
        {
            cfg_values = {
                g_config->evade_spells.graves_w_enabled->get< bool >( ),
                g_config->evade_spells.graves_w_danger->get< int >( ),
                g_config->evade_spells.graves_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "HecarimUlt" ):
        {
            cfg_values = {
                g_config->evade_spells.hecarim_r_enabled->get< bool >( ),
                g_config->evade_spells.hecarim_r_danger->get< int >( ),
                g_config->evade_spells.hecarim_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "HeimerdingerW" ):
        {
            cfg_values = {
                g_config->evade_spells.heimer_w_enabled->get< bool >( ),
                g_config->evade_spells.heimer_w_danger->get< int >( ),
                g_config->evade_spells.heimer_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "HeimerdingerEUlt" ):
        case ct_hash( "HeimerdingerE" ):
        {
            cfg_values = {
                g_config->evade_spells.heimer_e_enabled->get< bool >( ),
                g_config->evade_spells.heimer_e_danger->get< int >( ),
                g_config->evade_spells.heimer_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "IllaoiQ" ):
        {
            cfg_values = {
                g_config->evade_spells.illaoi_q_enabled->get< bool >( ),
                g_config->evade_spells.illaoi_q_danger->get< int >( ),
                g_config->evade_spells.illaoi_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "IllaoiE" ):
        {
            cfg_values = {
                g_config->evade_spells.illaoi_e_enabled->get< bool >( ),
                g_config->evade_spells.illaoi_e_danger->get< int >( ),
                g_config->evade_spells.illaoi_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "IreliaEMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.irelia_e_enabled->get< bool >( ),
                g_config->evade_spells.irelia_e_danger->get< int >( ),
                g_config->evade_spells.irelia_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "IreliaR" ):
        {
            cfg_values = {
                g_config->evade_spells.irelia_r_enabled->get< bool >( ),
                g_config->evade_spells.irelia_r_danger->get< int >( ),
                g_config->evade_spells.irelia_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "IvernQ" ):
        {
            cfg_values = {
                g_config->evade_spells.ivern_q_enabled->get< bool >( ),
                g_config->evade_spells.ivern_q_danger->get< int >( ),
                g_config->evade_spells.ivern_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "HowlingGaleSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.janna_q_enabled->get< bool >( ),
                g_config->evade_spells.janna_q_danger->get< int >( ),
                g_config->evade_spells.janna_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JarvanIVDragonStrike" ):
        {
            cfg_values = {
                g_config->evade_spells.jarvan_q_enabled->get< bool >( ),
                g_config->evade_spells.jarvan_q_danger->get< int >( ),
                g_config->evade_spells.jarvan_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JayceShockBlast" ):
        case ct_hash( "JayceShockBlastWallMis" ):
        {
            cfg_values = {
                g_config->evade_spells.jayce_q_enabled->get< bool >( ),
                g_config->evade_spells.jayce_q_danger->get< int >( ),
                g_config->evade_spells.jayce_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JhinW" ):
        {
            cfg_values = {
                g_config->evade_spells.jhin_w_enabled->get< bool >( ),
                g_config->evade_spells.jhin_w_danger->get< int >( ),
                g_config->evade_spells.jhin_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JhinRShotMis4" ):
        case ct_hash( "JhinRShotMis" ):
        {
            cfg_values = {
                g_config->evade_spells.jhin_r_enabled->get< bool >( ),
                g_config->evade_spells.jhin_r_danger->get< int >( ),
                g_config->evade_spells.jhin_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JinxWMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.jinx_w_enabled->get< bool >( ),
                g_config->evade_spells.jinx_w_danger->get< int >( ),
                g_config->evade_spells.jinx_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JinxR" ):
        {
            cfg_values = {
                g_config->evade_spells.jinx_r_enabled->get< bool >( ),
                g_config->evade_spells.jinx_r_danger->get< int >( ),
                g_config->evade_spells.jinx_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KaisaW" ):
        {
            cfg_values = {
                g_config->evade_spells.kaisa_w_enabled->get< bool >( ),
                g_config->evade_spells.kaisa_w_danger->get< int >( ),
                g_config->evade_spells.kaisa_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KarmaQMissile" ):
        case ct_hash( "KarmaQMissileMantra" ):
        case ct_hash( "KarmaQMantra" ):
        case ct_hash( "KarmaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.karma_q_enabled->get< bool >( ),
                g_config->evade_spells.karma_q_danger->get< int >( ),
                g_config->evade_spells.karma_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KarthusLayWasteA1" ):
        case ct_hash( "KarthusLayWasteA2" ):
        case ct_hash( "KarthusLayWasteA3" ):
        {
            cfg_values = {
                g_config->evade_spells.karthus_q_enabled->get< bool >( ),
                g_config->evade_spells.karthus_q_danger->get< int >( ),
                g_config->evade_spells.karthus_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RiftWalk" ):
        {
            cfg_values = {
                g_config->evade_spells.kassadin_r_enabled->get< bool >( ),
                g_config->evade_spells.kassadin_r_danger->get< int >( ),
                g_config->evade_spells.kassadin_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KayleQ" ):
        {
            cfg_values = {
                g_config->evade_spells.kayle_q_enabled->get< bool >( ),
                g_config->evade_spells.kayle_q_danger->get< int >( ),
                g_config->evade_spells.kayle_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KaynQ" ):
        {
            cfg_values = {
                g_config->evade_spells.kayn_q_enabled->get< bool >( ),
                g_config->evade_spells.kayn_q_danger->get< int >( ),
                g_config->evade_spells.kayn_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KaynW" ):
        {
            cfg_values = {
                g_config->evade_spells.kayn_w_enabled->get< bool >( ),
                g_config->evade_spells.kayn_w_danger->get< int >( ),
                g_config->evade_spells.kayn_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KennenShurikenHurlMissile1" ):
        {
            cfg_values = {
                g_config->evade_spells.kennen_q_enabled->get< bool >( ),
                g_config->evade_spells.kennen_q_danger->get< int >( ),
                g_config->evade_spells.kennen_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KhazixW" ):
        {
            cfg_values = {
                g_config->evade_spells.khazix_w_enabled->get< bool >( ),
                g_config->evade_spells.khazix_w_danger->get< int >( ),
                g_config->evade_spells.khazix_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KledQ" ):
        case ct_hash( "KledRiderQ" ):
        {
            cfg_values = {
                g_config->evade_spells.kled_q_enabled->get< bool >( ),
                g_config->evade_spells.kled_q_danger->get< int >( ),
                g_config->evade_spells.kled_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KledEDash" ):
        {
            cfg_values = {
                g_config->evade_spells.kled_e_enabled->get< bool >( ),
                g_config->evade_spells.kled_e_danger->get< int >( ),
                g_config->evade_spells.kled_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KogMawQ" ):
        {
            cfg_values = {
                g_config->evade_spells.kogmaw_q_enabled->get< bool >( ),
                g_config->evade_spells.kogmaw_q_danger->get< int >( ),
                g_config->evade_spells.kogmaw_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KogMawVoidOozeMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.kogmaw_e_enabled->get< bool >( ),
                g_config->evade_spells.kogmaw_e_danger->get< int >( ),
                g_config->evade_spells.kogmaw_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "KogMawLivingArtillery" ):
        {
            cfg_values = {
                g_config->evade_spells.kogmaw_r_enabled->get< bool >( ),
                g_config->evade_spells.kogmaw_r_danger->get< int >( ),
                g_config->evade_spells.kogmaw_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LeblancE" ):
        case ct_hash( "LeblancEMissile" ):
        case ct_hash( "LeblancREM" ):
        case ct_hash( "LeblancREMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.leblanc_e_enabled->get< bool >( ),
                g_config->evade_spells.leblanc_e_danger->get< int >( ),
                g_config->evade_spells.leblanc_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "BlindMonkQOne" ):
        {
            cfg_values = {
                g_config->evade_spells.leesin_q_enabled->get< bool >( ),
                g_config->evade_spells.leesin_q_danger->get< int >( ),
                g_config->evade_spells.leesin_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LeonaZenithBladeMissile" ):
        case ct_hash( "LeonaZenithBlade" ):
        {
            cfg_values = {
                g_config->evade_spells.leona_e_enabled->get< bool >( ),
                g_config->evade_spells.leona_e_danger->get< int >( ),
                g_config->evade_spells.leona_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LeonaSolarFlare" ):
        {
            cfg_values = {
                g_config->evade_spells.leona_r_enabled->get< bool >( ),
                g_config->evade_spells.leona_r_danger->get< int >( ),
                g_config->evade_spells.leona_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LilliaW" ):
        {
            cfg_values = {
                g_config->evade_spells.lillia_w_enabled->get< bool >( ),
                g_config->evade_spells.lillia_w_danger->get< int >( ),
                g_config->evade_spells.lillia_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LilliaE" ):
        case ct_hash( "LilliaERollingMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.lillia_e_enabled->get< bool >( ),
                g_config->evade_spells.lillia_e_danger->get< int >( ),
                g_config->evade_spells.lillia_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LissandraQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.lissandra_q_enabled->get< bool >( ),
                g_config->evade_spells.lissandra_q_danger->get< int >( ),
                g_config->evade_spells.lissandra_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LissandraEMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.lissandra_e_enabled->get< bool >( ),
                g_config->evade_spells.lissandra_e_danger->get< int >( ),
                g_config->evade_spells.lissandra_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LucianQ" ):
        {
            cfg_values = {
                g_config->evade_spells.lucian_q_enabled->get< bool >( ),
                g_config->evade_spells.lucian_q_danger->get< int >( ),
                g_config->evade_spells.lucian_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LucianW" ):
        {
            cfg_values = {
                g_config->evade_spells.lucian_w_enabled->get< bool >( ),
                g_config->evade_spells.lucian_w_danger->get< int >( ),
                g_config->evade_spells.lucian_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LuluQ" ):
        {
            cfg_values = {
                g_config->evade_spells.lulu_q_enabled->get< bool >( ),
                g_config->evade_spells.lulu_q_danger->get< int >( ),
                g_config->evade_spells.lulu_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LuxLightBindingMis" ):
        case ct_hash( "LuxLightBindingDummy" ):
        case ct_hash( "LuxLightBinding" ):
        {
            cfg_values = {
                g_config->evade_spells.lux_q_enabled->get< bool >( ),
                g_config->evade_spells.lux_q_danger->get< int >( ),
                g_config->evade_spells.lux_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LuxLightStrikeKugel" ):
        {
            cfg_values = {
                g_config->evade_spells.lux_e_enabled->get< bool >( ),
                g_config->evade_spells.lux_e_danger->get< int >( ),
                g_config->evade_spells.lux_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "LuxRMis" ):
        {
            cfg_values = {
                g_config->evade_spells.lux_r_enabled->get< bool >( ),
                g_config->evade_spells.lux_r_danger->get< int >( ),
                g_config->evade_spells.lux_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "UFSlash" ):
        {
            cfg_values = {
                g_config->evade_spells.malphite_r_enabled->get< bool >( ),
                g_config->evade_spells.malphite_r_danger->get< int >( ),
                g_config->evade_spells.malphite_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MalzaharQ" ):
        {
            cfg_values = {
                g_config->evade_spells.malz_q_enabled->get< bool >( ),
                g_config->evade_spells.malz_q_danger->get< int >( ),
                g_config->evade_spells.malz_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MissFortuneBulletTime" ):
        {
            cfg_values = {
                g_config->evade_spells.mf_e_enabled->get< bool >( ),
                g_config->evade_spells.mf_e_danger->get< int >( ),
                g_config->evade_spells.mf_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MordekaiserQ" ):
        {
            cfg_values = {
                g_config->evade_spells.mord_q_enabled->get< bool >( ),
                g_config->evade_spells.mord_q_danger->get< int >( ),
                g_config->evade_spells.mord_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MordekaiserE" ):
        {
            cfg_values = {
                g_config->evade_spells.mord_e_enabled->get< bool >( ),
                g_config->evade_spells.mord_e_danger->get< int >( ),
                g_config->evade_spells.mord_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "MorganaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.morg_q_enabled->get< bool >( ),
                g_config->evade_spells.morg_q_danger->get< int >( ),
                g_config->evade_spells.morg_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NamiQ" ):
        {
            cfg_values = {
                g_config->evade_spells.nami_q_enabled->get< bool >( ),
                g_config->evade_spells.nami_q_danger->get< int >( ),
                g_config->evade_spells.nami_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NamiRMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.nami_r_enabled->get< bool >( ),
                g_config->evade_spells.nami_r_danger->get< int >( ),
                g_config->evade_spells.nami_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NautilusAnchorDragMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.naut_q_enabled->get< bool >( ),
                g_config->evade_spells.naut_q_danger->get< int >( ),
                g_config->evade_spells.naut_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NeekoQ" ):
        {
            cfg_values = {
                g_config->evade_spells.neeko_q_enabled->get< bool >( ),
                g_config->evade_spells.neeko_q_danger->get< int >( ),
                g_config->evade_spells.neeko_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NeekoE" ):
        {
            cfg_values = {
                g_config->evade_spells.neeko_e_enabled->get< bool >( ),
                g_config->evade_spells.neeko_e_danger->get< int >( ),
                g_config->evade_spells.neeko_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "JavelinToss" ):
        {
            cfg_values = {
                g_config->evade_spells.nidalee_q_enabled->get< bool >( ),
                g_config->evade_spells.nidalee_q_danger->get< int >( ),
                g_config->evade_spells.nidalee_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NocturneDuskbringer" ):
        {
            cfg_values = {
                g_config->evade_spells.nocturne_q_enabled->get< bool >( ),
                g_config->evade_spells.nocturne_q_danger->get< int >( ),
                g_config->evade_spells.nocturne_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "NunuR" ):
        {
            cfg_values = {
                g_config->evade_spells.nunu_r_enabled->get< bool >( ),
                g_config->evade_spells.nunu_r_danger->get< int >( ),
                g_config->evade_spells.nunu_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "OlafAxeThrowCast" ):
        case ct_hash( "OlafAxeThrow" ):
        {
            cfg_values = {
                g_config->evade_spells.olaf_q_enabled->get< bool >( ),
                g_config->evade_spells.olaf_q_danger->get< int >( ),
                g_config->evade_spells.olaf_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "OrianaIzuna" ):
        {
            cfg_values = {
                g_config->evade_spells.orianna_q_enabled->get< bool >( ),
                g_config->evade_spells.orianna_q_danger->get< int >( ),
                g_config->evade_spells.orianna_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "OrnnRCharge" ):
        {
            cfg_values = {
                g_config->evade_spells.ornn_r_enabled->get< bool >( ),
                g_config->evade_spells.ornn_r_danger->get< int >( ),
                g_config->evade_spells.ornn_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PantheonQTap" ):
        case ct_hash( "PantheonQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.panth_q_enabled->get< bool >( ),
                g_config->evade_spells.panth_q_danger->get< int >( ),
                g_config->evade_spells.panth_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PantheonR" ):
        {
            cfg_values = {
                g_config->evade_spells.panth_r_enabled->get< bool >( ),
                g_config->evade_spells.panth_r_danger->get< int >( ),
                g_config->evade_spells.panth_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PoppyQSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.poppy_q_enabled->get< bool >( ),
                g_config->evade_spells.poppy_q_danger->get< int >( ),
                g_config->evade_spells.poppy_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PoppyRSpellInstant" ):
        case ct_hash( "PoppyRSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.poppy_r_enabled->get< bool >( ),
                g_config->evade_spells.poppy_r_danger->get< int >( ),
                g_config->evade_spells.poppy_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PykeQMelee" ):
        case ct_hash( "PykeQRange" ):
        {
            cfg_values = {
                g_config->evade_spells.pyke_q_enabled->get< bool >( ),
                g_config->evade_spells.pyke_q_danger->get< int >( ),
                g_config->evade_spells.pyke_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PykeE" ):
        {
            cfg_values = {
                g_config->evade_spells.pyke_e_enabled->get< bool >( ),
                g_config->evade_spells.pyke_e_danger->get< int >( ),
                g_config->evade_spells.pyke_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "PykeR" ):
        {
            cfg_values = {
                g_config->evade_spells.pyke_r_enabled->get< bool >( ),
                g_config->evade_spells.pyke_r_danger->get< int >( ),
                g_config->evade_spells.pyke_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "QiyanaQ_Rock" ):
        case ct_hash( "QiyanaQ_Grass" ):
        case ct_hash( "QiyanaQ_Water" ):
        case ct_hash( "QiyanaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.qiyana_q_enabled->get< bool >( ),
                g_config->evade_spells.qiyana_q_danger->get< int >( ),
                g_config->evade_spells.qiyana_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "QiyanaR" ):
        {
            cfg_values = {
                g_config->evade_spells.qiyana_r_enabled->get< bool >( ),
                g_config->evade_spells.qiyana_r_danger->get< int >( ),
                g_config->evade_spells.qiyana_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "QuinnQ" ):
        {
            cfg_values = {
                g_config->evade_spells.quinn_q_enabled->get< bool >( ),
                g_config->evade_spells.quinn_q_danger->get< int >( ),
                g_config->evade_spells.quinn_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RakanQ" ):
        {
            cfg_values = {
                g_config->evade_spells.rakan_q_enabled->get< bool >( ),
                g_config->evade_spells.rakan_q_danger->get< int >( ),
                g_config->evade_spells.rakan_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RakanW" ):
        {
            cfg_values = {
                g_config->evade_spells.rakan_w_enabled->get< bool >( ),
                g_config->evade_spells.rakan_w_danger->get< int >( ),
                g_config->evade_spells.rakan_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ReksaiQBurrowed" ):
        {
            cfg_values = {
                g_config->evade_spells.reksai_q_enabled->get< bool >( ),
                g_config->evade_spells.reksai_q_danger->get< int >( ),
                g_config->evade_spells.reksai_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RellR" ):
        {
            cfg_values = {
                g_config->evade_spells.rell_r_enabled->get< bool >( ),
                g_config->evade_spells.rell_r_danger->get< int >( ),
                g_config->evade_spells.rell_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RenataQ" ):
        {
            cfg_values = {
                g_config->evade_spells.renata_q_enabled->get< bool >( ),
                g_config->evade_spells.renata_q_danger->get< int >( ),
                g_config->evade_spells.renata_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RenataE" ):
        {
            cfg_values = {
                g_config->evade_spells.renata_e_enabled->get< bool >( ),
                g_config->evade_spells.renata_e_danger->get< int >( ),
                g_config->evade_spells.renata_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RenataR" ):
        {
            cfg_values = {
                g_config->evade_spells.renata_r_enabled->get< bool >( ),
                g_config->evade_spells.renata_r_danger->get< int >( ),
                g_config->evade_spells.renata_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RengarEEmp" ):
        case ct_hash( "RengarEMis" ):
        case ct_hash( "RengarE" ):
        {
            cfg_values = {
                g_config->evade_spells.rengar_e_enabled->get< bool >( ),
                g_config->evade_spells.rengar_e_danger->get< int >( ),
                g_config->evade_spells.rengar_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RivenIzunaBlade" ):
        {
            cfg_values = {
                g_config->evade_spells.riven_r_enabled->get< bool >( ),
                g_config->evade_spells.riven_r_danger->get< int >( ),
                g_config->evade_spells.riven_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RumbleGrenade" ):
        {
            cfg_values = {
                g_config->evade_spells.rumble_e_enabled->get< bool >( ),
                g_config->evade_spells.rumble_e_danger->get< int >( ),
                g_config->evade_spells.rumble_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "RyzeQWrapper" ):
        {
            cfg_values = {
                g_config->evade_spells.ryze_q_enabled->get< bool >( ),
                g_config->evade_spells.ryze_q_danger->get< int >( ),
                g_config->evade_spells.ryze_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SejuaniR" ):
        {
            cfg_values = {
                g_config->evade_spells.sejuani_r_enabled->get< bool >( ),
                g_config->evade_spells.sejuani_r_danger->get< int >( ),
                g_config->evade_spells.sejuani_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SennaQCast" ):
        {
            cfg_values = {
                g_config->evade_spells.senna_q_enabled->get< bool >( ),
                g_config->evade_spells.senna_q_danger->get< int >( ),
                g_config->evade_spells.senna_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SennaW" ):
        {
            cfg_values = {
                g_config->evade_spells.senna_w_enabled->get< bool >( ),
                g_config->evade_spells.senna_w_danger->get< int >( ),
                g_config->evade_spells.senna_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SennaR" ):
        {
            cfg_values = {
                g_config->evade_spells.senna_r_enabled->get< bool >( ),
                g_config->evade_spells.senna_r_danger->get< int >( ),
                g_config->evade_spells.senna_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SeraphineQCast" ):
        {
            cfg_values = {
                g_config->evade_spells.seraphine_q_enabled->get< bool >( ),
                g_config->evade_spells.seraphine_q_danger->get< int >( ),
                g_config->evade_spells.seraphine_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SeraphineECast" ):
        case ct_hash( "SeraphineECastMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.seraphine_e_enabled->get< bool >( ),
                g_config->evade_spells.seraphine_e_danger->get< int >( ),
                g_config->evade_spells.seraphine_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SeraphineR" ):
        {
            cfg_values = {
                g_config->evade_spells.seraphine_r_enabled->get< bool >( ),
                g_config->evade_spells.seraphine_r_danger->get< int >( ),
                g_config->evade_spells.seraphine_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ShenE" ):
        {
            cfg_values = {
                g_config->evade_spells.shen_e_enabled->get< bool >( ),
                g_config->evade_spells.shen_e_danger->get< int >( ),
                g_config->evade_spells.shen_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ShyvanaFireballDragon2" ):
        {
            cfg_values = {
                g_config->evade_spells.shyv_e_enabled->get< bool >( ),
                g_config->evade_spells.shyv_e_danger->get< int >( ),
                g_config->evade_spells.shyv_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SionQ" ):
        {
            cfg_values = {
                g_config->evade_spells.sion_q_enabled->get< bool >( ),
                g_config->evade_spells.sion_q_danger->get< int >( ),
                g_config->evade_spells.sion_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SionEMissile" ):
        case ct_hash( "SionE" ):
        {
            cfg_values = {
                g_config->evade_spells.sion_e_enabled->get< bool >( ),
                g_config->evade_spells.sion_e_danger->get< int >( ),
                g_config->evade_spells.sion_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SivirQMissile" ):
        case ct_hash( "SivirQ" ):
        {
            cfg_values = {
                g_config->evade_spells.sivir_q_enabled->get< bool >( ),
                g_config->evade_spells.sivir_q_danger->get< int >( ),
                g_config->evade_spells.sivir_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SonaR" ):
        {
            cfg_values = {
                g_config->evade_spells.sona_r_enabled->get< bool >( ),
                g_config->evade_spells.sona_r_danger->get< int >( ),
                g_config->evade_spells.sona_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SorakaQ" ):
        {
            cfg_values = {
                g_config->evade_spells.soraka_q_enabled->get< bool >( ),
                g_config->evade_spells.soraka_q_danger->get< int >( ),
                g_config->evade_spells.soraka_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SwainW" ):
        {
            cfg_values = {
                g_config->evade_spells.swain_w_enabled->get< bool >( ),
                g_config->evade_spells.swain_w_danger->get< int >( ),
                g_config->evade_spells.swain_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SwainE" ):
        {
            cfg_values = {
                g_config->evade_spells.swain_e_enabled->get< bool >( ),
                g_config->evade_spells.swain_e_danger->get< int >( ),
                g_config->evade_spells.swain_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SylasQ" ):
        {
            cfg_values = {
                g_config->evade_spells.sylas_q_enabled->get< bool >( ),
                g_config->evade_spells.sylas_q_danger->get< int >( ),
                g_config->evade_spells.sylas_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SylasE2" ):
        {
            cfg_values = {
                g_config->evade_spells.sylas_e_enabled->get< bool >( ),
                g_config->evade_spells.sylas_e_danger->get< int >( ),
                g_config->evade_spells.sylas_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SyndraQSpell" ):
        {
            cfg_values = {
                g_config->evade_spells.syndra_q_enabled->get< bool >( ),
                g_config->evade_spells.syndra_q_danger->get< int >( ),
                g_config->evade_spells.syndra_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "SyndraE" ):
        case ct_hash( "SyndraE5" ):
        case ct_hash( "SyndraESphereMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.syndra_e_enabled->get< bool >( ),
                g_config->evade_spells.syndra_e_danger->get< int >( ),
                g_config->evade_spells.syndra_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TahmKenchQ" ):
        {
            cfg_values = {
                g_config->evade_spells.kench_q_enabled->get< bool >( ),
                g_config->evade_spells.kench_q_danger->get< int >( ),
                g_config->evade_spells.kench_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TahmKenchW" ):
        {
            cfg_values = {
                g_config->evade_spells.kench_w_enabled->get< bool >( ),
                g_config->evade_spells.kench_w_danger->get< int >( ),
                g_config->evade_spells.kench_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TaliyahQ" ):
        {
            cfg_values = {
                g_config->evade_spells.taliyah_q_enabled->get< bool >( ),
                g_config->evade_spells.taliyah_q_danger->get< int >( ),
                g_config->evade_spells.taliyah_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TaliyahWVC" ):
        {
            cfg_values = {
                g_config->evade_spells.taliyah_w_enabled->get< bool >( ),
                g_config->evade_spells.taliyah_w_danger->get< int >( ),
                g_config->evade_spells.taliyah_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TalonW" ):
        {
            cfg_values = {
                g_config->evade_spells.talon_w_enabled->get< bool >( ),
                g_config->evade_spells.talon_w_danger->get< int >( ),
                g_config->evade_spells.talon_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ThreshQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.thresh_q_enabled->get< bool >( ),
                g_config->evade_spells.thresh_q_danger->get< int >( ),
                g_config->evade_spells.thresh_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ThreshEFlay" ):
        {
            cfg_values = {
                g_config->evade_spells.thresh_e_enabled->get< bool >( ),
                g_config->evade_spells.thresh_e_danger->get< int >( ),
                g_config->evade_spells.thresh_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TryndamereE" ):
        {
            cfg_values = {
                g_config->evade_spells.trynd_e_enabled->get< bool >( ),
                g_config->evade_spells.trynd_e_danger->get< int >( ),
                g_config->evade_spells.trynd_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "WildCards" ):
        {
            cfg_values = {
                g_config->evade_spells.tf_q_enabled->get< bool >( ),
                g_config->evade_spells.tf_q_danger->get< int >( ),
                g_config->evade_spells.tf_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "TwitchVenomCask" ):
        {
            cfg_values = {
                g_config->evade_spells.twitch_w_enabled->get< bool >( ),
                g_config->evade_spells.twitch_w_danger->get< int >( ),
                g_config->evade_spells.twitch_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "UrgotQ" ):
        {
            cfg_values = {
                g_config->evade_spells.urgot_q_enabled->get< bool >( ),
                g_config->evade_spells.urgot_q_danger->get< int >( ),
                g_config->evade_spells.urgot_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "UrgotE" ):
        {
            cfg_values = {
                g_config->evade_spells.urgot_e_enabled->get< bool >( ),
                g_config->evade_spells.urgot_e_danger->get< int >( ),
                g_config->evade_spells.urgot_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "UrgotR" ):
        {
            cfg_values = {
                g_config->evade_spells.urgot_r_enabled->get< bool >( ),
                g_config->evade_spells.urgot_r_danger->get< int >( ),
                g_config->evade_spells.urgot_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VarusQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.varus_q_enabled->get< bool >( ),
                g_config->evade_spells.varus_q_danger->get< int >( ),
                g_config->evade_spells.varus_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VarusE" ):
        {
            cfg_values = {
                g_config->evade_spells.varus_e_enabled->get< bool >( ),
                g_config->evade_spells.varus_e_danger->get< int >( ),
                g_config->evade_spells.varus_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VarusR" ):
        case ct_hash( "VarusRMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.varus_r_enabled->get< bool >( ),
                g_config->evade_spells.varus_r_danger->get< int >( ),
                g_config->evade_spells.varus_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VeigarBalefulStrike" ):
        {
            cfg_values = {
                g_config->evade_spells.veigar_q_enabled->get< bool >( ),
                g_config->evade_spells.veigar_q_danger->get< int >( ),
                g_config->evade_spells.veigar_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VeigarDarkMatterCastLockout" ):
        {
            cfg_values = {
                g_config->evade_spells.veigar_w_enabled->get< bool >( ),
                g_config->evade_spells.veigar_w_danger->get< int >( ),
                g_config->evade_spells.veigar_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VelkozQ" ):
        case ct_hash( "VelkozQMissile" ):
        case ct_hash( "VelkozQMissileSplit" ):
        {
            cfg_values = {
                g_config->evade_spells.velkoz_q_enabled->get< bool >( ),
                g_config->evade_spells.velkoz_q_danger->get< int >( ),
                g_config->evade_spells.velkoz_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VelkozW" ):
        case ct_hash( "VelkozWMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.velkoz_w_enabled->get< bool >( ),
                g_config->evade_spells.velkoz_w_danger->get< int >( ),
                g_config->evade_spells.velkoz_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VelkozE" ):
        case ct_hash( "VelkozEMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.velkoz_e_enabled->get< bool >( ),
                g_config->evade_spells.velkoz_e_danger->get< int >( ),
                g_config->evade_spells.velkoz_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VexQ" ):
        {
            cfg_values = {
                g_config->evade_spells.vex_q_enabled->get< bool >( ),
                g_config->evade_spells.vex_q_danger->get< int >( ),
                g_config->evade_spells.vex_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VexE" ):
        {
            cfg_values = {
                g_config->evade_spells.vex_e_enabled->get< bool >( ),
                g_config->evade_spells.vex_e_danger->get< int >( ),
                g_config->evade_spells.vex_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "VexR" ):
        {
            cfg_values = {
                g_config->evade_spells.vex_r_enabled->get< bool >( ),
                g_config->evade_spells.vex_r_danger->get< int >( ),
                g_config->evade_spells.vex_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViegoQ" ):
        {
            cfg_values = {
                g_config->evade_spells.viego_q_enabled->get< bool >( ),
                g_config->evade_spells.viego_q_danger->get< int >( ),
                g_config->evade_spells.viego_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViegoWMis" ):
        {
            cfg_values = {
                g_config->evade_spells.viego_w_enabled->get< bool >( ),
                g_config->evade_spells.viego_w_danger->get< int >( ),
                g_config->evade_spells.viego_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViegoR" ):
        {
            cfg_values = {
                g_config->evade_spells.viego_r_enabled->get< bool >( ),
                g_config->evade_spells.viego_r_danger->get< int >( ),
                g_config->evade_spells.viego_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViQ" ):
        {
            cfg_values = {
                g_config->evade_spells.vi_q_enabled->get< bool >( ),
                g_config->evade_spells.vi_q_danger->get< int >( ),
                g_config->evade_spells.vi_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViktorW" ):
        {
            cfg_values = {
                g_config->evade_spells.viktor_w_enabled->get< bool >( ),
                g_config->evade_spells.viktor_w_danger->get< int >( ),
                g_config->evade_spells.viktor_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ViktorEAugMissile" ):
        case ct_hash( "ViktorEAugParticle" ):
        case ct_hash( "ViktorDeathRayMissile" ):
        case ct_hash( "ViktorDeathRayMissile2" ):
        {
            cfg_values = {
                g_config->evade_spells.viktor_e_enabled->get< bool >( ),
                g_config->evade_spells.viktor_e_danger->get< int >( ),
                g_config->evade_spells.viktor_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "WarwickR" ):
        {
            cfg_values = {
                g_config->evade_spells.warwick_r_enabled->get< bool >( ),
                g_config->evade_spells.warwick_r_danger->get< int >( ),
                g_config->evade_spells.warwick_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "XerathArcanopulse2" ):
        {
            cfg_values = {
                g_config->evade_spells.xerath_q_enabled->get< bool >( ),
                g_config->evade_spells.xerath_q_danger->get< int >( ),
                g_config->evade_spells.xerath_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "XerathArcaneBarrage2" ):
        {
            cfg_values = {
                g_config->evade_spells.xerath_w_enabled->get< bool >( ),
                g_config->evade_spells.xerath_w_danger->get< int >( ),
                g_config->evade_spells.xerath_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "XerathMageSpear" ):
        case ct_hash( "XerathMageSpearMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.xerath_e_enabled->get< bool >( ),
                g_config->evade_spells.xerath_e_danger->get< int >( ),
                g_config->evade_spells.xerath_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "XerathLocusPulse" ):
        {
            cfg_values = {
                g_config->evade_spells.xerath_r_enabled->get< bool >( ),
                g_config->evade_spells.xerath_r_danger->get< int >( ),
                g_config->evade_spells.xerath_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "XinZhaoW" ):
        {
            cfg_values = {
                g_config->evade_spells.xin_w_enabled->get< bool >( ),
                g_config->evade_spells.xin_w_danger->get< int >( ),
                g_config->evade_spells.xin_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "YasuoQ1" ):
        case ct_hash( "YasuoQ2" ):
        case ct_hash( "YasuoQ3" ):
        case ct_hash( "YasuoQ3Mis" ):
        {
            cfg_values = {
                g_config->evade_spells.yasuo_q_enabled->get< bool >( ),
                g_config->evade_spells.yasuo_q_danger->get< int >( ),
                g_config->evade_spells.yasuo_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "YoneQ" ):
        case ct_hash( "YoneQ3" ):
        case ct_hash( "YoneQ3Missile" ):
        {
            cfg_values = {
                g_config->evade_spells.yone_q_enabled->get< bool >( ),
                g_config->evade_spells.yone_q_danger->get< int >( ),
                g_config->evade_spells.yone_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "YoneR" ):
        {
            cfg_values = {
                g_config->evade_spells.yone_r_enabled->get< bool >( ),
                g_config->evade_spells.yone_r_danger->get< int >( ),
                g_config->evade_spells.yone_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZacQ" ):
        {
            cfg_values = {
                g_config->evade_spells.zac_q_enabled->get< bool >( ),
                g_config->evade_spells.zac_q_danger->get< int >( ),
                g_config->evade_spells.zac_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZedQ" ):
        case ct_hash( "ZedQMissile" ):
        {
            cfg_values = {
                g_config->evade_spells.zed_q_enabled->get< bool >( ),
                g_config->evade_spells.zed_q_danger->get< int >( ),
                g_config->evade_spells.zed_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZeriQ" ):
        {
            cfg_values = {
                g_config->evade_spells.zeri_q_enabled->get< bool >( ),
                g_config->evade_spells.zeri_q_danger->get< int >( ),
                g_config->evade_spells.zeri_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZeriW" ):
        {
            cfg_values = {
                g_config->evade_spells.zeri_w_enabled->get< bool >( ),
                g_config->evade_spells.zeri_w_danger->get< int >( ),
                g_config->evade_spells.zeri_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZiggsQ" ):
        {
            cfg_values = {
                g_config->evade_spells.ziggs_q_enabled->get< bool >( ),
                g_config->evade_spells.ziggs_q_danger->get< int >( ),
                g_config->evade_spells.ziggs_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZiggsW" ):
        {
            cfg_values = {
                g_config->evade_spells.ziggs_w_enabled->get< bool >( ),
                g_config->evade_spells.ziggs_w_danger->get< int >( ),
                g_config->evade_spells.ziggs_w_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZiggsE" ):
        {
            cfg_values = {
                g_config->evade_spells.ziggs_e_enabled->get< bool >( ),
                g_config->evade_spells.ziggs_e_danger->get< int >( ),
                g_config->evade_spells.ziggs_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZiggsR" ):
        {
            cfg_values = {
                g_config->evade_spells.ziggs_r_enabled->get< bool >( ),
                g_config->evade_spells.ziggs_r_danger->get< int >( ),
                g_config->evade_spells.ziggs_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZileanQ" ):
        {
            cfg_values = {
                g_config->evade_spells.zilean_q_enabled->get< bool >( ),
                g_config->evade_spells.zilean_q_danger->get< int >( ),
                g_config->evade_spells.zilean_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZoeQMissile" ):
        case ct_hash( "ZoeQMis2Warning" ):
        case ct_hash( "ZoeQMis2" ):
        {
            cfg_values = {
                g_config->evade_spells.zoe_q_enabled->get< bool >( ),
                g_config->evade_spells.zoe_q_danger->get< int >( ),
                g_config->evade_spells.zoe_q_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZoeE" ):
        case ct_hash( "ZoeEMisAudio" ):
        {
            cfg_values = {
                g_config->evade_spells.zoe_e_enabled->get< bool >( ),
                g_config->evade_spells.zoe_e_danger->get< int >( ),
                g_config->evade_spells.zoe_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZyraE" ):
        {
            cfg_values = {
                g_config->evade_spells.zyra_e_enabled->get< bool >( ),
                g_config->evade_spells.zyra_e_danger->get< int >( ),
                g_config->evade_spells.zyra_e_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        case ct_hash( "ZyraR" ):
        {
            cfg_values = {
                g_config->evade_spells.zyra_r_enabled->get< bool >( ),
                g_config->evade_spells.zyra_r_danger->get< int >( ),
                g_config->evade_spells.zyra_r_min_dodge_health->get< int >( )
            };

            cfg_values.valid = true;
            break;
        }
        default:
            break;
        }

        return cfg_values;
    }
}
