#include "pch.hpp"

#include "feature.hpp"

#include "champion_modules/module.hpp"

std::unique_ptr< features::Features > g_features = std::make_unique< features::Features >( );

#if enable_lua
auto features::IFeature::lua_pre_run( ) -> void{
    //std::unique_lock lock( m_lua_mutex );
    //g_lua->call_callbacks( m_lua_pre_run_callbacks, sol::nil, true );
}

auto features::IFeature::lua_post_run( ) -> void{
    //std::unique_lock lock( m_lua_mutex );
    //g_lua->call_callbacks( m_lua_post_run_callbacks, sol::nil, true );
}

auto features::IFeature::on_lua_reset( ) -> void{
    // std::unique_lock lock( m_lua_mutex );
    m_lua_post_run_callbacks.functions.clear( );
    m_lua_pre_run_callbacks.functions.clear( );
}

auto features::IFeature::push_pre_run_callback( const sol::function& f ) -> void{
    // std::unique_lock lock( m_lua_mutex );
    m_lua_pre_run_callbacks.functions.push_back( f );
}

auto features::IFeature::push_post_run_callback( const sol::function& f ) -> void{
    // std::unique_lock lock( m_lua_mutex );
    m_lua_post_run_callbacks.functions.push_back( f );
}
#endif

auto features::Features::get_module_for_champion( hash_t name ) -> std::shared_ptr< champion_modules::IModule >{
    const auto found = std::ranges::find_if(
        modules,
        [name]( const std::shared_ptr< champion_modules::IModule >& module ) -> bool{
            return module->get_champion_name( ) == name;
        }
    );

    if ( found == modules.end( ) ) return nullptr;

    return *found;
}
