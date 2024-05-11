#pragma once

#include "../../sdk/sdk.hpp"
#include "../../security/src/hash_t.hpp"
#include "../buff_cache.hpp"
#include "../prediction.hpp"
#include "../../sdk/game/object.hpp"
#include "../../sdk/game/spell_cast_info.hpp"
#include "../../sdk/game/spell_data.hpp"
#include "../../security/src/xorstr.hpp"
#include "../../sdk/game/spell_info.hpp"

// #include "../lua-v2/custom_structs.hpp"

// namespace sdk::game {
//     class Object;
// }

namespace lua {
    class LuaSpellSlot;
}

namespace features::champion_modules {
    class IModule {
        using spell_callback_t = std::function< bool( /* pass spell object here */ ) >;

    public:
        enum ESpellFlag {
            none,
            q_spell = 1 << 0,
            w_spell = 1 << 1,
            e_spell = 1 << 2,
            r_spell = 1 << 3
        };

        enum ESpellSetting {
            lane_clear = 1 << 0,
            last_hit   = 1 << 1
        };

        struct LasthitInfo {
            int16_t         index{ };
            int16_t         index2{ };
            float           damage{ };
            float           travel_time{ };
            sdk::math::Vec3 cast_position{ };
        };

        struct TargetableLasthitInfo {
            int16_t                        index{ };
            uint32_t                       network_id{ };
            float                          travel_time{ };
            float                          damage{ };
            sdk::game::Object::EMinionType type{ };
        };

        struct CollidingSkillshot {
            float time_until_collision{ };
            int   danger{ };

            std::string name{ };
        };

        struct LaneclearInfo {
            int16_t target_index{ };
            float   damage{ };
            float   travel_time{ };

            bool will_execute{ };

            sdk::math::Vec3 cast_position{ };
        };

        struct DamageData {
            float   travel_time{ };
            float   damage{ };
            int16_t index{ };
        };

        struct FarmPosition {
            int32_t                   value{ };
            sdk::math::Vec3           position{ };
            std::vector< DamageData > target_data{ };
        };

        struct ViktorFarmPosition {
            sdk::math::Vec3 start_position{ };
            sdk::math::Vec3 end_position{ };

            int count{ };
        };

        struct SimpleFarmPosition {
            int32_t         value{ };
            sdk::math::Vec3 position{ };
        };

        struct CollisionPosition {
            int32_t         hit_count{ };
            sdk::math::Vec3 position{ };
        };

        struct MiscAction {
            std::shared_ptr< sdk::game::Object >   target;
            std::optional< sdk::game::ESpellSlot > slot{ };
        };

        enum class EDamageOvertimeForm {
            summoner,
            ability,
            item,
            neutral_buff
        };

        enum class EDamageOvertimeType {
            none = 0,
            red_buff_passive,
            ignite,
            liandrys_torment_burn,
            demonic_embrace_burn,
            brand_passive,
            cassiopeia_q,
            cassiopeia_w,
            darius_passive,
            fiddlesticks_w,
            fizz_w,
            gangplank_passive,
            lillia_passive,
            malzahar_e,
            maokai_e,
            nautilus_w,
            nidalee_w,
            rumble_r,
            talon_passive,
            teemo_e,
            teemo_r,
            trundle_r,
            twitch_passive
        };

        struct EDamageOvertimeInstance {
            EDamageOvertimeForm form;
            EDamageOvertimeType type;

            int   stacks{ };
            float end_time{ };

            std::string name{ };
        };

        virtual auto get_name( ) -> hash_t = 0;
        virtual auto get_champion_name( ) -> hash_t = 0;

        /**
         * \brief Loop through all spell slots sorted by priority list and call spell functions
         */
        virtual auto run( ) -> void;

        virtual auto on_draw( ) -> void{
        }

        virtual auto initialize( ) -> void{
        }

        virtual auto initialize_menu( ) -> void{
        }

        virtual auto on_all_champions_loaded( ) -> void{
        }

        virtual auto is_lua_module( ) -> bool{ return false; }

#if enable_lua
        static auto register_lua( sol::state* state ) -> void{
            state->new_enum(
                "e_spell_flag",
                "q_spell",
                q_spell,
                "w_spell",
                w_spell,
                "e_spell",
                e_spell,
                "r_spell",
                r_spell
            );

            state->new_usertype< IModule >(
                "c_module",
                "spell_q",
                &IModule::spell_q,
                "spell_w",
                &IModule::spell_w,
                "spell_e",
                &IModule::spell_e,
                "spell_r",
                &IModule::spell_r,
                "disable_q_on",
                &IModule::disable_q_on,
                "disable_w_on",
                &IModule::disable_w_on,
                "disable_e_on",
                &IModule::disable_e_on,
                "disable_r_on",
                &IModule::disable_r_on,
                "get_slot_q",
                &IModule::get_slot_q_lua,
                "get_slot_w",
                &IModule::get_slot_w_lua,
                "get_slot_e",
                &IModule::get_slot_e_lua,
                "get_slot_r",
                &IModule::get_slot_r_lua,
                "priority_list",
                &IModule::m_priority_list
            );
        }
#endif

    protected:
        virtual auto spell_q( ) -> bool{ return false; }

        virtual auto spell_w( ) -> bool{ return false; }

        virtual auto spell_e( ) -> bool{ return false; }

        virtual auto spell_r( ) -> bool{ return false; }

        auto initialize_spell_slots( ) -> void;

        virtual auto run_laneclear( ) -> std::optional< MiscAction >{ return std::nullopt; }

        virtual auto run_lasthit( ) -> std::optional< MiscAction >{ return std::nullopt; }

    public:
        auto disable_q_on( const spell_callback_t& fn ) -> spell_callback_t*{
            m_disable_q.push_back( fn );
            return &m_disable_q.back( );
        }

        auto disable_w_on( const spell_callback_t& fn ) -> spell_callback_t*{
            m_disable_w.push_back( fn );
            return &m_disable_w.back( );
        }

        auto disable_e_on( const spell_callback_t& fn ) -> spell_callback_t*{
            m_disable_e.push_back( fn );
            return &m_disable_e.back( );
        }

        auto disable_r_on( const spell_callback_t& fn ) -> spell_callback_t*{
            m_disable_r.push_back( fn );
            return &m_disable_r.back( );
        }

        auto get_priority_list( ) const -> const std::vector< ESpellFlag >&{ return m_priority_list; }

    protected:
        ~IModule( ) = default;

        static auto should_run( const std::vector< spell_callback_t >& checks ) -> bool;
        static auto cast_spell( sdk::game::ESpellSlot slot, const sdk::math::Vec3& position ) -> bool;
        static auto cast_spell(
            sdk::game::ESpellSlot  slot,
            const sdk::math::Vec3& start_position,
            const sdk::math::Vec3& end_position
        ) -> bool;
        static auto cast_spell( sdk::game::ESpellSlot slot ) -> bool;
        static auto cast_spell( sdk::game::ESpellSlot slot, unsigned nid ) -> bool;
        static auto release_chargeable(
            sdk::game::ESpellSlot  slot,
            const sdk::math::Vec3& position,
            bool                   release = true
        ) -> bool;

        static auto get_line_lasthit_target_advanced(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            const std::function< float( sdk::game::Object* unit ) >& get_traveltime,
            float                                                    range,
            float                                                    radius,
            float                                                    cast_delay,
            int                                                      collision_limit = 1,
            bool                                                     conserve_mana   = false
        )
            -> std::optional< LasthitInfo >;

        static auto get_line_lasthit_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            float                                                    range,
            float                                                    speed,
            float                                                    delay,
            float                                                    width,
            bool                                                     has_collision
        ) -> std::optional< LasthitInfo >;
        static auto get_line_laneclear_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            const std::function< float( sdk::game::Object* unit ) >& get_travel_time,
            float                                                    range,
            float                                                    width,
            bool                                                     has_collision,
            bool                                                     only_jungle_monsters = false,
            bool                                                     fast                 = false
        ) -> std::optional< LaneclearInfo >;

        static auto get_zeri_laneclear_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            const std::function< float( sdk::game::Object* unit ) >& get_traveltime,
            float                                                    range,
            float                                                    width,
            bool                                                     has_collision
        ) -> std::optional< LaneclearInfo >;
        static auto get_zeri_lasthit_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            const std::function< float( sdk::game::Object* unit ) >& get_traveltime,
            float                                                    range,
            float                                                    width,
            bool                                                     collision
        ) -> std::optional< LasthitInfo >;

        static auto get_targetable_lasthit_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            float                                                    range,
            const std::function< float( sdk::game::Object* unit ) >& get_travel_time = { }
        ) -> std::optional< TargetableLasthitInfo >;


        static auto get_circle_lasthit_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            float                                                    range,
            float                                                    speed,
            float                                                    delay,
            float                                                    width
        ) -> FarmPosition;
        static auto get_circle_laneclear_target(
            const std::function< float( sdk::game::Object* unit ) >& get_damage,
            float                                                    range,
            float                                                    speed,
            float                                                    delay,
            float                                                    width
        ) -> FarmPosition;
        static auto get_best_laneclear_position(
            float range,
            float width,
            bool  allow_lane_minion    = true,
            bool  allow_jungle_monster = true,
            float delay                = 0.f
        ) -> SimpleFarmPosition;

        static auto get_multihit_count(
            const sdk::math::Vec3& end_position,
            float                  range,
            float                  speed,
            float                  radius,
            float                  delay,
            bool                   linear
        ) -> int32_t;
        static auto get_multihit_position(
            float range,
            float speed,
            float radius,
            float delay,
            bool  linear
        ) -> CollisionPosition;

        static auto get_circle_multihit(
            const std::function< float( sdk::game::Object* unit ) >& get_travel_time,
            float                                                    range,
            float                                                    radius
        ) -> CollisionPosition;


        static auto get_viktor_multihit_position(
            const sdk::math::Vec3& source_position,
            float                  speed,
            float                  radius,
            unsigned               ignored_nid
        ) -> std::optional< CollisionPosition >;
        static auto get_viktor_spellclear_position( ) -> std::optional< ViktorFarmPosition >;

        static auto will_kill(
            sdk::game::Object*                                       target,
            const std::function< float( sdk::game::Object* unit ) >& get_damage
        ) -> bool;

        static auto get_antigapclose_position_weight(
            const sdk::math::Vec3& position,
            const sdk::math::Vec3& dash_end,
            bool                   passes_through_walls = true
        ) -> float;

    public:
        static auto get_antigapclose_target( float danger_distance = 500.f ) -> sdk::game::Object*;
        auto get_interruptable_target( float range, sdk::math::Vec3 source_position = { } ) const -> sdk::game::Object*;
        static auto get_advanced_antigapclose_target(
            float           range,
            float           speed,
            float           width,
            float           delay,
            bool            edge_range      = false,
            sdk::math::Vec3 source_position = { }
        ) -> sdk::game::Object*;

        static auto get_stasis_target(
            float                                                    range,
            const std::function< float( sdk::game::Object* unit ) >& get_traveltime,
            float                                                    hitbox_width    = 0.f,
            sdk::math::Vec3                                          source_position = { }
        ) -> sdk::game::Object*;

    protected:
        static auto should_spellshield_ally( sdk::game::Object* hero ) -> bool;
        static auto get_colliding_skillshot( int16_t index ) -> std::optional< CollidingSkillshot >;
        auto        get_damage_overtime_data(
            int16_t index,
            bool    include_abilities     = true,
            bool    include_summoners     = true,
            bool    include_items         = true,
            bool    include_neutral_buffs = true
        ) const -> std::optional< std::vector< EDamageOvertimeInstance > >;

        static auto is_position_in_turret_range(
            sdk::math::Vec3 position    = sdk::math::Vec3( 0, 0, 0 ),
            bool            ally_turret = false
        ) -> bool;

        [[nodiscard]] auto get_slot_q( ) const -> const utils::MemoryHolder< sdk::game::SpellSlot >&{ return m_slot_q; }

        [[nodiscard]] auto get_slot_w( ) const -> const utils::MemoryHolder< sdk::game::SpellSlot >&{ return m_slot_w; }

        [[nodiscard]] auto get_slot_e( ) const -> const utils::MemoryHolder< sdk::game::SpellSlot >&{ return m_slot_e; }

        [[nodiscard]] auto get_slot_r( ) const -> const utils::MemoryHolder< sdk::game::SpellSlot >&{ return m_slot_r; }

#if enable_lua
        [[nodiscard]] auto get_slot_q_lua( ) const -> lua::LuaSpellSlot;

        [[nodiscard]] auto get_slot_w_lua( ) const -> lua::LuaSpellSlot;

        [[nodiscard]] auto get_slot_e_lua( ) const -> lua::LuaSpellSlot;

        [[nodiscard]] auto get_slot_r_lua( ) const -> lua::LuaSpellSlot;
#endif

        virtual auto get_q_spell_settings( ) const -> int32_t{ return 0; }

        virtual auto get_w_spell_settings( ) const -> int32_t{ return 0; }

        virtual auto get_e_spell_settings( ) const -> int32_t{ return 0; }

        virtual auto get_r_spell_settings( ) const -> int32_t{ return 0; }

        virtual auto get_spell_damage( sdk::game::ESpellSlot slot, sdk::game::Object* target ) -> float{ return 0.f; }

        virtual auto get_spell_travel_time( sdk::game::ESpellSlot slot, sdk::game::Object* target ) -> float{
            return 0.f;
        }

    protected:
        std::vector< spell_callback_t > m_disable_q{ };
        std::vector< spell_callback_t > m_disable_w{ };
        std::vector< spell_callback_t > m_disable_e{ };
        std::vector< spell_callback_t > m_disable_r{ };
        /**
         * \brief Prioritised spells first, others last. Spells missing in this list won't be run. 
         */
        std::vector< ESpellFlag > m_priority_list{ q_spell, w_spell, e_spell, r_spell };
        CHolder*                  m_target{ };

        utils::MemoryHolder< sdk::game::SpellSlot > m_slot_q{ };
        utils::MemoryHolder< sdk::game::SpellSlot > m_slot_w{ };
        utils::MemoryHolder< sdk::game::SpellSlot > m_slot_e{ };
        utils::MemoryHolder< sdk::game::SpellSlot > m_slot_r{ };
        float                                       m_last_spell_update_time{ };

        std::vector< std::string > m_interruptable_list = {
            _( "XerathLocusOfPower2" ),
            _( "KatarinaR" ),
            _( "CaitlynR" ),
            _( "JhinR" ),
            _( "MissFortuneBulletTime" ),
            _( "FiddleSticksR" ),
            _( "KarthusFallenOne" ),
            _( "TeleportChannelBar4500" ),
            _( "PantheonR" ),
            _( "NunuR" ),
            _( "MalzaharR" ),
            _( "ShenR" ),
            _( "VelkozR" ),
            _( "TaliyahR" ),
            _( "QuinnR" ),
            _( "ReapTheWhirlwind" ),
            _( "WarwickRChannel" ),
            _( "SionQ" ),
            _( "FiddleSticksW" ),
            _( "TahmKenchW" ),
            _( "Meditate" ),
            _( "ZacE" ),
            _( "IreliaW" ),

            // spells using server_cast_time
            _( "SennaR" ),
            _( "EzrealR" ),
            _( "JinxR" )

        };
    };
}
