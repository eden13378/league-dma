#pragma once
#include "../../lua-v2/state.hpp"
#if enable_lua

#include "module.hpp"

#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../target_selector/target_selector.hpp"
// #include "../../lua/c_lua.hpp"
#include "../../lua-v2/custom_structs.hpp"

namespace features::champion_modules {
    class LuaModule final : public IModule {
        auto get_name( ) -> hash_t override{ return ct_hash( "lua_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "lua" ); }

        auto initialize_menu( ) -> void override{
        }

    public:
        auto is_lua_module( ) -> bool override{ return true; }

        explicit LuaModule( const std::shared_ptr< lua::LuaState::ChampionModule >& module )
            : m_module( module ){ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto run( ) -> void override{
            if ( !g_lua2 ) return;

            g_lua2->execute_locked(
                [&]( ) -> void{
                    if ( !m_module || !m_module->get_priorities || !m_module->get_priorities.lua_state( ) ) return;

                    m_priority_list.clear( );
                    const auto r_p = m_module->get_priorities( );
                    if ( !r_p.valid( ) || r_p.get_type( ) != sol::type::table ) return;
                    const sol::table p = r_p;

                    for ( auto pr : p ) {
                        if ( pr.second.get_type( ) != sol::type::string ) continue;
                        auto s = pr.second.as< std::string >( );

                        switch ( rt_hash( s.data( ) ) ) {
                        case ct_hash( "spell_q" ):
                            m_priority_list.push_back( ESpellFlag::q_spell );
                            break;
                        case ct_hash( "spell_w" ):
                            m_priority_list.push_back( ESpellFlag::w_spell );
                            break;
                        case ct_hash( "spell_e" ):
                            m_priority_list.push_back( ESpellFlag::e_spell );
                            break;
                        case ct_hash( "spell_r" ):
                            m_priority_list.push_back( ESpellFlag::r_spell );
                            break;
                        }
                    }

                    initialize_spell_slots( );

                    if ( !m_champion_module_spell_data ) {
                        m_champion_module_spell_data = std::make_shared<
                            lua::ChampionModuleSpellData >( );
                    }

                    m_champion_module_spell_data->spell_q = lua::LuaSpellSlot( get_slot_q( ).get( ) );
                    m_champion_module_spell_data->spell_w = lua::LuaSpellSlot( get_slot_w( ).get( ) );
                    m_champion_module_spell_data->spell_e = lua::LuaSpellSlot( get_slot_e( ).get( ) );
                    m_champion_module_spell_data->spell_r = lua::LuaSpellSlot( get_slot_r( ).get( ) );

                    if ( const auto target =
                        g_features->target_selector->get_default_target( )
                    )
                        m_champion_module_spell_data->target = target;

                    IModule::run( );
                }
            );
        }

        auto on_draw( ) -> void override{
            if ( !g_lua2 && m_module->on_draw ) return;

            g_lua2->execute_locked( [&]( ) -> void{ g_lua2->run_function( m_module->on_draw ); } );
        }

        auto initialize( ) -> void override{
            if ( !g_lua2 ) return;

            debug_log( "initializing lua module" );

            g_lua2->run_function( m_module->initialize );

            debug_log( "initializing lua module done" );
        }

    protected:
        auto spell_q( ) -> bool override{
            if ( !g_lua2 ||
                !m_module ||
                !m_module->spell_q ||
                !m_module->spell_q.valid( ) ||
                m_module->spell_q.lua_state( ) != g_lua_state2
            )
                return false;

            const auto result = g_lua2->run_function(
                m_module->spell_q,
                m_champion_module_spell_data
                    ? sol::make_object( g_lua_state2, m_champion_module_spell_data.get( ) )
                    : sol::nil
            );

            if ( !result.lua_state( ) ||
                !result.valid( ) ||
                result.lua_state( ) != g_lua_state2 ||
                result.get_type( ) != sol::type::boolean
            )
                return false;

            return result.as< bool >( );
        }

        auto spell_w( ) -> bool override{
            if ( !g_lua2 ||
                !m_module ||
                !m_module->spell_w ||
                !m_module->spell_w.valid( ) ||
                m_module->spell_w.lua_state( ) != g_lua_state2
            )
                return false;

            const auto result = g_lua2->run_function(
                m_module->spell_w,
                m_champion_module_spell_data
                    ? sol::make_object( g_lua_state2, m_champion_module_spell_data.get( ) )
                    : sol::nil
            );

            if ( !result.lua_state( ) ||
                !result.valid( ) ||
                result.lua_state( ) != g_lua_state2 ||
                result.get_type( ) != sol::type::boolean
            )
                return false;

            return result.as< bool >( );
        }

        auto spell_e( ) -> bool override{
            if ( !g_lua2 ||
                !m_module ||
                !m_module->spell_e ||
                !m_module->spell_e.valid( ) ||
                m_module->spell_e.lua_state( ) != g_lua_state2
            )
                return false;

            const auto result = g_lua2->run_function(
                m_module->spell_e,
                m_champion_module_spell_data
                    ? sol::make_object( g_lua_state2, m_champion_module_spell_data.get( ) )
                    : sol::nil
            );

            if ( !result.lua_state( ) ||
                !result.valid( ) ||
                result.lua_state( ) != g_lua_state2 ||
                result.get_type( ) != sol::type::boolean
            )
                return false;

            return result.as< bool >( );
        }

        auto spell_r( ) -> bool override{
            if ( !g_lua2 ||
                !m_module ||
                !m_module->spell_r ||
                !m_module->spell_r.valid( ) ||
                m_module->spell_r.lua_state( ) != g_lua_state2
            )
                return false;

            const auto result = g_lua2->run_function(
                m_module->spell_r,
                m_champion_module_spell_data
                    ? sol::make_object( g_lua_state2, m_champion_module_spell_data.get( ) )
                    : sol::nil
            );

            if ( !result.lua_state( ) ||
                !result.valid( ) ||
                result.lua_state( ) != g_lua_state2 ||
                result.get_type( ) != sol::type::boolean
            )
                return false;

            return result.as< bool >( );
        }

    private:
        std::shared_ptr< lua::LuaState::ChampionModule > m_module{ };
        std::shared_ptr< lua::ChampionModuleSpellData >  m_champion_module_spell_data{ };
    };
}

#endif
