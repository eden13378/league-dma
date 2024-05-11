#include "pch.hpp"
#if __DEBUG

#include "../../features/entity_list.hpp"

#include "test.hpp"

#include <boost/assert.hpp>

#include "../state.hpp"
#include "../../features/feature.hpp"

#define TEST_ENTITY_LIST_TABLE( lua_fn )     auto test_##lua_fn( sol::state& state ) -> void{ \
                                                  state.script( std::format(R"(result = features.entity_list:{}())", #lua_fn) ); \
\
                                                  sol::object result = state[ "result" ];\
\
                                                  BOOST_ASSERT( result.get_type( ) == sol::type::table );\
                                                  auto table = result.as< sol::table >( );\
\
                                                  BOOST_ASSERT( table.size( ) == g_features->entity_list->lua_fn( ).size( ) );\
                                                  BOOST_ASSERT( !!table.size( ) );\
                                          \
                                                  for ( auto [ i,o ] : table ) {\
                                                      if ( o.get_type( ) == sol::type::nil ) {\
                                                          debug_log( "object was nil" );\
                                                          continue;\
                                                      }\
\
                                                      BOOST_ASSERT( o.is<Object*>( ) );\
                                                  }\
                                              }

namespace lua::test {
    // "get_enemies",
    // sol::resolve( &EntityList2::get_enemies_lua ),
    // "get_allies",
    // sol::resolve( &EntityList2::get_allies_lua ),
    // "get_enemy_turrets",
    // sol::resolve( &EntityList2::get_enemy_turrets_lua ),
    // "get_ally_turrets",
    // sol::resolve( &EntityList2::get_ally_turrets_lua ),
    // "get_ally_minions",
    // sol::resolve( &EntityList2::get_ally_minions_lua ),
    // "get_enemy_minions",
    // sol::resolve( &EntityList2::get_enemy_minions_lua ),
    // "get_by_index",
    // sol::resolve( &EntityList2::get_by_index_raw ),
    // "get_by_network_id",
    // sol::resolve( &EntityList2::get_by_network_id_raw ),
    // // "on_pre_call",
    // // &EntityList2::push_pre_run_callback,
    // // "on_post_call",
    // // &EntityList2::push_post_run_callback,
    // "get_all",
    // sol::resolve( &EntityList2::get_all_table ),
    // "get_ally_missiles",
    // sol::resolve( &EntityList2::get_ally_missiles_lua ),
    // "get_enemy_missiles",
    // sol::resolve( &EntityList2::get_enemy_missiles_lua ),
    // "get_in_range",
    // sol::resolve( &EntityList2::get_in_range_table ),
    // "ally_uncategorized",
    // sol::resolve( &EntityList2::get_ally_uncategorized_lua ),
    // "enemy_uncategorized",
    // sol::resolve( &EntityList2::get_enemy_uncategorized_lua )

    TEST_ENTITY_LIST_TABLE( get_allies )
    TEST_ENTITY_LIST_TABLE( get_enemies )
    TEST_ENTITY_LIST_TABLE( get_enemy_turrets )
    TEST_ENTITY_LIST_TABLE( get_ally_turrets )
    TEST_ENTITY_LIST_TABLE( get_ally_minions )
    TEST_ENTITY_LIST_TABLE( get_enemy_minions )
    TEST_ENTITY_LIST_TABLE( get_ally_missiles )
    TEST_ENTITY_LIST_TABLE( get_enemy_missiles )
    TEST_ENTITY_LIST_TABLE( get_ally_uncategorized )
    TEST_ENTITY_LIST_TABLE( get_enemy_uncategorized )

    auto test_get_all( sol::state& state ) -> void{
        // state.script( std::format( R"(result = features.entity_list:{}())", "get_all" ) );
        // sol::object result = state[ "result" ];
        // BOOST_ASSERT( result.get_type( ) == sol::type::table );
        // auto table = result.as< sol::table >( );
        // BOOST_ASSERT( table.size( ) == g_features->entity_list->get_all_table( ).size( ) );
        // BOOST_ASSERT( !!table.size( ) );
        // for ( auto [ i,o ] : table ) {
        //     if ( o.get_type( ) == sol::type::nil ) {
        //         debug_log( "object was nil" );
        //         continue;
        //     }
        //     BOOST_ASSERT( o.is<Object*>( ) );
        // }
    }


    auto test_entity_list( ) -> void{
        BOOST_ASSERT( !!g_local );

        auto& state = g_lua2->get_state( );

        test_get_enemies( state );
        test_get_all( state );
        test_get_allies( state );
        test_get_enemy_turrets( state );
        test_get_ally_turrets( state );
        test_get_ally_minions( state );
        test_get_enemy_minions( state );
        test_get_ally_missiles( state );
        test_get_enemy_missiles( state );
        test_get_ally_uncategorized( state );
        test_get_enemy_uncategorized( state );


        debug_log( "Entity List Test successful" );
    }
}

#endif
