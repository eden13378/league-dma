#include "pch.hpp"


#include "resource_updater.hpp"

#include "debug_logger.hpp"
#include "directory_manager.hpp"
#include "path.hpp"
#include "../security/src/xorstr.hpp"

namespace utils::resource_updater {
    auto curl_http_write_callback_ru(
        char*             ptr,
        const std::size_t size,
        const std::size_t nmemb,
        void*             user_data
    ) -> std::size_t{
        auto* data = static_cast< std::string* >( user_data );
        data->append( ptr, size * nmemb );
        return size * nmemb;
    }

    auto get( const std::string_view url, const uint32_t timeout ) -> std::expected< std::string, const char* >{
        try {
            const auto curl_data = curl_easy_init( );
            if ( !curl_data ) return std::unexpected( "error setting up curl" );

            struct curl_slist* headers = nullptr;

            std::string response;
            curl_easy_setopt( curl_data, CURLOPT_URL, url.data( ) );

            headers = curl_slist_append( headers, _( "Expect:" ) );
            // headers = curl_slist_append ( headers, _ ( "Content-Type: application/json" ) );
            curl_easy_setopt( curl_data, CURLOPT_HTTPHEADER, headers );

            // curl_easy_setopt ( curl_data, CURLOPT_, 1L );
            //auto encrypted = security::encryption::encrypt ( data );
            curl_easy_setopt( curl_data, CURLOPT_TIMEOUT, timeout );
            curl_easy_setopt( curl_data, CURLOPT_SSL_VERIFYPEER, 1L );
            curl_easy_setopt( curl_data, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3 );
            curl_easy_setopt( curl_data, CURLOPT_MAXFILESIZE, 1024 * 1024 * 1024 );
            curl_easy_setopt( curl_data, CURLOPT_WRITEDATA, &response );
            curl_easy_setopt( curl_data, CURLOPT_WRITEFUNCTION, curl_http_write_callback_ru );
            curl_easy_setopt( curl_data, CURLOPT_USERAGENT, _( "slooted" ) );

            curl_easy_perform( curl_data );
            curl_slist_free_all( headers );
            curl_easy_cleanup( curl_data );

            return response;
        } catch ( ... ) { return std::unexpected( "error getting data" ); }
    }

    auto download_to_file( const std::string& local_path, const std::string& url ) -> void{
        try {
            if ( std::filesystem::exists( local_path ) ) return;

            const auto content = get( url, 8 );
            if ( !content ) return;
            if ( content->empty( ) ) return;

            std::ofstream stream( local_path, std::ios::binary );
            stream.write( content->data( ), content->length( ) );
            stream.flush( );
            stream.close( );
        } catch ( ... ) {
        }
    }

    auto update( ) -> void{
        // return;
#if enable_sentry
        auto transaction = app->sentry->start_transaction( _( "resources" ), "update" );
#endif
        // remove old resources
        if ( std::filesystem::exists( "C:\\resources" ) ) std::filesystem::remove_all( "C:\\resources" );
        if ( std::filesystem::exists( "C:\\hwinfo32.dll" ) ) std::filesystem::remove( "C:\\hwinfo32.dll" );

        const auto d_dragon_url     = "http://ddragon.leagueoflegends.com/";
        const auto c_dragon_url     = "https://cdn.communitydragon.org/";
        const auto c_raw_dragon_url = "https://raw.communitydragon.org/";

#if enable_sentry
        auto version_parse_transaction = transaction->add_child( "parse", "parse game version" );
#endif

        auto versions_data = get( std::format( "{}api/versions.json", d_dragon_url ), 15 );
        if ( !versions_data ) return;
        auto versions = nlohmann::json::parse( *versions_data );

        debug_log( "version: {}", versions[0].get< std::string >( ) );

        const auto current_version = versions[ 0 ].get< std::string >( );

        auto champions_data = get(
            std::format( "{}cdn/{}/data/en_US/champion.json", d_dragon_url, current_version ),
            15
        );
        if ( !champions_data ) return;
        auto champions = nlohmann::json::parse(
            *champions_data
        )[ "data" ];

#if enable_sentry
        version_parse_transaction->finish( );
#endif

        const auto resource_folder = path::join(
                { directory_manager::get_resources_path( ), "champions" }
            ).value( ) +
            "\\";
        if ( !std::filesystem::exists( resource_folder ) ) {
            std::filesystem::create_directories( resource_folder );
            download_to_file(
                std::format( "{}jammer.png", resource_folder ),
                std::format( "{}latest/game/assets/ux/minimap/icons/minimap_jammer_enemy.png", c_raw_dragon_url )
            );
            download_to_file(
                std::format( "{}ward.png", resource_folder ),
                std::format( "{}latest/game/assets/ux/minimap/icons/minimap_ward_pink_enemy.png", c_raw_dragon_url )
            );
        }

        const auto download_champion_data = [&]( nlohmann::json champion ) -> void{
            const auto champ_name       = champion[ "id" ].get< std::string >( );
            const auto champion_url     = std::format( "{}latest/champion/{}/", c_dragon_url, champ_name );
            const auto local_path       = std::format( "{}{}", resource_folder, champ_name );
            auto       champ_name_lower = champ_name;
            std::transform(
                champ_name_lower.begin( ),
                champ_name_lower.end( ),
                champ_name_lower.begin( ),
                ::tolower
            );
            if ( !std::filesystem::exists(
                std::format( "{}\\spells", local_path )
            ) )
                std::filesystem::create_directories( std::format( "{}\\spells", local_path ) );

            try {
                std::vector< std::thread > threads;

                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\{}_square.png", local_path, champ_name ),
                            std::format( "{}square.png", champion_url )
                        );
                    }
                );
                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\spells\\{}_q.png", local_path, champ_name ),
                            std::format( "{}ability-icon/q", champion_url )
                        );
                    }
                );
                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\spells\\{}_w.png", local_path, champ_name ),
                            std::format( "{}ability-icon/w", champion_url )
                        );
                    }
                );
                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\spells\\{}_e.png", local_path, champ_name ),
                            std::format( "{}ability-icon/e", champion_url )
                        );
                    }
                );
                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\spells\\{}_r.png", local_path, champ_name ),
                            std::format( "{}ability-icon/r", champion_url )
                        );
                    }
                );
                threads.emplace_back(
                    [&]( ) -> void{
                        download_to_file(
                            std::format( "{}\\spells\\{}_passive.png", local_path, champ_name ),
                            std::format( "{}ability-icon/passive", champion_url )
                        );
                    }
                );

                for ( auto& thread : threads ) thread.join( );
            } catch ( ... ) {
            }
        };

        std::vector< nlohmann::json > champs_array;
        for ( auto basic_jsons : champions ) champs_array.push_back( basic_jsons );

        std::vector< std::thread > threads;

        const auto do_download = [&]( const size_t offset ) -> void{
            for ( auto i = offset; i < champs_array.size( ); i += 3 ) {
                if ( i >= champs_array.size( ) ) continue;

                download_champion_data( champs_array[ i ] );
            }
        };

        threads.emplace_back( do_download, 0u );
        threads.emplace_back( do_download, 1u );
        threads.emplace_back( do_download, 2u );

        for ( auto& thread : threads ) thread.join( );

        const std::string extra_resouces_path = "https://cdn.slotted.cc/public/league-content/";
        const std::array  extra_resources{
            "common/jammer.png",
            "common/ward.png",
            "common/blue_ward.png",
            "common/clone.png",
            "common/normal_ward.png",
            "common/zombie_ward.png",

            "common/zombieward.png",
            "common/blueward.png",
            "common/controlward.png",
            "common/trinket_icon.png",
            "common/stopwatch.png",
            "common/zhonyas.png",
            "common/galeforce.png",
            "common/qss.png",
            "common/shieldbow.png",
            "common/mercurial.png",
            "automatic_ward_icon.png",
            "bold_control_ward_icon.png",
            "bold_ward_highlight.png",
            "bold_ward_icon.png",

            "jungle/baron_square.png",
            "jungle/bluesentinel_square.png",
            "jungle/brambleback_square.png",
            "jungle/crab_square.png",
            "jungle/dragon_square.png",
            "jungle/greatermurkwolf_square.png",
            "jungle/gromp_square.png",
            "jungle/krug_square.png",
            "jungle/murkwolf_square.png",
            "jungle/razorbeak_square.png",
            "jungle/razorbeakmini_square.png",
            "jungle/sruriftherald_square.png",
            "summoners/S5_SummonerSmiteDuel.png",
            "summoners/S5_SummonerSmitePlayerGanker.png",
            "summoners/SummonerBarrier.png",
            "summoners/SummonerBoost.png",
            "summoners/SummonerDot.png",
            "summoners/SummonerExhaust.png",
            "summoners/SummonerFlash.png",
            "summoners/SummonerFlashPerksHextechFlashtraptionV2.png",
            "summoners/SummonerHaste.png",
            "summoners/SummonerHeal.png",
            "summoners/SummonerMana.png",
            "summoners/SummonerPoroRecall.png",
            "summoners/SummonerPoroThrow.png",
            "summoners/SummonerSmite.png",
            "summoners/SummonerSnowball.png",
            "summoners/SummonerSnowURFSnowball_Mark.png",
            "summoners/SummonerTeleport.png",
            "select1.wav",
            "killsound.wav"
        };

        const auto resource_folder2 = directory_manager::get_resources_path( ) + "\\";

        const std::array local_folders{ "summoners", "common", "jungle" };

        for ( auto f : local_folders ) {
            auto path = std::string( resource_folder2 ) + f;

            if ( !std::filesystem::exists( path ) ) std::filesystem::create_directories( path );
        }

        for ( auto r : extra_resources ) {
            auto path = extra_resouces_path + r;

            const auto local_path = std::format( "{}{}", resource_folder2, r );

            download_to_file( local_path, path );
        }

#if enable_sentry
        transaction->finish( );
#endif
    }
}
