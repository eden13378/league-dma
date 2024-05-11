#include "pch.hpp"

#include "../custom_structs.hpp"
#include "../state.hpp"
#include "../../renderer/color.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/buff_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"
#include "../../sdk/game/inventory.hpp"
#include "../../sdk/game/inventory_slot.hpp"
#include "../../sdk/game/item_data.hpp"
#include "../../sdk/game/league_string.hpp"
#include "../../sdk/game/menugui.hpp"
#include "../../sdk/game/navgrid.hpp"
#include "../../sdk/game/spell_book.hpp"
#include "../../sdk/game/spell_details.hpp"
#include "../../sdk/game/tactical_map.hpp"
#include "../../sdk/game/unit_info_component.hpp"
#include "../../sdk/game/object.hpp"
#include "../../sdk/game/spell_info.hpp"
#include "../../sdk/game/spell_data.hpp"

namespace lua {
    
    auto LuaState::register_sdk_types( sol::state& state ) -> void{
        using AiManager = sdk::game::AiManager;

        state.new_usertype< AiManager >(
            "c_ai_manager",
            "get_path_end_position",
            sol::resolve( &AiManager::get_path_end_position ),
            "velocity",
            sol::readonly( &AiManager::velocity ),
            "is_dashing",
            sol::readonly( &AiManager::is_dashing ),
            "dash_speed",
            sol::readonly( &AiManager::dash_speed ),
            "is_moving",
            sol::readonly( &AiManager::is_moving ),
            "next_path_node",
            sol::readonly( &AiManager::next_path_node ),
            "path_start",
            sol::resolve( &AiManager::path_start ),
            "path_end",
            sol::resolve( &AiManager::path_end ),
            "path",
            sol::property( &AiManager::get_path_table )
        );

        using BaseItem = sdk::game::BaseItem;

        state.new_usertype< BaseItem >(
            "c_base_item",
            "get_item_data",
            sol::resolve( &BaseItem::get_item_data_raw ),
            "can_use",
            sol::readonly( &BaseItem::can_use ),
            "stacks_left",
            sol::readonly( &BaseItem::stacks_left ),
            "price",
            sol::readonly( &BaseItem::price ),
            "rageblade_damage",
            sol::readonly( &BaseItem::rageblade_damage )
        );

        using HudManager = sdk::game::HudManager;

        state.new_usertype< HudManager >(
            "c_hud_manager",
            "cursor_position_clipped",
            sol::readonly( &HudManager::cursor_position_clipped ),
            "cursor_position_unclipped",
            sol::readonly( &HudManager::cursor_position_unclipped ),
            "last_waypoint",
            sol::readonly( &HudManager::last_waypoint )
        );

        using Inventory = sdk::game::Inventory;

        state.new_usertype< Inventory >(
            "c_inventory",
            "get_inventory_slot",
            sol::resolve( &Inventory::get_inventory_slot )
        );

        using InventorySlot = sdk::game::InventorySlot;

        state.new_usertype< InventorySlot >(
            "c_inventory_slot",
            "get_base_item",
            sol::resolve( &InventorySlot::get_base_item_raw ),
            "update_time",
            sol::readonly( &InventorySlot::update_time ),
            "stacks",
            sol::readonly( &InventorySlot::stacks )
        );

        using ItemData = sdk::game::ItemData;

        state.new_usertype< ItemData >(
            "c_item_data",
            "id",
            sol::readonly( &ItemData::id ),
            "max_stacks",
            sol::readonly( &ItemData::max_stacks ),
            "is_consumable",
            sol::readonly( &ItemData::is_consumable )
        );

        using LeagueString = sdk::game::LeagueString;

        state.new_usertype< LeagueString >(
            "lstring",
            "text",
            sol::readonly( &LeagueString::text ),
            "size",
            sol::readonly( &LeagueString::size ),
            "is_valid",
            sol::resolve( &LeagueString::is_valid )
        );

        using MenuGui = sdk::game::MenuGui;

        state.new_usertype< MenuGui >(
            "c_menu_gui",
            "is_chat_open",
            sol::resolve( &MenuGui::is_chat_open ),
            "is_shop_open",
            sol::resolve( &MenuGui::is_shop_open ),
            "menu_state",
            sol::readonly( &MenuGui::menu_state )
        );

        using Navgrid = sdk::game::Navgrid;

        state.new_usertype< Navgrid >(
            "c_navgrid",
            "get_collision",
            sol::resolve( &Navgrid::get_collision ),
            "is_wall",
            sol::resolve( &Navgrid::is_wall ),
            "is_bush",
            sol::resolve( &Navgrid::is_bush ),
            "is_river",
            sol::resolve( &Navgrid::is_river ),
            "is_building",
            sol::resolve( &Navgrid::is_building ),
            "get_height",
            sol::resolve( &Navgrid::get_height ),
            "min_grid_pos",
            sol::readonly( &Navgrid::min_grid_pos ),
            "max_grid_pos",
            sol::readonly( &Navgrid::max_grid_pos ),
            "collision_map",
            sol::readonly( &Navgrid::collision_map ),
            "sampled_heights",
            sol::readonly( &Navgrid::sampled_heights ),
            "x_cell_count",
            sol::readonly( &Navgrid::x_cell_count ),
            "y_cell_count",
            sol::readonly( &Navgrid::y_cell_count ),
            "cell_size",
            sol::readonly( &Navgrid::cell_size )
        );

        using Object = sdk::game::Object;

        state.new_usertype< Object >(
            "c_object",
            "is_local",
            sol::resolve( &Object::is_local ),
            "compare_flags",
            sol::resolve( &Object::compare_flags ),
            "get_unit_info_component",
            sol::resolve( &Object::get_unit_info_component ),
            "get_hpbar_position",
            sol::resolve( &Object::get_hpbar_position ),
            "get_ai_manager",
            &Object::get_ai_manager_raw,
            "is_game_object",
            sol::resolve( &Object::is_game_object ),
            "is_neutral_camp",
            sol::resolve( &Object::is_neutral_camp ),
            "is_ai_base_common",
            sol::resolve( &Object::is_ai_base_common ),
            "is_attackable_unit",
            sol::resolve( &Object::is_attackable_unit ),
            "is_ai",
            sol::resolve( &Object::is_ai ),
            "is_minion",
            sol::resolve( &Object::is_minion ),
            "is_hero",
            sol::resolve( &Object::is_hero ),
            "is_turret",
            sol::resolve( &Object::is_turret ),
            "is_missile",
            sol::resolve( &Object::is_missile ),
            "is_building",
            sol::resolve( &Object::is_building ),
            "is_invisible",
            sol::resolve( &Object::is_invisible ),
            "is_visible",
            sol::resolve( &Object::is_visible ),
            "is_targetable",
            sol::resolve( &Object::is_targetable ),
            "is_alive",
            sol::resolve( &Object::is_alive ),
            "is_dead",
            sol::resolve( &Object::is_dead ),
            "is_recalling",
            sol::resolve( &Object::is_recalling ),
            "is_enemy",
            sol::resolve( &Object::is_enemy ),
            "is_zombie",
            sol::resolve( &Object::is_zombie ),
            "is_ally",
            sol::resolve( &Object::is_ally ),
            "dist_to_local",
            sol::resolve( &Object::dist_to_local ),
            "get_attack_damage",
            sol::resolve( &Object::attack_damage ),
            "get_bonus_attack_damage",
            sol::resolve( &Object::bonus_attack_damage ),
            "get_ability_power",
            sol::resolve( &Object::ability_power ),
            "get_missile_spell_info",
            sol::resolve( &Object::missile_spell_info ),
            "get_minion_type",
            sol::resolve( &Object::get_minion_type ),
            "is_senna_minion",
            sol::resolve( &Object::is_senna_minion ),
            "is_misc_minion",
            sol::resolve( &Object::is_misc_minion ),
            "is_plant",
            sol::resolve( &Object::is_plant ),
            "is_zyra_plant",
            sol::resolve( &Object::is_zyra_plant ),
            "is_barrel",
            sol::resolve( &Object::is_barrel ),
            "is_feather",
            sol::resolve( &Object::is_feather ),
            "is_jarvan_flag",
            sol::resolve( &Object::is_jarvan_flag ),
            "is_sand_soldier",
            sol::resolve( &Object::is_sand_soldier ),
            "is_tentacle",
            sol::resolve( &Object::is_tentacle ),
            "is_windwall",
            sol::resolve( &Object::is_windwall ),
            "is_spore",
            sol::resolve( &Object::is_spore ),
            "is_minion_only_autoattackable",
            sol::resolve( &Object::is_minion_only_autoattackable ),
            "has_special_minion_health",
            sol::resolve( &Object::has_special_minion_health ),
            "is_turret_object",
            sol::resolve( &Object::is_turret_object ),
            "is_turret_attackable",
            sol::resolve( &Object::is_turret_attackable ),
            "is_ward",
            sol::resolve( &Object::is_ward ),
            "is_untargetable_minion",
            sol::resolve( &Object::is_untargetable_minion ),
            "is_lane_minion",
            sol::resolve( &Object::is_lane_minion ),
            "is_jungle_monster",
            sol::resolve( &Object::is_jungle_monster ),
            "is_major_monster",
            sol::resolve( &Object::is_major_monster ),
            "is_normal_minion",
            sol::resolve( &Object::is_normal_minion ),
            "is_main_camp_monster",
            sol::resolve( &Object::is_main_camp_monster ),
            "get_monster_priority",
            sol::resolve( &Object::get_monster_priority ),
            "get_bounding_radius",
            sol::resolve( &Object::get_bounding_radius ),
            "get_object_name",
            sol::resolve( &Object::get_name ),
            "get_alternative_name",
            sol::resolve( &Object::get_alternative_name ),
            "get_particle_direction",
            sol::resolve( &Object::get_particle_direction ),
            "get_particle_source_index",
            sol::resolve( &Object::get_particle_source_index ),
            "get_particle_target_index",
            sol::resolve( &Object::get_particle_target_index ),
            "get_kills",
            sol::resolve( &Object::get_kills ),
            "get_ward_type",
            sol::resolve( &Object::get_ward_type ),
            "get_history_position",
            sol::resolve( &Object::get_history_position ),
            "get_history_position_time",
            sol::resolve( &Object::get_history_position_time ),
            "get_experience",
            sol::resolve( &Object::get_experience ),
            "get_lethality",
            sol::resolve( &Object::get_lethality ),
            "get_base_ms",
            sol::resolve( &Object::get_base_ms ),
            "is_invulnerable",
            sol::resolve( &Object::is_invulnerable ),
            "is_bad_minion",
            sol::resolve( &Object::is_bad_minion ),
            "get_owner_index",
            sol::resolve( &Object::get_owner_index ),
            "get_vision_score",
            sol::resolve( &Object::get_vision_score ),
            "get_random_position",
            sol::resolve( &Object::get_random_position ),
            "get_missile_spawn_time",
            sol::resolve( &Object::missile_spawn_time ),
            "get_missile_target_index",
            sol::resolve( &Object::get_missile_target_index ),
            "get_entity_info",
            sol::resolve( &Object::get_entity_info ),
            "index",
            sol::readonly( &Object::index ),
            // "next_object_index",
            // sol::readonly( &Object::next_object_index ),
            "team",
            sol::readonly( &Object::team ),
            "name",
            sol::property( &Object::get_player_name ),
            "network_id",
            sol::readonly( &Object::network_id ),
            "is_invalid_object",
            sol::resolve( &Object::is_invalid_object ),
            // "is_dead_flag",
            // sol::readonly( &Object::is_dead_flag ),
            "position",
            sol::readonly( &Object::position ),
            // "is_alive_flag",
            // sol::readonly( &Object::is_alive_flag ),
            // "invisible_flag",
            // sol::readonly( &Object::invisible_flag ),
            "targetable_flag",
            sol::readonly( &Object::targetable_flag ),
            "selectable_flag",
            sol::readonly( &Object::selectable_flag ),
            "mana",
            sol::readonly( &Object::mana ),
            "max_mana",
            sol::readonly( &Object::max_mana ),
            // "mana_enabled",
            // sol::readonly( &Object::mana_enabled ),
            "targetable_flag",
            sol::readonly( &Object::targetable_flag ),
            "selectable_flag",
            sol::readonly( &Object::selectable_flag ),
            "recalling_flag",
            sol::readonly( &Object::recalling_flag ),
            "health",
            sol::readonly( &Object::health ),
            "max_health",
            sol::readonly( &Object::max_health ),
            "shield",
            sol::readonly( &Object::shield ),
            "physical_shield",
            sol::readonly( &Object::physical_shield ),
            "magic_shield",
            sol::readonly( &Object::magic_shield ),
            "lethality",
            sol::readonly( &Object::lethality ),
            "bonus_attack",
            sol::readonly( &Object::bonus_attack ),
            "base_ap",
            sol::readonly( &Object::base_ap ),
            "modifier",
            sol::readonly( &Object::modifier ),
            "bonus_attack_speed",
            sol::readonly( &Object::bonus_attack_speed ),
            "life_steal",
            sol::readonly( &Object::life_steal ),
            "attack_speed",
            sol::readonly( &Object::attack_speed ),
            "base_attack",
            sol::readonly( &Object::base_attack ),
            "crit_chance",
            sol::readonly( &Object::crit_chance ),
            "total_armor",
            sol::readonly( &Object::total_armor ),
            "bonus_armor",
            sol::readonly( &Object::bonus_armor ),
            "total_mr",
            sol::readonly( &Object::total_mr ),
            "bonus_mr",
            sol::readonly( &Object::bonus_mr ),
            "base_health_regen",
            sol::readonly( &Object::base_health_regen ),
            "total_health_regen",
            sol::readonly( &Object::total_health_regen ),
            "movement_speed",
            sol::readonly( &Object::movement_speed ),
            "attack_range",
            sol::readonly( &Object::attack_range ),
            "armor_penetration_percent",
            sol::readonly( &Object::armor_penetration_percent ),
            "flat_magic_penetration",
            sol::readonly( &Object::flat_magic_penetration ),
            "magic_penetration_percent",
            sol::readonly( &Object::magic_penetration_percent ),
            "gold",
            sol::readonly( &Object::current_gold ),
            "total_gold",
            sol::readonly( &Object::total_gold ),
            // "min_gold",
            // sol::readonly( &Object::min_gold ),
            "max_gold",
            sol::readonly( &Object::max_gold ),
            "direction",
            sol::readonly( &Object::direction ),
            "champion_name",
            sol::readonly( &Object::champion_name ),
            "level",
            sol::readonly( &Object::level ),
            "inventory",
            sol::readonly( &Object::inventory ),
            "get_spell_book",
            sol::resolve( &Object::get_spell_book ),
            "get_buff_manager",
            sol::resolve( &Object::get_buff_manager )
        );

        using SpellBook = sdk::game::SpellBook;

        state.new_usertype< SpellBook >(
            "c_spell_book",
            "get_spell_slot",
            sol::resolve( &SpellBook::get_spell_slot_raw ),
            "get_spell_cast_info",
            sol::resolve( &SpellBook::get_spell_cast_info_raw )
        );

        state.new_usertype< LuaSpellCastInfo >(
            "c_spell_cast_info",
            "get_target_index",
            sol::resolve( &LuaSpellCastInfo::get_target_index ),
            "missile_index",
            sol::property( &LuaSpellCastInfo::get_missile_index ),
            "missile_nid",
            sol::property( &LuaSpellCastInfo::get_missile_nid ),
            "start_position",
            sol::property( &LuaSpellCastInfo::get_start_position ),
            "end_position",
            sol::property( &LuaSpellCastInfo::get_end_position ),
            "windup_time",
            sol::property( &LuaSpellCastInfo::get_windup_time ),
            "total_cast_time",
            sol::property( &LuaSpellCastInfo::get_total_cast_time ),
            "is_autoattack",
            sol::property( &LuaSpellCastInfo::is_autoattack ),
            "is_special_attack",
            sol::property( &LuaSpellCastInfo::is_special_attack ),
            "was_autoattack_cast",
            sol::property( &LuaSpellCastInfo::was_autoattack_cast ),
            "was_autoattack_cast2",
            sol::property( &LuaSpellCastInfo::was_autoattack_cast2 ),
            "slot",
            sol::property( &LuaSpellCastInfo::get_slot ),
            "server_cast_time",
            sol::property( &LuaSpellCastInfo::get_server_cast_time ),
            "end_time",
            sol::property( &LuaSpellCastInfo::get_end_time ),
            "start_time",
            sol::property( &LuaSpellCastInfo::get_start_time ),
            "owner_character",
            sol::property( &LuaSpellCastInfo::get_owner_character ),
            "coefficient",
            sol::property( &LuaSpellCastInfo::get_coefficient ),
            "coefficient2",
            sol::property( &LuaSpellCastInfo::get_coefficient2 ),
            "cooldown_time",
            sol::property( &LuaSpellCastInfo::get_cooldown_time ),
            "name",
            sol::property( &LuaSpellCastInfo::get_name ),
            "mana_cost",
            sol::property( &LuaSpellCastInfo::get_mana_cost ),
            "missile_speed",
            sol::property( &LuaSpellCastInfo::get_missile_speed ),
            "channel_duration",
            sol::property( &LuaSpellCastInfo::get_channel_duration ),
            "missile_width",
            sol::property( &LuaSpellCastInfo::get_missile_width )
        );

        using SpellDetails = sdk::game::SpellDetails;

        state.new_usertype< SpellDetails >(
            "c_spell_details",
            "last_start_position",
            sol::readonly( &SpellDetails::last_start_position ),
            "last_end_position",
            sol::readonly( &SpellDetails::last_end_position )
        );

        using SpellInfo = sdk::game::SpellInfo;

        state.new_usertype< SpellInfo >(
            "c_spell_info",
            "get_spell_data",
            sol::resolve( &SpellInfo::get_spell_data_raw )
        );

        using SpellSlot = sdk::game::SpellSlot;

        state.new_usertype< LuaSpellSlot >(
            "c_spell_slot",
            "level",
            sol::property( &LuaSpellSlot::get_level ),
            "cooldown_expire",
            sol::property( &LuaSpellSlot::get_cooldown_expire ),
            "charges",
            sol::property( &LuaSpellSlot::get_charges ),
            "final_cooldown_expire",
            sol::property( &LuaSpellSlot::get_final_cooldown_expire ),
            "cooldown",
            sol::property( &LuaSpellSlot::get_cooldown ),
            "coefficient",
            sol::property( &LuaSpellSlot::get_coefficient ),
            "coefficient2",
            sol::property( &LuaSpellSlot::get_coefficient2 ),
            "cooldown_time",
            sol::property( &LuaSpellSlot::get_cooldown_time ),
            "get_name",
            sol::resolve( &LuaSpellSlot::get_name ),
            "get_mana_cost",
            sol::resolve( &LuaSpellSlot::get_mana_cost ),
            "missile_speed",
            sol::property( &LuaSpellSlot::get_missile_speed ),
            "channel_duration",
            sol::property( &LuaSpellSlot::get_channel_duration ),
            "missile_width",
            sol::property( &LuaSpellSlot::get_missile_width ),
            "owner_character",
            sol::property( &LuaSpellSlot::get_owner_character ),
            "is_ready",
            sol::resolve( &LuaSpellSlot::is_ready )
        );

        using TacticalMap = sdk::game::TacticalMap;

        state.new_usertype< TacticalMap >(
            "c_tactical_map",
            "world_scale",
            sol::readonly( &TacticalMap::world_scale ),
            "top_left_clipping_inverse",
            sol::readonly( &TacticalMap::top_left_clipping_inverse ),
            "top_left_clipping",
            sol::readonly( &TacticalMap::top_left_clipping ),
            "map_area",
            sol::readonly( &TacticalMap::map_area )
        );

        using UnitInfoComponent = sdk::game::UnitInfoComponent;

        state.new_usertype< UnitInfoComponent >(
            "c_unit_info_component",
            "get_health_bar_height",
            sol::resolve( &UnitInfoComponent::get_health_bar_height ),
            "get_champion_id",
            sol::resolve( &UnitInfoComponent::get_champion_id ),
            "get_base_ms",
            sol::resolve( &UnitInfoComponent::get_base_ms )
        );

        state.new_usertype< Color >(
            "color",
            sol::constructors<
                Color( ),
                Color( int, int, int ),
                Color( int, int, int, int ),
                Color( float, float, float ),
                Color( float, float, float, float ),
                Color( const std::string& )
            >( ),
            "r",
            &Color::r,
            "g",
            &Color::g,
            "b",
            &Color::b,
            "a",
            &Color::a,
            "white",
            sol::resolve( &Color::white ),
            "black",
            sol::resolve( &Color::black ),
            "red",
            sol::resolve< Color( ) >( &Color::red ),
            "green",
            sol::resolve< Color( ) >( &Color::green ),
            "blue",
            sol::resolve< Color( ) >( &Color::blue ),
            "to_hex",
            sol::resolve( &Color::to_hex )
        );

        state.new_usertype< LuaBuff >(
            "c_buff",
            "active",
            sol::property( &LuaBuff::is_active ),
            "hard_cc",
            sol::property( &LuaBuff::is_hard_cc ),
            "disabling",
            sol::property( &LuaBuff::is_disabling ),
            "knock_up",
            sol::property( &LuaBuff::is_knock_up ),
            "silence",
            sol::property( &LuaBuff::is_silence ),
            "cripple",
            sol::property( &LuaBuff::is_cripple ),
            "invincible",
            sol::property( &LuaBuff::is_invincible ),
            "slow",
            sol::property( &LuaBuff::is_slow ),
            "type",
            sol::property( &LuaBuff::get_type ),
            "start_time",
            sol::property( &LuaBuff::get_start_time ),
            "end_time",
            sol::property( &LuaBuff::get_end_time ),
            "alt_amount",
            sol::property( &LuaBuff::get_alt_amount ),
            "amount",
            sol::property( &LuaBuff::get_amount ),
            "max_stack",
            sol::property( &LuaBuff::get_max_stack ),
            "name",
            sol::property( &LuaBuff::get_name ),
            "get_amount",
            sol::resolve( &LuaBuff::get_get_amount )
        );

        using BuffManager = sdk::game::BuffManager;

        state.new_usertype< BuffManager >(
            "c_buff_manager",
            "get_all",
            sol::resolve( &BuffManager::get_all_raw ),
            "size",
            sol::resolve( &BuffManager::size )
        );
    }
}
