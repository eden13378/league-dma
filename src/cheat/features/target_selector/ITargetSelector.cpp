#include "pch.hpp"

#include "ITargetSelector.hpp"

#include "../entity_list.hpp"
#include "../../lua-v2/state.hpp"
#include "../champion_modules/module.hpp"

namespace features {
#if enable_lua
    // ReSharper disable once CppPassValueParameterByConstReference
    auto ITargetSelector::lua_force_target( sol::object target ) -> bool{
        if ( target.get_type( ) == sol::type::number ) {
            const auto index = target.as< int32_t >( );

            if ( index == -1 ) {
                reset_forced_target( );
                return true;
            }

            auto& ent = g_entity_list.get_by_index( index );
            if ( !ent ) return false;
            set_forced_target( &ent );
            return true;
        }

        if ( target.is< Object* >( ) ) {
            set_forced_target( CHolder::from_object( target.as< Object* >( ) ) );
            return true;
        }

        if ( target.get_type( ) == sol::type::nil || target.get_type( ) == sol::type::lua_nil ) {
            reset_forced_target( );
            return true;
        }

        return false;
    }
#endif

    auto ITargetSelector::get_champion_range( const hash_t champion_name ) -> float{
        switch ( champion_name ) {
        case ct_hash( "Aatrox" ):
            return 625.f;
        case ct_hash( "Ahri" ):
            return 880.f;
        case ct_hash( "Akali" ):
            return 825.f;
        case ct_hash( "Akshan" ):
            return 800.f;
        case ct_hash( "Alistar" ):
            return 700.f;
        case ct_hash( "Amumu" ):
            return 1100.f;
        case ct_hash( "Anivia" ):
            return 1075.f;
        case ct_hash( "Annie" ):
            return 625.f;
        case ct_hash( "Aphelios" ):
            return 650.f;
        case ct_hash( "Ashe" ):
            return 1200.f;
        case ct_hash( "AurelionSol" ):
            return 1500.f;
        case ct_hash( "Azir" ):
            return 740.f;
        case ct_hash( "Bard" ):
            return 950.f;
        case ct_hash( "BelVeth" ):
            return 660.f;
        case ct_hash( "Blitzcrank" ):
            return 1150.f;
        case ct_hash( "Brand" ):
            return 1050.f;
        case ct_hash( "Braum" ):
            return 1000.f;
        case ct_hash( "Caitlyn" ):
            return 1250.f;
        case ct_hash( "Camille" ):
            return 650.f;
        case ct_hash( "Cassiopeia" ):
            return 850.f;
        case ct_hash( "Chogath" ):
            return 950.f;
        case ct_hash( "Corki" ):
            return 1500.f;
        case ct_hash( "Darius" ):
            return 535.f;
        case ct_hash( "Diana" ):
            return 900.f;
        case ct_hash( "Draven" ):
            return 1050.f;
        case ct_hash( "DrMundo" ):
            return 975.f;
        case ct_hash( "Ekko" ):
            return 1175.f;
        case ct_hash( "Elise" ):
            return 850.f;
        case ct_hash( "Evelynn" ):
            return 800.f;
        case ct_hash( "Ezreal" ):
            return 1200.f;
        case ct_hash( "Fiddlesticks" ):
            return 850.f;
        case ct_hash( "Fiora" ):
            return 750.f;
        case ct_hash( "Fizz" ):
            return 1300.f;
        case ct_hash( "Galio" ):
            return 825.f;
        case ct_hash( "Gangplank" ):
            return 625.f;
        case ct_hash( "Garen" ):
            return 400.f;
        case ct_hash( "Gnar" ):
            return 1125.f;
        case ct_hash( "Gragas" ):
            return 850.f;
        case ct_hash( "Graves" ):
            return 800.f;
        case ct_hash( "Gwen" ):
            return 1350.f;
        case ct_hash( "Hecarim" ):
            return 1510.f;
        case ct_hash( "Heimerdinger" ):
            return 1325.f;
        case ct_hash( "Illaoi" ):
            return 850.f;
        case ct_hash( "Irelia" ):
            return 950.f;
        case ct_hash( "Ivern" ):
            return 1075.f;
        case ct_hash( "Janna" ):
            return 1750.f;
        case ct_hash( "JarvanIV" ):
            return 860.f;
        case ct_hash( "Jax" ):
            return 700.f;
        case ct_hash( "Jayce" ):
            return 1600.f;
        case ct_hash( "Jhin" ):
            return 1750.f;
        case ct_hash( "Jinx" ):
            return 1450.f;
        case ct_hash( "KaiSa" ):
            return 1750.f;
        case ct_hash( "Kalista" ):
            return 1150.f;
        case ct_hash( "Karma" ):
            return 950.f;
        case ct_hash( "Karthus" ):
            return 875.f;
        case ct_hash( "Kassadin" ):
            return 600.f;
        case ct_hash( "Katarina" ):
            return 7250.f;
        case ct_hash( "Kayle" ):
            return 900.f;
        case ct_hash( "Kayn" ):
            return 700.f;
        case ct_hash( "Kennen" ):
            return 1050.f;
        case ct_hash( "KhaZix" ):
            return 1000.f;
        case ct_hash( "Kindred" ):
            return 560.f;
        case ct_hash( "Kled" ):
            return 800.f;
        case ct_hash( "KogMaw" ):
            return 1800.f;
        case ct_hash( "KSante" ):
            return 465.f;
        case ct_hash( "LeBlanc" ):
            return 925.f;
        case ct_hash( "LeeSin" ):
            return 1100.f;
        case ct_hash( "Leona" ):
            return 775.f;
        case ct_hash( "Lillia" ):
            return 1300.f;
        case ct_hash( "Lissandra" ):
            return 1025.f;
        case ct_hash( "Lucian" ):
            return 900.f;
        case ct_hash( "Lulu" ):
            return 925.f;
        case ct_hash( "Lux" ):
            return 1175.f;
        case ct_hash( "Malphite" ):
            return 1000.f;
        case ct_hash( "Malzahar" ):
            return 900.f;
        case ct_hash( "Maokai" ):
            return 600.f;
        case ct_hash( "MasterYi" ):
            return 600.f;
        case ct_hash( "Milio" ):
            return 1000.f;
        case ct_hash( "MissFortune" ):
            return 1000.f;
        case ct_hash( "Mordekaiser" ):
            return 900.f;
        case ct_hash( "Morgana" ):
            return 1250.f;
        case ct_hash( "Nami" ):
            return 875.f;
        case ct_hash( "Nasus" ):
            return 700.f;
        case ct_hash( "Nautilus" ):
            return 925.f;
        case ct_hash( "Neeko" ):
            return 1000.f;
        case ct_hash( "Nidalee" ):
            return 1500.f;
        case ct_hash( "Nilah" ):
            return 600.f;
        case ct_hash( "Nocturne" ):
            return 1200.f;
        case ct_hash( "Nunu" ):
            return 625.f;
        case ct_hash( "Olaf" ):
            return 1000.f;
        case ct_hash( "Orianna" ): // Probably change to Ball return range
            return 825.f;
        case ct_hash( "Ornn" ):
            return 800.f;
        case ct_hash( "Phanteon" ):
            return 1200.f;
        case ct_hash( "Poppy" ):
            return 430.f;
        case ct_hash( "Pyke" ):
            return 1100.f;
        case ct_hash( "Qiyana" ):
            return 925.f;
        case ct_hash( "Quinn" ):
            return 1025.f;
        case ct_hash( "Rakan" ):
            return 850.f;
        case ct_hash( "Rammus" ):
            return 1100.f;
        case ct_hash( "RekSai" ):
            return 1625.f;
        case ct_hash( "Rell" ):
            return 700.f;
        case ct_hash( "RenataGlasc" ):
            return 900.f;
        case ct_hash( "Renekton" ):
            return 480.f;
        case ct_hash( "Rengar" ):
            return 1000.f;
        case ct_hash( "Riven" ):
            return 1100.f;
        case ct_hash( "Rumble" ):
            return 825.f;
        case ct_hash( "Ryze" ):
            return 1000.f;
        case ct_hash( "Samira" ):
            return 950.f;
        case ct_hash( "Sejuani" ):
            return 650.f;
        case ct_hash( "Senna" ):
            return 1300.f;
        case ct_hash( "Seraphine" ):
            return 900.f;
        case ct_hash( "Sett" ):
            return 790.f;
        case ct_hash( "Shaco" ):
            return 625.f;
        case ct_hash( "Shen" ):
            return 600.f;
        case ct_hash( "Shyvana" ):
            return 925.f;
        case ct_hash( "Singed" ):
            return 1000.f;
        case ct_hash( "Sion" ):
            return 750.f;
        case ct_hash( "Sivir" ):
            return 1250.f;
        case ct_hash( "Skarner" ):
            return 1000.f;
        case ct_hash( "Sona" ):
            return 825.f;
        case ct_hash( "Soraka" ):
            return 810.f;
        case ct_hash( "Swain" ):
            return 850.f;
        case ct_hash( "Sylas" ):
            return 850.f;
        case ct_hash( "Syndra" ):
            return 800.f;
        case ct_hash( "Tahmkench" ):
            return 900.f;
        case ct_hash( "Taliyah" ):
            return 1000.f;
        case ct_hash( "Talon" ):
            return 650.f;
        case ct_hash( "Taric" ):
            return 610.f;
        case ct_hash( "Teemo" ):
            return 680.f;
        case ct_hash( "Thresh" ):
            return 1100.f;
        case ct_hash( "Tristana" ):
            return 900.f;
        case ct_hash( "Trundle" ):
            return 650.f;
        case ct_hash( "Tryndamere" ):
            return 660.f;
        case ct_hash( "TwistedFate" ):
            return 1450.f;
        case ct_hash( "Twitch" ):
            return 950.f;
        case ct_hash( "Udyr" ):
            return 800.f;
        case ct_hash( "Urgot" ):
            return 800.f;
        case ct_hash( "Varus" ):
            return 1525.f;
        case ct_hash( "Vayne" ):
            return 550.f;
        case ct_hash( "Veigar" ):
            return 900.f;
        case ct_hash( "VelKoz" ):
            return 1550.f;
        case ct_hash( "Vex" ): // To test in actual game
            return 3000.f;
        case ct_hash( "Vi" ):
            return 725.f;
        case ct_hash( "Viego" ):
            return 900.f;
        case ct_hash( "Viktor" ):
            return 1150.f;
        case ct_hash( "Vladimir" ):
            return 600.f;
        case ct_hash( "Volibear" ):
            return 1200.f;
        case ct_hash( "Warwick" ):
            return 350.f;
        case ct_hash( "MonkeyKing" ):
            return 650.f;
        case ct_hash( "Xayah" ):
            return 1100.f;
        case ct_hash( "Xerath" ):
            return 1400.f;
        case ct_hash( "XinZhao" ):
            return 900.f;
        case ct_hash( "Yasuo" ):
            return 1000.f;
        case ct_hash( "Yone" ):
            return 1000.f;
        case ct_hash( "Yorick" ):
            return 700.f;
        case ct_hash( "Yuumi" ):
            return 1100.f;
        case ct_hash( "Zac" ):
            return 800.f;
        case ct_hash( "Zed" ):
            return 900.f;
        case ct_hash( "Zeri" ):
            return 1500.f;
        case ct_hash( "Ziggs" ):
            return 850.f;
        case ct_hash( "Zilean" ):
            return 900.f;
        case ct_hash( "Zoe" ):
            return 1600.f;
        case ct_hash( "Zyra" ):
            return 1100.f;
        default:
            return 0.f;
        }
    }

#if enable_lua
    auto ITargetSelector::lua_is_forced( ) -> bool{ return !!get_forced_target( ); }

    // ReSharper disable once CppPassValueParameterByConstReference
    auto ITargetSelector::lua_get_antigapclose_target( sol::object danger_distance ) -> sol::object{
        lua_arg_check_ct( danger_distance, float, "number" )

        const auto target = champion_modules::IModule::get_antigapclose_target( danger_distance.as< float >( ) );

        if ( !target ) return sol::nil;
        return sol::make_object( g_lua_state2, target );
    }

#endif
}
