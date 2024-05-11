#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

// #include "../../renderer/c_fonts.h"
// #include "../../renderer/c_renderer.h"
// #include "../../utils/c_keybind_system.h"
#include "c_draw_list.hpp"
#include "menu_shared.hpp"
#include "../../include/imgui/imgui.h"
// #include "../../renderer/color.h"
#include "animations.hpp"
#include "c_base_window.hpp"
#include "c_base_window.hpp"
/*
#include "components/c_checkbox.hpp"
#include "components/c_component.hpp"
#include "components/c_conditional.hpp"
#include "components/c_topbar.hpp"
*/

#undef max
namespace config {
    struct ConfigVar;
}

namespace menu::framework {
    namespace components {
        class c_conditional;
        class c_button;
        class c_component;
        class c_checkbox;
        class c_select;
        class c_slider_float;
        class c_slider_int;
        class c_multi_select;
    }

    class c_window : public c_base_window {
    public:
        struct item_container_t {
            item_container_t( ) = default;

            std::deque< std::shared_ptr< components::c_component > > children;
            std::deque< std::shared_ptr< components::c_component > > lua_children;
            std::optional< std::function< bool( ) > >                condition;
            c_base_window*                                           parent{ };

            auto reset( ) const -> void{ for ( const auto child : children ) child->reset( ); }

            auto checkbox(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > config_var,
                bool                                 lua = false
            ) -> std::shared_ptr< components::c_checkbox >;
#if enable_lua
            auto lua_checkbox( sol::object label, sol::object config_var ) -> std::shared_ptr< components::c_checkbox >;
#endif
            auto checkbox(
                std::string                   label,
                std::function< void( bool ) > callback,
                bool*                         checked
            ) -> std::shared_ptr< components::c_checkbox >;
            auto select(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > config_var,
                std::vector< std::string >           items,
                bool                                 lua = false
            ) -> std::shared_ptr< components::c_select >;
#if enable_lua
            auto lua_select(
                sol::object label,
                sol::object config_var,
                sol::object items
            ) -> std::shared_ptr< components::c_select >;
#endif
            auto multi_select(
                std::string                                         label,
                std::vector< std::shared_ptr< config::ConfigVar > > config_vars,
                std::vector< std::string >                          items,
                bool                                                lua = false
            ) -> std::shared_ptr< components::c_multi_select >;
#if enable_lua
            auto lua_multi_select(
                sol::object label,
                sol::object config_vars,
                sol::object items
            ) -> std::shared_ptr< components::c_multi_select >;
#endif
            auto slider_int(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > config_var,
                int32_t                              min,
                int32_t                              max,
                int32_t                              step = 1,
                bool                                 lua  = false
            ) -> std::shared_ptr< components::c_slider_int >;
#if enable_lua
            auto lua_slider_int(
                sol::object label,
                sol::object config_var,
                sol::object min,
                sol::object max,
                sol::object step
            ) -> std::shared_ptr< components::c_slider_int >;
#endif
            auto slider_float(
                std::string                          label,
                std::shared_ptr< config::ConfigVar > config_var,
                float                                min,
                float                                max,
                float                                step = 0.1f,
                bool                                 lua  = false
            ) -> std::shared_ptr< components::c_slider_float >;
#if enable_lua
            auto lua_slider_float(
                sol::object label,
                sol::object config_var,
                sol::object min,
                sol::object max,
                sol::object step
            ) -> std::shared_ptr< components::c_slider_float >;
#endif
            auto button(
                std::string                    label,
                const std::function< void( ) > on_click,
                bool                           lua = false
            ) -> std::shared_ptr< components::c_button >;
#if enable_lua
            auto lua_button( sol::object label, sol::object on_click ) -> std::shared_ptr< components::c_button >;
#endif
            auto add_conditional(
                std::function< bool( ) > should_enable,
                bool                     lua = false
            ) -> std::shared_ptr< components::c_conditional >;

#if enable_lua
            auto lua_add_conditional( sol::object should_enable ) -> std::shared_ptr< components::c_conditional >;
            auto lua_get_child( sol::object name ) -> sol::object;
#endif

            auto get_child( std::string name, bool lower = false ) -> std::shared_ptr< components::c_component >{
                const auto found = std::ranges::find_if(
                    children,
                    [&]( std::shared_ptr< components::c_component > child ) -> bool{
                        if ( lower ) {
                            auto l = child->get_label( );
                            std::ranges::transform( l, l.begin( ), ::tolower );
                            // fmt::print( "l: [{}] name: [{}]   |   {}", l, name, l == name );
                            return l == name;
                        }

                        return rt_hash( child->get_label( ).data( ) ) == rt_hash( name.data( ) );
                    }
                );

                if ( found == children.end( ) ) return nullptr;

                return *found;
            }

            auto remove_child( std::string name ) -> void{
                const auto found = get_child( name );

                if ( !found ) return;

                remove_child( found );
            }

            auto remove_child( std::shared_ptr< components::c_component > component ) -> void{
                auto found = std::ranges::remove(
                    children,
                    component
                );

                if ( found.empty( ) ) return;

                children.erase( found.begin( ), found.end( ) );
            }

        private:
            auto push( const std::shared_ptr< components::c_component > component, bool lua = false ) -> void{
                component->set_parent( parent );
                children.push_back( component );
                if ( lua ) lua_children.push_back( component );
            }
        };

        struct section_t : item_container_t {
            section_t( ) = default;

            auto get_height( ) const -> float{
                float height = 0.f;
                for ( const auto child : children ) { height += child->get_height( ) + parent->get_padding( ); }

                return height;
            }

            auto is_hovered( const Vec2 start, const float width ) const -> bool{
                return parent->cursor_in_rect( start, Vec2( width, get_height( ) ) );
            }

            std::string name;
        };

        struct navigation_t {
            navigation_t( ){
                animation = AnimationState( .3f );
                animation.start( );
                animation.animation_start = animation.animation_start + animation.animation_time;
            }

            auto add_section(
                std::string                               name,
                std::optional< std::function< bool( ) > > conditional = std::nullopt,
                bool                                      lua         = false
            ) -> std::shared_ptr< section_t >{
                std::ranges::transform( name, name.begin( ), ::toupper );

                if ( auto found = get_section( name ); found ) return found;

                auto ptr       = std::make_shared< section_t >( );
                ptr->name      = name;
                ptr->parent    = parent;
                ptr->condition = conditional;

                sections.push_back( ptr );
                if ( lua ) lua_sections.push_back( ptr );

                return ptr;
            }

#if enable_lua
            auto lua_add_section( sol::object name ) -> std::shared_ptr< section_t >;

            auto lua_get_section( sol::object label ) -> sol::object;
#endif

            auto get_section( const std::string label, bool lower = false ) -> std::shared_ptr< section_t >{
                const auto found = std::ranges::find_if(
                    sections,
                    [&]( const std::shared_ptr< section_t > section ) -> bool{
                        if ( lower ) {
                            auto n = section->name;
                            std::ranges::transform( n, n.begin( ), ::tolower );
                            return n == label;
                        }
                        return rt_hash( section->name.data( ) ) == rt_hash( label.data( ) );
                    }
                );

                if ( found == sections.end( ) ) return nullptr;

                return *found;
            }

            auto remove_section( std::string label ) -> void{
                const auto found = get_section( label );
                if ( !found ) return;
                remove_section( found );
            }

            auto remove_section( std::shared_ptr< section_t > section ) -> void{
                auto found = std::ranges::remove(
                    sections,
                    section
                );

                if ( found.empty( ) ) return;

                sections.erase( found.begin( ), found.end( ) );
            }

            auto ease_in_out_cubic( float x ) const -> float{
                return x < 0.5f ? 4.f * x * x * x : 1.f - pow( -2.f * x + 2.f, 3.f ) / 2.f;
            }

            auto calculate_underline_length( const float length ) -> float{
                const auto progress = 1.f - animation_queue.current( );
                return ( length ) * ease_in_out_cubic( progress );
            }

            auto get_height( ) const -> float{
                auto    h1 = 0.f;
                auto    h2 = 0.f;
                int32_t i  = 0;
                for ( const auto s : sections ) {
                    if ( i % 2 == 0 ) h1 += s->get_height( ) + parent->get_padding( ) * 2.f;
                    else h2 += s->get_height( ) + parent->get_padding( ) * 2.f;
                    i++;
                }

                return std::max( h1, h2 ) + parent->get_padding( ) * 4.f;
            }

            bool                                       was_hovered{ false };
            AnimationState                             animation;
            animation_queue_t< float >                 animation_queue;
            int32_t                                    selected{ };
            std::string                                name{ };
            c_window*                                  parent{ };
            std::deque< std::shared_ptr< section_t > > sections{ };
            std::deque< std::shared_ptr< section_t > > lua_sections{ };
            int32_t                                    display_order{ };
            int32_t                                    scroll_offset{ 0 };
        };

    public:
        c_window( ) = default;

        explicit c_window( const hash_t name )
            : c_base_window( name ){
        }

        c_window( const float height, const float width )
            : c_base_window( height, width ){
        }

        auto push( std::string label, int32_t display_order, bool lua = false ) -> std::shared_ptr< navigation_t >;

        auto draw_contents( ) -> void override;

        // auto get_size( ) const -> Vec2 override{
        //     return {
        //         get_width(),
        //         get_height()
        //     };
        // }

#if enable_lua
        auto lua_push( sol::object label, sol::object display_order ) -> std::shared_ptr< navigation_t >;
        auto lua_get_navigation( sol::object label ) -> std::shared_ptr< navigation_t >;
#endif
        auto get_navigation( std::string label, bool lower = false ) -> std::shared_ptr< navigation_t >;
        auto remove_navigation( std::string label ) -> void;
        auto remove_navigation( std::shared_ptr< navigation_t > navigation ) -> void;

        auto on_lua_reset( ) -> void;

        auto process_input( ) -> void;

        auto is_opened( ) const -> bool{ return m_opened; }

        auto get_name( ) const -> hash_t{ return m_name; }


        auto wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void override;
        auto initialize( ) -> void;
#if enable_lua
        auto lua_set_position( sol::object position ) -> void;
#endif

        auto set_active_component( components::c_component* component ) -> void{
            auto i = m_navigations[ m_selected ];

            for ( const auto s : m_navigations[ m_selected ]->sections ) {
                for ( auto c : s->children ) {
                    if ( reinterpret_cast< void* >( c.get( ) ) == reinterpret_cast< void* >(
                        component ) )
                        m_active_component = c;
                }
            }
        }

        std::mutex m_mutex;

    private:
        auto calculate_column_height( ) const -> float{ return get_size(  ).y - get_padding(  ) * 2.f; }

        auto calculate_column_width( ) const -> float;

        auto calculate_box_size(
            ImFont*          font,
            std::string_view text,
            float            text_size
        ) const -> std::pair< Vec2, Vec2 >;
        auto draw_navbar( ) -> bool;
        auto draw_top_bar( ) -> void;

    protected:
        auto centered_text( std::string_view text, float y ) -> Vec2;

        int32_t m_current_navigation{ -1 };
        int32_t m_selected{ 0 };
        //animation_queue_t< float > m_navigations_animation;
        std::deque< std::shared_ptr< navigation_t > > m_navigations{ };
        std::deque< std::shared_ptr< navigation_t > > m_lua_navigations{ };
        std::vector< std::shared_ptr< section_t > >   m_sections;
        std::shared_ptr< Renderer::Texture >          m_logo;
    };
}
