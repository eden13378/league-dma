#pragma once
#include "../feature.hpp"
#include "ITargetSelector.hpp"
#include "../names.hpp"

namespace sdk::game {
    class Object;
}

namespace features {
    class TargetSelector final : public ITargetSelector {
    public:
        auto run( ) -> void override;

        auto get_orbwalker_target_raw( ) const -> sdk::game::Object*{
            return m_is_target_forced ? m_forced_target->get( ) : m_default_target->get( );
        }

        auto get_default_target( bool secondary = false ) -> sdk::game::Object* override;
        auto get_orbwalker_default_target( bool secondary = false ) -> sdk::game::Object* override;
        auto get_forced_target( ) -> sdk::game::Object* override;
        auto set_forced_target( int16_t index ) -> bool override;
        auto set_forced_target( sdk::game::Object* object ) -> bool override;
        auto set_forced_target( CHolder* holder ) -> bool override;
        auto reset_forced_target( ) -> void override;

        // auto get_forced_target_lua( ) -> sol::object;

        auto get_name( ) noexcept -> hash_t override{ return names::target_selector; }
#if _DEBUG
        auto get_full_name( ) -> std::string override{ return "c_target_selector"; }
#endif

        auto is_forced( ) const -> bool override{ return m_is_target_forced; }

        auto is_bad_target(
            int16_t index,
            bool    ignore_dead      = false,
            bool    ignore_invisible = false
        ) -> bool override;

    private:

    public:
        auto get_spell_specific_target(
            float range,
            std::function< float( sdk::game::Object* unit ) > get_travel_time = { },
            std::function< float( sdk::game::Object* unit ) > get_damage = { },
            int damage_type = 0 // 0 = true damage, 1 = physical, 2 = magic
        ) -> sdk::game::Object* override;

        auto get_killsteal_target(
            float                                             range,
            std::function< float( sdk::game::Object* unit ) > get_travel_time,
            std::function< float( sdk::game::Object* unit ) > get_damage,
            int                                               damage_type     = 0,
            sdk::math::Vec3                                   source_position = { }
        ) -> sdk::game::Object* override;

        auto get_target_priority( std::string champion_name ) -> int32_t override;

    private:
        auto update_orbwalker_target( ) -> void;
        auto update_forced_target( ) -> void;

        auto old_target_selection_mode( ) -> void;
        auto select_target( bool secondary = false ) -> void;
        auto select_target_new( bool secondary = false ) -> void;
        auto nenny_target_selection_mode( bool secondary = false ) -> void;

        CHolder* m_default_target{ };
        CHolder* m_secondary_target{ };

        CHolder* m_forced_target{ };

        bool  m_is_target_forced{ };
        float m_last_force_time{ };

        std::optional< CHolder* > m_override_target{ };

        std::vector< std::string > m_low_priority_targets = std::vector< std::string >
        {
            "Alistar",
            "Amumu",
            "Bard",
            "Blitzcrank",
            "Braum",
            "Cho'Gath",
            "Dr. Mundo",
            "Garen",
            "Gnar",
            "Hecarim",
            "Illaoi",
            "Janna",
            "Jarvan IV",
            "Leona",
            "Lulu",
            "Malphite",
            "Nami",
            "Nasus",
            "Nautilus",
            "Nunu",
            "Olaf",
            "Ornn",
            "Rammus",
            "Renekton",
            "Rell",
            "Sejuani",
            "Shen",
            "Shyvana",
            "Singed",
            "Sion",
            "Skarner",
            "Sona",
            "Taric",
            "TahmKench",
            "Thresh",
            "Volibear",
            "Warwick",
            "MonkeyKing",
            "Yorick",
            "Yuumi",
            "Zac"
        };

        std::vector< std::string > m_medium_priority_targets = std::vector< std::string >
        {
            "Aatrox",
            "Camille",
            "Darius",
            "Elise",
            "Evelynn",
            "Galio",
            "Gangplank",
            "Gragas",
            "Irelia",
            "Ivern",
            "Jax",
            "Kled",
            "KSante",
            "Lee Sin",
            "Lillia",
            "Maokai",
            "Morgana",
            "Nocturne",
            "Pantheon",
            "Poppy",
            "Rakan",
            "Rengar",
            "Rumble",
            "Ryze",
            "Sett",
            "Swain",
            "Sylas",
            "Trundle",
            "Tryndamere",
            "Udyr",
            "Urgot",
            "Vi",
            "XinZhao",
            "RekSai"
        };

        std::vector< std::string > m_high_priority_targets = std::vector< std::string >
        {
            "Akali",
            "Diana",
            "Ekko",
            "Fiddlesticks",
            "Fiora",
            "Fizz",
            "Gwen",
            "Heimerdinger",
            "Jayce",
            "Karma",
            "Kassadin",
            "Kayn",
            "Kayle",
            "Kha'Zix",
            "Lissandra",
            "Milio",
            "Mordekaiser",
            "Neeko",
            "Nidalee",
            "Pyke",
            "Qiyana",
            "Renata",
            "Riven",
            "Senna",
            "Shaco",
            "Taliyah",
            "Viego",
            "Vladimir",
            "Yasuo",
            "Zilean",
            "Zyra"
        };

        std::vector< std::string > m_highest_priority_targets = std::vector< std::string >
        {
            "Ahri",
            "Akshan",
            "Anivia",
            "Annie",
            "Aphelios",
            "Ashe",
            "Azir",
            "Brand",
            "Belveth",
            "Caitlyn",
            "Cassiopeia",
            "Corki",
            "Draven",
            "Ezreal",
            "Graves",
            "Jinx",
            "Kaisa",
            "Kalista",
            "Karthus",
            "Katarina",
            "Kennen",
            "KogMaw",
            "Kindred",
            "Leblanc",
            "Lucian",
            "Lux",
            "Malzahar",
            "MasterYi",
            "MissFortune",
            "Nilah",
            "Orianna",
            "Quinn",
            "Samira",
            "Sivir",
            "Syndra",
            "Talon",
            "Teemo",
            "Tristana",
            "TwistedFate",
            "Twitch",
            "Varus",
            "Vayne",
            "Veigar",
            "Velkoz",
            "Vex",
            "Viktor",
            "Xayah",
            "Xerath",
            "Yone",
            "Zed",
            "Ziggs",
            "Zoe",
            "Jhin",
            "Soraka",
            "Zeri"
        };
    };
}
