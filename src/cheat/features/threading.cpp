#include "pch.hpp"
#include "threading.hpp"

namespace features {
    auto Threading::is_feature_thread( ) const -> bool{
        static auto is_multi_core = g_config->misc.use_multi_core_runtime->get< bool >( );

        if ( is_multi_core ) { return feature_thread == std::this_thread::get_id( ); }

        return true;
    }

    auto Threading::is_render_thread( ) const -> bool{
        static auto is_multi_core = g_config->misc.use_multi_core_runtime->get< bool >( );

        if ( is_multi_core ) { return render_thread == std::this_thread::get_id( ); }

        return true;
    }
}

std::unique_ptr< features::Threading > g_threading = std::make_unique< features::Threading >( );
