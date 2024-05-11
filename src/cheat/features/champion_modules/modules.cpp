#include "pch.hpp"

#include "modules.hpp"

#include "ezreal_module.hpp"
#include "twitch_module.hpp"
#include "kalista_module.hpp"
#include "xerath_module.hpp"
#include "ashe_module.hpp"
#include "kaisa_module.hpp"
#include "jinx_module.hpp"
#include "kogmaw_module.hpp"
#include "syndra_module.hpp"
#include "tahmkench_module.hpp"
#include "varus_module.hpp"
#include "blitzcrank_module.hpp"
#include "zed_module.hpp"
#include "zeri_module.hpp"
#include "brand_module.hpp"
#include"riven_module.hpp"
#include"karthus_module.hpp"
#include "jax_module.hpp"
#include "kayle_module.hpp"
#include "tristana_module.hpp"
#include "kassadin_module.hpp"
#include "ryze_module.hpp"
#include "leesin_module.hpp"
#include "leblanc_module.h"
#include "pyke_module.hpp"
#include "ahri_module.hpp"
#include "veigar_module.hpp"
#include "lucian_module.hpp"
#include "jhin_module.hpp"
#include "cassiopeia_module.hpp"
#include "lux_module.hpp"
#include "mundo_module.hpp"
#include "tryndamere_module.hpp"
#include "corki_module.hpp"
#include "zyra_module.hpp"
#include "morgana_module.hpp"
#include "yasuo_module.hpp"
#include "yone_module.hpp"
#include "viktor_module.hpp"
#include "senna_module.hpp"
#include "vayne_module.hpp"
#include "kindred_module.hpp"
#include "belveth_module.hpp"
#include "sylas_module.hpp"
#include "katarina_module.hpp"
#include "chogath_module.hpp"
#include "olaf_module.hpp"
#include "ziggs_module.hpp"
#include "samira_module.hpp"
#include "seraphine_module.hpp"
#include "sett_module.hpp"
#include "gnar_module.hpp"
#include "sion_module.hpp"
#include "nilah_module.hpp"
#include "sivir_module.hpp"
#include "illaoi_module.hpp"
#include "irelia_module.hpp"
#include "zoe_module.hpp"
#include "kennen_module.hpp"
#include "draven_module.hpp"
#include "akali_module.hpp"
#include "missfortune_module.hpp"
#include "caitlyn_module.hpp"
#include "vladimir_module.hpp"
#include "aatrox_module.hpp"
#include "vex_module.hpp"
#include "yorick_module.hpp"
#include "darius_module.hpp"
#include "taliyah_module.hpp"
#include "swain_module.hpp"
#include "orianna_module.hpp"
#include "lulu_module.hpp"
#include "soraka_module.hpp"
#include "diana_module.hpp"
#include "velkoz_module.hpp"
#include "anivia_module.hpp"
#include "aurelionsol_module.hpp"
//#include "viego_module.hpp"
#include "gangplank_module.hpp"
#include "annie_module.hpp"
#include "fiora_module.hpp"
#include "jarvan_module.hpp"
#include "xayah_module.hpp"
#include "nami_module.hpp"
#include "mordekaiser_module.hpp"
#include "amumu_module.hpp"
#include "zilean_module.hpp"
#include "yuumi_module.hpp"
#include "janna_module.hpp"
#include "malzahar_module.hpp"
#include "thresh_module.hpp"
#include "milio_module.hpp"
#include "akshan_module.hpp"
#include "evelynn_module.hpp"
#include "trundle_module.hpp"
#include "teemo_module.hpp""
//#include "azir_module.hpp"

#include "../feature.hpp"

#define register_module( cls ) g_features->modules.push_back( std::make_shared< cls>( ) )

namespace features::champion_modules {
    auto initialize( ) -> void{
        register_module( ezreal_module );
        register_module( twitch_module );
        register_module( kalista_module );
        register_module( xerath_module );
        register_module( ashe_module );
        register_module( kaisa_module );
        register_module( jinx_module );
        register_module( kogmaw_module );
        register_module( syndra_module );
        register_module( tahm_module );
        register_module( varus_module );
        register_module( blitzcrank_module );
        register_module( zed_module );
        register_module( zeri_module );
        register_module( brand_module );
        register_module( riven_module );
        register_module( karthus_module );
        register_module( jax_module );
        register_module( KayleModule );
        register_module( tristana_module );
        register_module( kassadin_module );
        register_module( ryze_module );
        register_module( leesin_module );
        register_module( leblanc_module );
        register_module( pyke_module );
        register_module( ahri_module );
        register_module( veigar_module );
        register_module( lucian_module );
        register_module( jhin_module );
        register_module( cassiopeia_module );
        register_module( lux_module );
        register_module( mundo_module );
        register_module( tryndamere_module );
        register_module( corki_module );
        register_module( ZyraModule );
        register_module( morgana_module );
        register_module( yasuo_module );
        register_module( yone_module );
        register_module( viktor_module );
        register_module( senna_module );
        register_module( vayne_module );
        register_module( kindred_module );
        register_module( belveth_module );
        register_module( sylas_module );
        register_module( katarina_module );
        register_module( chogath_module );
        register_module( olaf_module );
        register_module( ZiggsModule );
        register_module( samira_module );
        register_module( seraphine_module );
        register_module( sett_module );
        register_module( gnar_module );
        register_module( sion_module );
        register_module( nilah_module );
        register_module( sivir_module );
        register_module( illaoi_module );
        register_module( irelia_module );
        register_module( ZoeModule );
        register_module( kennen_module );
        register_module( draven_module );
        register_module( akali_module );
        register_module( missfortune_module );
        register_module( caitlyn_module );
        register_module( aatrox_module );
        register_module( vladimir_module );
        register_module( vex_module );
        register_module( yorick_module );
        register_module( darius_module );
        register_module( taliyah_module );
        register_module( swain_module );
        register_module( orianna_module );
        register_module( lulu_module );
        register_module( soraka_module );
        register_module( diana_module );
        register_module( velkoz_module );
        register_module( anivia_module );
        register_module( aurelion_module );
        register_module( gangplank_module );
        register_module( annie_module );
        register_module( fiora_module );
        register_module( jarvan_module );
        register_module( xayah_module );
        register_module( nami_module );
        register_module( mordekaiser_module );
        register_module( amumu_module );
        register_module( ZileanModule );
        register_module( yuumi_module );
        register_module( janna_module );
        register_module( malzahar_module );
        register_module( thresh_module );
        register_module( milio_module );
        register_module( akshan_module );
        register_module( evelynn_module );
        register_module( trundle_module );
        register_module( teemo_module );
        //register_module( azir_module );
    }
}
