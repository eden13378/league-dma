#pragma once
#include <cstdint>
#include <ostream>

// #include "ai_manager.hpp"
#include "buff_manager.hpp"
#include "league_string.hpp"
#include "spell_book.hpp"
#include "inventory.hpp"
// #include "entity_info.hpp"
#include "obfuscated_flag.hpp"
// #include "unit_info_component.hpp"
#include "entity_info.hpp"
// #include "../../utils/holder.hpp"
#include "../math/vec3.hpp"
#include "../math/vec2.hpp"

class CHolder;

namespace sdk::game {
    class SpellInfo;
    class AiManager;
    class UnitInfoComponent;

    class Object {
    public:
        Object( ) = default;

        enum class EMinionType {
            error = 0,
            turret,
            jungle,
            ranged,
            melee,
            siege,
            super,
            plant,
            misc
        };

        friend auto operator==( const Object& lhs, const Object& rhs ) -> bool{
            return lhs.network_id == rhs.network_id;
        }

        friend auto operator!=( const Object& lhs, const Object& rhs ) -> bool{
            return !( lhs == rhs );
        }

        friend auto operator<<( std::ostream& os, const Object& obj ) -> std::ostream&{
            return os << "c_object( "
                << "index: " << obj.index
                << ", name: " << obj.name
                << ", network_id: " << obj.network_id
                << ", champion_name: " << obj.champion_name << " )";
        }

        enum class EObjectTypeFlags {
            game_object = 1 << 0,
            neutral_camp = 1 << 1,
            dead_object = 1 << 4,
            invalid_object = 1 << 5,
            ai_base_common = 1 << 7,
            attackable_unit = 1 << 9,
            ai = 1 << 10,
            minion = 1 << 11,
            hero = 1 << 12,
            turret = 1 << 13,
            unknown0 = 1 << 14,
            missile = 1 << 15,
            unknown1 = 1 << 16,
            building = 1 << 17,
            unknown2 = 1 << 18,
        };

        enum class EWardType {
            unknown = 0,
            normal,
            control,
            blue,
            zombie,
            teemo_shroom,
            shaco_box,
            jhin_trap,
            nidalee_trap,
            maokai_sproutling,
            fiddlesticks_effigy,
            caitlyn_trap
        };

    public:
        auto is_local( ) const -> bool;

        auto get_raw_flags( ) const -> std::optional< ObfuscatedFlag >;

        auto get_flags( ) const -> unsigned long;

        auto compare_flags( EObjectTypeFlags compare_flag ) const -> bool;

        auto get_unit_info_component( ) const -> utils::MemoryHolder< UnitInfoComponent >;
        auto get_hpbar_position( ) const -> sdk::math::Vec2;
        auto get_ai_manager( ) const -> utils::MemoryHolder< AiManager >;

        auto get_ai_manager_raw( ) const -> std::shared_ptr< AiManager >{
            auto ai_manager = get_ai_manager( );
            if ( !ai_manager ) return nullptr;

            return ai_manager.get( );
        }

        auto is_game_object( ) const -> bool{
            return compare_flags( EObjectTypeFlags::game_object );
        }

        auto is_dead_object( ) const -> bool{
            return compare_flags( EObjectTypeFlags::dead_object );
        }

        auto is_invalid_object( ) const -> bool{
            return compare_flags( EObjectTypeFlags::invalid_object );
        }

        auto is_neutral_camp( ) const -> bool{
            return compare_flags( EObjectTypeFlags::neutral_camp );
        }

        auto is_ai_base_common( ) const -> bool{
            return compare_flags( EObjectTypeFlags::ai_base_common );
        }

        auto is_attackable_unit( ) const -> bool{
            return compare_flags( EObjectTypeFlags::attackable_unit );
        }

        auto is_ai( ) const -> bool{
            return compare_flags( EObjectTypeFlags::ai );
        }

        auto is_minion( ) const -> bool{
            return compare_flags( EObjectTypeFlags::minion );
        }

        auto is_hero( ) const -> bool{
            return compare_flags( EObjectTypeFlags::hero );
        }

        auto is_turret( ) const -> bool{
            return compare_flags( EObjectTypeFlags::turret );
        }

        auto is_missile( ) const -> bool{
            return compare_flags( EObjectTypeFlags::missile );
        }

        auto is_building( ) const -> bool{
            return compare_flags( EObjectTypeFlags::building );
        }

        auto is_invisible( ) const -> bool{
            return ( invisible_flag % 2 == 0 );
        }

        auto is_visible( ) const -> bool{
            return !is_invisible( );
        }

        auto is_targetable( ) const -> bool{
            return get_targetable_flag( ) % 1 == 0;
        }

        auto is_alive( ) const -> bool;

        auto is_dead( ) const -> bool{
            return !is_alive( );
        }

        auto is_recalling( ) const -> bool{
            return recalling_flag > 0 && recalling_flag != 10;
        }

        auto is_enemy( ) const -> bool;
        auto is_ally( ) const -> bool;

        auto get_is_dead_flag( ) const -> int{
            return is_dead_flag;
        }

        auto is_zombie( ) const -> bool;

        auto dist_to_local( ) const -> float;

        auto attack_damage( ) const -> float;
        auto bonus_attack_damage( ) const -> float;

        auto ability_power( ) const -> float{
            return base_ap * ( 1 + modifier );
        }

        auto missile_spell_info( ) const -> utils::MemoryHolder< SpellInfo >;
        auto get_minion_type( ) const -> EMinionType;
        auto is_senna_minion( ) const -> bool;
        auto is_misc_minion( ) const -> bool;
        auto is_plant( ) const -> bool;
        auto is_zyra_plant( ) const -> bool;
        auto is_barrel( ) const -> bool;
        auto is_feather( ) const -> bool;
        auto is_jarvan_flag( ) const -> bool;
        auto is_sand_soldier( ) const -> bool;
        auto is_tentacle( ) const -> bool;
        auto is_windwall( ) const -> bool;
        auto is_spore( ) const -> bool;
        auto is_minion_only_autoattackable( ) const -> bool;
        auto has_special_minion_health( ) const -> bool;
        auto is_turret_object( ) const -> bool;
        auto $is_turret_object( ) const -> bool;
        auto is_turret_attackable( ) const -> bool;

        auto is_ward( ) const -> bool{
            return get_ward_type( ) > EWardType::unknown;
        }

        auto is_untargetable_minion( ) const -> bool{
            const auto hash = rt_hash( get_name().c_str() );

            return hash == ct_hash( "MaokaiSproutling" ) || hash == ct_hash( "CaitlynTrap" );
        }

        auto is_lane_minion( ) const -> bool;

        auto is_jungle_monster( ) const -> bool;

        auto is_major_monster( ) const -> bool;

        auto $is_normal_minion( ) const -> bool;
        auto is_normal_minion( ) const -> bool;

        auto is_main_camp_monster( ) const -> bool;

        auto get_monster_priority( ) const -> int32_t;

#if enable_lua
        auto lua_get_bounding_radius( ) const -> sol::object;
#endif

        auto get_bounding_radius( ) const -> float;

        auto get_name( ) const -> std::string;
        auto get_alternative_name( ) const -> std::string;

        auto get_particle_direction( ) const -> math::Vec3;
        auto get_particle_source_index( ) const -> int16_t;
        auto get_particle_target_index( ) const -> int16_t;
        auto get_particle_spawn_time( ) const -> float;
        auto get_particle_alt_spawn_time( ) const -> float;

        auto get_kills( ) const -> int;

        auto get_ward_type( ) const -> EWardType;

        auto get_history_position( int32_t history_tick = 1 ) const -> const math::Vec3&;
        auto get_history_position_time( int32_t history_tick = 1 ) const -> float;

        auto get_experience( ) const -> float;

        auto get_targetable_flag( ) const -> int{
            return this->targetable_flag;
        }

        auto get_selectable_flag( ) const -> int{
            return this->selectable_flag;
        }

        auto get_flat_magic_penetration( ) const -> float{
            return this->flat_magic_penetration;
        }

        auto get_magic_penetration_percent( ) const -> float{
            return this->magic_penetration_percent;
        }

        auto get_armor_penetration_percent( ) const -> float{
            return this->armor_penetration_percent;
        }

        auto get_lethality( ) const -> float{
            return this->lethality;
        }

        auto get_base_ms( ) const -> float;

        auto is_invulnerable( ) const -> bool;

        auto is_bad_minion( ) const -> bool;

        auto get_owner_index( ) const -> int16_t;

        auto get_vision_score( ) const -> float;

        //auto missile_target_index( ) const -> int16_t { return this->missile_target_index; }

        auto get_random_position( ) const -> const math::Vec3&{
            return *reinterpret_cast< math::Vec3* >( reinterpret_cast< uintptr_t >( this ) + 0x3798 );
        }

        auto missile_spawn_time( ) const -> float{
            return *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x430 );
        }

        auto get_current_gold( ) const -> float{
            return *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x2118 );
        }

        auto get_spellcast_state( ) const -> int32_t{
            return *reinterpret_cast< int8_t* >( reinterpret_cast< uintptr_t >( this ) + 0x36C0 );
        }

        auto get_spellcast_value( ) const -> int32_t{
            return *reinterpret_cast< int8_t* >( reinterpret_cast< uintptr_t >( this ) + 0x39A4 );
        }

        auto get_missile_target_index( ) const -> int16_t;

        auto get_model_size_modifier( ) const -> float{
            // TODO: FOR TORE TO UPDATE: PLAY NASUS AND ULT, ULT CHANGES MODEL SIZE TEMPORARILY, value is 1.0f and changes to bigger like 1.35f

            auto value = *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(this) + 0x20D8);

            return value < 0.75f ? 1.f : value;
        }

        auto get_direction( ) const -> math::Vec3{
            return { this->direction }; // TODO: update this offset
            // return math::Vec3(
            //     -this->direction_x_inverted,
            //     0.f,
            //     this->direction_z
            // );
        }

        auto get_cooldown_reduction_percent( ) -> float{
            auto ability_haste = *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( this ) + 0x1CF0 );
            // TODO: ALWAYS UPDATE THIS OFFSET ON NEW PATCH
            return ability_haste / ( ability_haste + 100.f );
        }

        //raw_damage * ( 100.f / ( 100.f + actual_mr ) )

        auto get_entity_info( ) const -> utils::MemoryHolder< EntityInfo >;

        auto get_holder( ) const -> CHolder&;

        auto get_spell_book( ) -> SpellBook&{
            return spell_book;
        }

        auto get_buff_manager( ) -> BuffManager&{
            return buff_manager;
        }

        auto get_player_name( ) const -> std::string{
            if ( !name.is_valid( ) ) return { };
            return name.text;
        }

    public:
char pad_0000[16]; //0x0000
	int16_t index; //0x0010
	int16_t next_object_index; //0x0012
	char pad_0014[40]; //0x0014
	int32_t team; //0x003C
	char pad_0040[32]; //0x0040
	class LeagueString name; //0x0060
	char pad_0078[80]; //0x0078
	uint32_t network_id; //0x00C8
	char pad_00CC[284]; //0x00CC
	bool maybe_is_visible_on_screen; //0x01E8
	char pad_01E9[1]; //0x01E9
	bool maybe_is_visible_on_screen2; //0x01EA
	char pad_01EB[9]; //0x01EB
	int32_t framecount; //0x01F4
	char pad_01F8[128]; //0x01F8
	int8_t is_dead_flag; //0x0278
	char pad_0279[7]; //0x0279
	math::Vec3 position; //0x0280
	char pad_028C[48]; //0x028C
	int32_t is_alive_flag; //0x02BC
	char pad_02C0[48]; //0x02C0
	class N00002D4B *missile_spell_info_p; //0x02F0
	char pad_02F8[80]; //0x02F8
	int32_t invisible_flag; //0x0348
	char pad_034C[44]; //0x034C
	float mana; //0x0378
	char pad_037C[20]; //0x037C
	float max_mana; //0x0390
	math::Vec3 missile_start_position; //0x0394
	math::Vec3 missile_end_position; //0x03A0
	char pad_03AC[24]; //0x03AC
	math::Vec3 old_missile_start_position; //0x03C4
	math::Vec3 old_missile_end_position; //0x03D0
	char pad_03DC[20]; //0x03DC
	uint16_t missile_target_index; //0x03F0
	char pad_03F2[90]; //0x03F2
	float _missile_spawn_time; //0x044C
	char pad_0450[544]; //0x0450
	uint32_t N000002F3; //0x0670
	char pad_0674[4]; //0x0674
	uint32_t targetable_flag; //0x0678
	char pad_067C[2460]; //0x067C
	uint32_t selectable_flag; //0x1018
	char pad_101C[204]; //0x101C
	int32_t recalling_flag; //0x10E8
	char pad_10EC[244]; //0x10EC
	float health; //0x11E0
	char pad_11E4[36]; //0x11E4
	float max_health; //0x1208
	char pad_120C[44]; //0x120C
	float shield; //0x1238
	char pad_123C[20]; //0x123C
	float physical_shield; //0x1250
	char pad_1254[20]; //0x1254
	float magic_shield; //0x1268
	char pad_126C[1508]; //0x126C
	float lethality; //0x1850
	char pad_1854[68]; //0x1854
	float bonus_attack; //0x1898
	char pad_189C[12]; //0x189C
	float base_ap; //0x18A8
	float modifier; //0x18AC
	char pad_18B0[76]; //0x18B0
	float bonus_attack_speed; //0x18FC
	char pad_1900[16]; //0x1900
	float life_steal; //0x1910
	char pad_1914[20]; //0x1914
	float attack_speed; //0x1928
	float base_attack; //0x192C
	float base_attack2; //0x1930
	char pad_1934[28]; //0x1934
	float crit_chance; //0x1950
	float total_armor; //0x1954
	float bonus_armor; //0x1958
	float total_mr; //0x195C
	float bonus_mr; //0x1960
	float base_health_regen; //0x1964
	float total_health_regen; //0x1968
	float movement_speed; //0x196C
	char pad_1970[4]; //0x1970
	float attack_range; //0x1974
	char pad_1978[1832]; //0x1978
	float armor_penetration_percent; //0x20A0
	char pad_20A4[92]; //0x20A4
	float flat_magic_penetration; //0x2100
	char pad_2104[44]; //0x2104
	float magic_penetration_percent; //0x2130
	char pad_2134[1828]; //0x2134
	float current_gold; //0x2858
	char pad_285C[36]; //0x285C
	float total_gold; //0x2880
	char pad_2884[36]; //0x2884
	float min_gold; //0x28A8
	char pad_28AC[36]; //0x28AC
	float max_gold; //0x28D0
	char pad_28D4[44]; //0x28D4
	math::Vec3 direction; //0x2900
	char pad_290C[1620]; //0x290C
	class BuffManager buff_manager; //0x2F60
	char pad_2F80[552]; //0x2F80
	class SpellBook spell_book; //0x31A8
	char *object_name; //0x41D0
	char pad_41D8[272]; //0x41D8
	uint64_t ai_manager; //0x42E8
	char pad_42F0[320]; //0x42F0
	class LeagueString champion_name; //0x4430
	char pad_4448[608]; //0x4448
	char *object_name2; //0x46A8
	char pad_46B0[1960]; //0x46B0
	int32_t level; //0x4E58
	char pad_4E5C[68]; //0x4E5C
	class Inventory inventory; //0x4EA0
	char pad_4FA8[480]; //0x4FA8

    };
}
