#include "pch.hpp"

#include "c_window.hpp"

#include "menu_options.hpp"
#include "../../renderer/c_renderer.hpp"
#include "components/c_button.hpp"

#include "components/c_checkbox.hpp"
#include "components/c_select.hpp"
#include "components/c_slider.hpp"
#include "../../utils/utils.hpp"

#define NOMINMAX
#include <windowsx.h>
#if enable_new_lua
#include "../../lua-v2/lua_def.hpp"
#include "../../lua-v2/state.hpp"
#endif
#include "components/c_conditional.hpp"
#include "components/c_multi_select.hpp"
#if enable_lua
#include <sol/sol.hpp>
#endif


#undef max
#undef min

namespace menu::framework {
    auto c_window::item_container_t::checkbox(
        std::string                          label,
        std::shared_ptr< config::ConfigVar > config_var,
        bool                                 lua
    ) -> std::shared_ptr< components::c_checkbox >{
        if ( !config_var ) return nullptr;

        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_checkbox >( label, config_var );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_checkbox(
        sol::object label,
        sol::object config_var
    ) -> std::shared_ptr< components::c_checkbox >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( config_var, std::shared_ptr< config::ConfigVar >, "config_var_t" )

        return checkbox(
            label.as< std::string >( ),
            config_var.as< std::shared_ptr< config::ConfigVar > >( ),
            true
        );
    }
#endif

    auto c_window::item_container_t::checkbox(
        std::string                   label,
        std::function< void( bool ) > callback,
        bool*                         checked
    ) -> std::shared_ptr< components::c_checkbox >{
        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_checkbox >( label, callback, checked );
        push( c );
        return c;
    }

    auto c_window::item_container_t::select(
        std::string                          label,
        std::shared_ptr< config::ConfigVar > config_var,
        std::vector< std::string >           items,
        bool                                 lua
    ) -> std::shared_ptr< components::c_select >{
        if ( !config_var ) return nullptr;

        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_select >( label, config_var, items );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_select(
        sol::object label,
        sol::object config_var,
        sol::object items
    ) -> std::shared_ptr< components::c_select >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( config_var, std::shared_ptr< config::ConfigVar >, "config_var_t" )
        lua_arg_check_ct( items, std::vector<std::string>, "table<number, string>" )

        return select(
            label.as< std::string >( ),
            config_var.as< std::shared_ptr< config::ConfigVar > >( ),
            items.as< std::vector< std::string > >( ),
            true
        );
    }
#endif

    auto c_window::item_container_t::multi_select(
        std::string                                         label,
        std::vector< std::shared_ptr< config::ConfigVar > > config_vars,
        std::vector< std::string >                          items,
        bool                                                lua
    ) -> std::shared_ptr< components::c_multi_select >{
        for ( const auto& var : config_vars ) if ( !var ) return nullptr;

        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_multi_select >( label, config_vars, items );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_multi_select(
        sol::object label,
        sol::object config_vars,
        sol::object items
    ) -> std::shared_ptr< components::c_multi_select >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct(
            config_vars,
            std::vector< std::shared_ptr< config::ConfigVar > >,
            "table<number, config_var_t>"
        )
        lua_arg_check_ct( items, std::vector<std::string>, "table<number, string>" )

        std::vector< std::shared_ptr< config::ConfigVar > > v;
        sol::table                                          t = config_vars.as< sol::table >( );
        t.for_each(
            [&]( std::pair< sol::object, sol::object > element ) -> void{
                debug_log( "yes" );
                if ( element.second.is< std::shared_ptr< config::ConfigVar > >( ) )
                    v.push_back(
                        element.second.as< std::shared_ptr< config::ConfigVar > >( )
                    );
            }
        );

        return multi_select( label.as< std::string >( ), v, items.as< std::vector< std::string > >( ), true );
    }
#endif

    auto c_window::item_container_t::slider_int(
        std::string                          label,
        std::shared_ptr< config::ConfigVar > config_var,
        int32_t                              min,
        int32_t                              max,
        int32_t                              step,
        bool                                 lua
    ) -> std::shared_ptr< components::c_slider_int >{
        if ( !config_var ) return nullptr;

        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_slider_int >( label, min, max, step, config_var );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_slider_int(
        sol::object label,
        sol::object config_var,
        sol::object min,
        sol::object max,
        sol::object step
    ) -> std::shared_ptr< components::c_slider_int >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( config_var, std::shared_ptr< config::ConfigVar >, "config_var_t" )
        lua_arg_check_ct( min, int32_t, "number" )
        lua_arg_check_ct( max, int32_t, "number" )
        if ( step.get_type( ) != sol::type::nil )
            lua_arg_check_ct( step, int32_t, "number" )


        return slider_int(
            label.as< std::string >( ),
            config_var.as< std::shared_ptr< config::ConfigVar > >( ),
            min.as< int32_t >( ),
            max.as< int32_t >( ),
            step.get_type( ) == sol::type::nil ? 1 : step.as< int32_t >( ),
            true
        );
    }
#endif

    auto c_window::item_container_t::slider_float(
        std::string                          label,
        std::shared_ptr< config::ConfigVar > config_var,
        float                                min,
        float                                max,
        float                                step,
        bool                                 lua
    ) -> std::shared_ptr< components::c_slider_float >{
        if ( !config_var ) return nullptr;

        if ( auto child = get_child( label ); child ) return nullptr;

        const auto c = std::make_shared< components::c_slider_float >( label, min, max, step, config_var );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_slider_float(
        sol::object label,
        sol::object config_var,
        sol::object min,
        sol::object max,
        sol::object step
    ) -> std::shared_ptr< components::c_slider_float >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( config_var, std::shared_ptr< config::ConfigVar >, "config_var_t" )
        lua_arg_check_ct( min, float, "number" )
        lua_arg_check_ct( max, float, "number" )
        if ( step.get_type( ) != sol::type::nil )
            lua_arg_check_ct( step, float, "number" )


        return slider_float(
            label.as< std::string >( ),
            config_var.as< std::shared_ptr< config::ConfigVar > >( ),
            min.as< float >( ),
            max.as< float >( ),
            step.get_type( ) == sol::type::nil ? 0.1f : step.as< float >( ),
            true
        );
    }
#endif

    auto c_window::item_container_t::button(
        std::string                    label,
        const std::function< void( ) > on_click,
        bool                           lua
    ) -> std::shared_ptr< components::c_button >{
        if ( auto child = get_child( label ); child ) return nullptr;
        const auto c = std::make_shared< components::c_button >( label, on_click );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_button(
        sol::object label,
        sol::object on_click
    ) -> std::shared_ptr< components::c_button >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( on_click, std::function<void()>, "function" )

        return button(
            label.as< std::string >( ),
            [on_click]( ) -> void{
                if ( !on_click || !on_click.lua_state( ) || !on_click.valid( ) ) return;

                auto fn = on_click.as< sol::function >( );

                if ( !fn || !fn.valid( ) || fn.lua_state( ) ) return;

                fn( );
            },
            true
        );
    }
#endif

    auto c_window::item_container_t::add_conditional(
        std::function< bool( ) > should_enable,
        bool                     lua
    ) -> std::shared_ptr< components::c_conditional >{
        const auto c = std::make_shared< components::c_conditional >( should_enable );
        push( c, lua );
        return c;
    }
#if enable_lua
    auto c_window::item_container_t::lua_add_conditional(
        sol::object should_enable
    ) -> std::shared_ptr< components::c_conditional >{
        lua_arg_check_ct( should_enable, std::function<bool()>, "function" )
        return add_conditional( should_enable.as< std::function< bool( ) > >( ), true );
    }
#endif

#if enable_lua
    auto c_window::item_container_t::lua_get_child( sol::object name ) -> sol::object{
        lua_arg_check_ct( name, std::string, "string" )

        auto n = name.as< std::string >( );
        std::ranges::transform( n, n.begin( ), ::tolower );

        auto component = get_child( n, true );

        if ( !component ) return sol::nil;

        switch ( component->get_type( ) ) {
        case components::EComponent::button:
            return sol::make_object( g_lua_state2, ( components::c_button* )( component.get( ) ) );
        case components::EComponent::checkbox:
            return sol::make_object( g_lua_state2, ( components::c_checkbox* )( component.get( ) ) );
        case components::EComponent::conditional:
            return sol::make_object( g_lua_state2, ( components::c_conditional* )( component.get( ) ) );
        case components::EComponent::multi_select:
            return sol::make_object( g_lua_state2, ( components::c_multi_select* )( component.get( ) ) );
        case components::EComponent::select:
            return sol::make_object( g_lua_state2, ( components::c_select* )( component.get( ) ) );
        case components::EComponent::slider_int:
            return sol::make_object( g_lua_state2, ( components::c_slider_int* )( component.get( ) ) );
        case components::EComponent::slider_float:
            return sol::make_object( g_lua_state2, ( components::c_slider_float* )( component.get( ) ) );
        default: ;
        }

        return sol::nil;
    }

    auto c_window::navigation_t::lua_add_section( sol::object name ) -> std::shared_ptr< section_t >{
        lua_arg_check_ct( name, std::string, "string" )
        return add_section( name.as< std::string >( ), std::nullopt, true );
    }

    auto c_window::navigation_t::lua_get_section( sol::object label ) -> sol::object{
        lua_arg_check_ct( label, std::string, "string" )

        auto n = label.as< std::string >( );
        std::ranges::transform( n, n.begin( ), ::tolower );

        const auto section = get_section( n, true );
        if ( !section ) return sol::nil;

        return sol::make_object( g_lua_state2, section.get( ) );
    }
#endif
    auto c_window::push(
        const std::string label,
        const int32_t     display_order,
        bool              lua
    ) -> std::shared_ptr< navigation_t >{
        if ( const auto f = get_navigation( label ) ) return f;

        auto navigation           = std::make_shared< navigation_t >( );
        navigation->parent        = this;
        navigation->name          = label;
        navigation->display_order = display_order;
        m_navigations.push_back( navigation );
        if ( lua ) m_lua_navigations.push_back( navigation );
        return navigation;
    }

    void c_window::draw_contents( ){
        if ( !m_opened ) return;
        std::unique_lock lock( m_mutex );

        m_selected = std::clamp( m_selected, 0, static_cast< int32_t >( m_navigations.size( ) ) - 1 );

        m_any_right_click_menu_opened = false;
        if ( m_current_navigation != -1 ) {
            for ( const auto& sections : m_navigations[ m_current_navigation ]->sections ) {
                for ( const auto& child : sections->children ) {
                    if ( child->is_right_click_menu_opened( ) ) m_any_right_click_menu_opened = true;
                }
            }
        }

        std::ranges::sort(
            m_navigations,
            []( std::shared_ptr< navigation_t > first, std::shared_ptr< navigation_t > second ) -> bool{
                return first->display_order < second->display_order;
            }
        );

        g_render->filled_box( m_position, get_size( ), colors::background, 0 );
        g_render->box( m_position, get_size( ), colors::outline, 0, 2 );

        draw_navbar( );

        const auto navbar_width = 150.f * ( g_config ? g_config->misc.screen_scaling->get< float >( ) : 1.f );

        const auto container_size = Vec2( ( get_width( ) ) - navbar_width, get_height( ) );

        // container divider
        m_draw_list.line(
            m_position + Vec2( navbar_width + container_size.x / 2.f, 0.f ),
            m_position + Vec2( navbar_width + container_size.x / 2.f, get_height( ) ),
            colors::outline,
            2.f
        );

        const auto container_position = Vec2( m_position.x + navbar_width, m_position.y + m_topbar->get_height( ) );

        m_draw_list.set_clip_box( container_position, container_size - Vec2( 0.f, get_padding( ) ) );

        /* container content */
        const auto inner_width = container_size.x / 2.f - get_padding( ) * 2.f;

        std::array y_offset = {
            -static_cast< float >( m_navigations[ m_selected ]->scroll_offset ),
            -static_cast< float >( m_navigations[ m_selected ]->scroll_offset )
        };

        auto sections = m_sections;
        std::ranges::sort(
            sections,
            []( const std::shared_ptr< section_t > first, const std::shared_ptr< section_t > second ) -> bool{
                return first->get_height( ) > second->get_height( );
            }
        );

        const auto draw_section = [&]( const std::shared_ptr< section_t >& section, const int32_t side ) -> void{
            g_render->set_clip_box( m_position, get_size( ) );

            auto start_pos = container_position;
            if ( side == 1 ) start_pos.x += container_size.x / 2.f;

            const auto section_children_height = section->get_height( ) - get_padding( ) * 2.f;
            const auto section_label           = section->name;
            const auto size                    = g_render->get_text_size( section_label, g_fonts->get_block( ), 8.f );
            const auto text_position           = start_pos + Vec2(
                inner_width / 2.f - size.x / 2.f,
                get_padding( ) + y_offset[ side ]
            );

            const auto line_start = start_pos + Vec2(
                get_padding( ) * 2.f,
                get_padding( ) + size.y / 2.f + y_offset[ side ]
            );

            const auto section_color = Color( 40, 40, 40 );

            g_render->filled_box(
                line_start,
                Vec2( inner_width - get_padding( ) * 2.f, section_children_height + get_padding( ) ),
                Color::black( ).alpha( 25 )
            );

            m_draw_list.line(
                Vec2( line_start.x, line_start.y + section_children_height + get_padding( ) ),
                Vec2(
                    line_start.x + ( inner_width - 2.f * get_padding( ) ) + 1.f,
                    line_start.y + section_children_height + get_padding( )
                ),
                section_color,
                1.f
            );
            m_draw_list.line(
                line_start + Vec2( 0.f, 1.f ),
                Vec2( line_start.x, line_start.y + section_children_height + get_padding( ) ),
                section_color,
                1.f
            );
            m_draw_list.line(
                line_start + Vec2( inner_width - get_padding( ) * 2.f, 0.f ),
                Vec2(
                    line_start.x + inner_width - get_padding( ) * 2.f,
                    line_start.y + section_children_height + get_padding( )
                ),
                section_color,
                1.f
            );
            m_draw_list.line( line_start, Vec2( text_position.x - get_padding( ), line_start.y ), section_color, 1.f );
            m_draw_list.line(
                Vec2( text_position.x + size.x + get_padding( ), line_start.y ),
                Vec2( start_pos.x + inner_width, line_start.y ),
                section_color,
                1.f
            );
            m_draw_list.text_shadow(
                start_pos + Vec2( inner_width / 2.f - size.x / 2.f, get_padding( ) - 1.f + y_offset[ side ] + 1.f ),
                Color::white( ).alpha( 125 ),
                g_fonts->get_block( ),
                section_label.data( ),
                8
            );

            auto inner_y = get_padding( ) * 3.f + 13.f;

            for ( const auto& component : section->children ) {
                if ( !component->draw_as_child( ) ) continue;
                component->set_position(
                    Vec2( line_start.x + get_padding( ), m_position.y + y_offset[ side ] + inner_y )
                );
                component->set_width( inner_width - get_padding( ) * 4.f );
                const auto on_screen = m_position.y + y_offset[ side ] + inner_y < m_position.y + get_height( );
                if ( on_screen && ( !m_active_component || m_active_component == component ) ) {
                    if ( component->process_input( ) ) { m_active_component = component; } else
                        m_active_component =
                            nullptr;
                }
                component->draw( );
                inner_y += component->get_height( ) + get_padding( );
            }
            g_render->reset_clip_box( );

            y_offset[ side ] += get_padding( ) + size.y + section_children_height;
        };

        int32_t section_index = 0;
        for ( const auto& section : m_navigations[ m_selected ]->sections ) {
            if ( section->condition && !( *section->condition )( ) ) continue;
            draw_section( section, section_index % 2 );
            ++section_index;
        }

        m_draw_list.reset_clip_box( 500 );

        m_draw_list.render( );
        draw_top_bar( );

        m_mouse_state = EMouseState::none;
    }
#if enable_lua
    auto c_window::lua_push( sol::object label, sol::object display_order ) -> std::shared_ptr< navigation_t >{
        lua_arg_check_ct( label, std::string, "string" )
        lua_arg_check_ct( display_order, int32_t, "number" )

        return push( label.as< std::string >( ), display_order.as< int32_t >( ), true );
    }

    auto c_window::lua_get_navigation( sol::object label ) -> std::shared_ptr< navigation_t >{
        lua_arg_check_ct( label, std::string, "string" )

        auto l = label.as< std::string >( );
        std::ranges::transform( l, l.begin( ), ::tolower );

        return get_navigation( l, true );
    }
#endif

    auto c_window::get_navigation( const std::string label, bool lower ) -> std::shared_ptr< navigation_t >{
        const auto found = std::ranges::find_if(
            m_navigations,
            [label, lower]( const std::shared_ptr< navigation_t > nav ) -> bool{
                if ( lower ) {
                    auto n = nav->name;
                    std::ranges::transform( n, n.begin( ), ::tolower );
                    return n == label;
                }
                return nav->name == label;
            }
        );

        if ( found == m_navigations.end( ) ) return nullptr;

        return *found;
    }

    auto c_window::remove_navigation( const std::string label ) -> void{
        const auto found = get_navigation( label );
        if ( !found ) return;
        remove_navigation( found );
    }

    auto c_window::remove_navigation( std::shared_ptr< navigation_t > navigation ) -> void{
        auto to_remove = std::ranges::remove( m_navigations, navigation );
        if ( to_remove.empty( ) ) return;
        m_navigations.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto c_window::on_lua_reset( ) -> void{
        // std::thread(
        //     [&]( )-> void{
        // std::unique_lock lock( m_mutex );
        for ( const auto& navigation : m_navigations ) {
            for ( const auto& section : navigation->sections ) {
                for ( auto& lua_child : section->lua_children ) {
                    auto found = std::ranges::find( section->children, lua_child );

                    if ( found != section->children.end( ) ) { section->children.erase( found ); }
                }

                section->lua_children.clear( );
            }
            for ( auto& section : navigation->lua_sections ) {
                auto found = std::ranges::find( navigation->sections, section );

                if ( found != navigation->sections.end( ) ) { navigation->sections.erase( found ); }
            }
            navigation->lua_sections.clear( );
        }
        for ( auto& nav : m_lua_navigations ) {
            auto found = std::ranges::find( m_navigations, nav );

            if ( found != m_navigations.end( ) ) { m_navigations.erase( found ); }
        }
        m_lua_navigations.clear( );
        // }
        // ).detach( );
    }

    auto c_window::process_input( ) -> void{
        if ( g_keybind_system->was_key_pressed( utils::EKey::insert ) || g_keybind_system->was_key_pressed(
            utils::EKey::delete_key
        ) ) {
            m_opened = !m_opened;
            if ( !m_opened ) g_config_system->save( _( "cfg" ) );
        }

        if ( m_topbar->process_input( ) ) m_active_component = m_topbar;
        else m_active_component                              = nullptr;

        const auto screen_size = g_render->get_screensize( );

        if ( m_topbar->has_mouse_down( ) ) {
            set_position(
                Vec2(
                    std::clamp( get_cursor_position( ).x, 50.f, screen_size.x - 50.f ),
                    std::clamp( get_cursor_position( ).y, 0.f, screen_size.y - 50.f )
                ) + m_move_start_delta
            );
        }
    }

    auto c_window::wnd_proc_handler( HWND hwnd, const UINT msg, const WPARAM w_param, const LPARAM l_param ) -> void{
        c_base_window::wnd_proc_handler( hwnd, msg, w_param, l_param );

        switch ( msg ) {
        case WM_MOUSEWHEEL:
        {
            const auto a = GET_WHEEL_DELTA_WPARAM( w_param ) >= 0;
            m_navigations[ m_selected ]->scroll_offset += a ? -25 : 25;

            const auto navigation_max_height = m_navigations[ m_selected ]->get_height( );

            if ( navigation_max_height < get_height( ) ) { m_navigations[ m_selected ]->scroll_offset = 0; } else {
                m_navigations[ m_selected ]->scroll_offset = std::clamp(
                    m_navigations[ m_selected ]->scroll_offset,
                    0,
                    static_cast< int32_t >( navigation_max_height - get_height( ) )
                );
            }
        }
        break;
        default: ;
        }
    }

    auto c_window::initialize( ) -> void{
        // m_logo = g_render->load_texture_from_memory( logo_data, logo_size );

        m_topbar = std::make_shared< components::Topbar >(
            []( ) -> void{ app->unload( ); }
        );
        m_topbar->set_parent( this );
    }
#if enable_lua
    auto c_window::lua_set_position( sol::object position ) -> void{
        lua_arg_check_v( position, Vec2 )

        m_position = position.as< Vec2 >( );
    }
#endif

    auto c_window::calculate_column_width( ) const -> float{
        const auto navbar_width = 150.f * ( g_config ? g_config->misc.screen_scaling->get< float >( ) : 1.f );

        return ( get_width( ) * g_config->misc.screen_scaling->get< float >( ) - navbar_width ) / 2.f - get_padding( ) *
            2.f;
    }

    auto c_window::calculate_box_size(
        ImFont*                font,
        const std::string_view text,
        const float            text_size
    ) const -> std::pair< Vec2, Vec2 >{
        const auto size = g_render->get_text_size( text, font, text_size );

        return std::make_pair( Vec2{ ( size.x + get_padding( ) * 2.f ), ( size.y + get_padding( ) * 2.f ) }, size );
    }

    auto c_window::draw_navbar( ) -> bool{
        bool       one_hovered = false;
        const auto button_size = calculate_box_size( g_fonts->get_default( ), _( "ITEM" ), 16 );

        auto y_offset = get_height( ) / 2.f -
            ( button_size.first.y * static_cast< float >( m_navigations.size( ) ) ) / 2.f -
            ( get_padding( ) * static_cast< float >( m_navigations.size( ) ) ) / 2.f;

        const auto navbar_width = 150.f * ( g_config ? g_config->misc.screen_scaling->get< float >( ) : 1.f );


        if ( m_logo ) {
            m_draw_list.image(
                Vec2(
                    m_position.x + navbar_width / 2.f - 25.f,
                    m_position.y + m_topbar->get_height( ) + y_offset / 2.f - 25.f
                ),
                Vec2( 50.f, 50.f ),
                m_logo
            );
        }

        int32_t i = 0;
        for ( const auto& navigation : m_navigations ) {
            const bool selected = m_selected == i;
            const auto box      = calculate_box_size( g_fonts->get_default_navbar( ), navigation->name, 19 );

            const auto box_start = m_position + Vec2(
                navbar_width / 2.f - ( box.first.x ) / 2.f,
                get_padding( ) + y_offset
            );

            const auto hovered = cursor_in_rect( box_start, box.first );

            if ( hovered ) one_hovered = true;

            const auto underline_length_full = ( box_start.x + box.second.x ) - box_start.x;
            const auto length                = navigation->calculate_underline_length( underline_length_full );

            if ( hovered ) {
                if ( !navigation->was_hovered ) {
                    navigation->was_hovered = true;
                    navigation->animation_queue.add( 1.f, 0.15f );
                    navigation->animation.start( );
                }
            } else if ( navigation->was_hovered && m_selected != i ) {
                navigation->was_hovered = false;
                navigation->animation.start( );
                navigation->animation_queue.add( 0.f, 0.15f );
            }

            // m_draw_list.filled_box( box_start, box.first, hovered ? color::white( ) : color::black( ), 0 );
            g_render->text(
                box_start + Vec2( get_padding( ), get_padding( ) ),
                Color::white( ),
                g_fonts->get_default_navbar( ),
                navigation->name.data( ),
                19
            );

            if ( navigation->animation.get_progress( ) != 0.f || !navigation->was_hovered ) {
                //if ( !navigation->was_hovered && !selected )
                g_render->line(
                    box_start + Vec2( get_padding( ) + length, box.second.y + get_padding( ) ),
                    Vec2(
                        box_start.x + underline_length_full + get_padding( ),
                        box_start.y + box.second.y + get_padding( )
                    ),
                    Color::white( ),
                    2.f
                );
                //else
                //    m_draw_list.line( box_start + vec2( get_padding(  ), box.second.y + get_padding(  ) ), vec2( box_start.x + length + get_padding(  ), box_start.y + box.second.y + get_padding(  ) ), color::white( ), 2.f );
            }

            if ( hovered && !m_active_component && m_mouse_state == EMouseState::left_up ) m_selected = i;

            y_offset += button_size.first.y + get_padding( );
            ++i;
        }


        g_render->line(
            m_position + Vec2( navbar_width, 0.f ),
            m_position + Vec2( navbar_width, get_height( ) ),
            colors::outline,
            2.f
        );

        return one_hovered;
    }

    auto c_window::draw_top_bar( ) -> void{
        m_topbar->set_position( m_position );
        m_topbar->set_width( get_with_unscaled( ) );
        m_topbar->draw( );
    }

    auto c_window::centered_text( const std::string_view text, const float y ) -> Vec2{
        const auto size = g_render->get_text_size( text, g_fonts->get_default( ), 16 );
        g_render->text(
            m_position + Vec2(
                ( ( get_width( ) ) / 2.f ) - ( std::ceil( size.x ) / 2.f ),
                y
            ),
            Color::white( ),
            g_fonts->get_default( ),
            text.data( ),
            16
        );
        return size;
    }
}
