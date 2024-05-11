#include "pch.hpp"
#include "../state.hpp"
#include "../../menu/framework/components/c_checkbox.hpp"
#include "../../menu/framework/components/c_select.hpp"
#include "../../menu/framework/components/c_slider.hpp"
#include "../menu/menu.hpp"

namespace lua {
        auto LuaState::register_menu( sol::state& state ) -> void{
        state.new_usertype< menu::framework::components::c_checkbox >(
            "c_checkbox",
            "get_value",
            sol::resolve( &menu::framework::components::c_checkbox::get_value ),
            "set_value",
            sol::resolve( &menu::framework::components::c_checkbox::set_value ),
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_select >(
            "c_select",
            "get_value",
            sol::resolve( &menu::framework::components::c_select::get_value ),
            "set_value",
            sol::resolve( &menu::framework::components::c_select::set_value ),
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_multi_select >(
            "c_multi_select",
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_slider_int >(
            "c_slider_int",
            "get_value",
            sol::resolve( &menu::framework::components::c_slider_int::get_value ),
            "set_value",
            sol::resolve( &menu::framework::components::c_slider_int::set_value ),
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_slider_float >(
            "c_slider_float",
            "get_value",
            sol::resolve( &menu::framework::components::c_slider_float::get_value ),
            "set_value",
            sol::resolve( &menu::framework::components::c_slider_float::set_value ),
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_button >(
            "c_button",
            "set_tooltip",
            sol::resolve( &menu::framework::components::c_checkbox::set_tooltip )
        );
        state.new_usertype< menu::framework::components::c_conditional >( "c_conditional" );

        state.new_usertype< menu::framework::c_window::section_t >(
            "section_t",
            "get_height",
            sol::resolve( &menu::framework::c_window::section_t::get_height ),
            "is_hovered",
            sol::resolve( &menu::framework::c_window::section_t::is_hovered ),
            "name",
            sol::readonly( &menu::framework::c_window::section_t::name ),
            "checkbox",
            sol::resolve( &menu::framework::c_window::section_t::lua_checkbox ),
            "select",
            sol::resolve( &menu::framework::c_window::section_t::lua_select ),
            "multi_select",
            sol::resolve( &menu::framework::c_window::section_t::lua_multi_select ),
            "slider_int",
            sol::resolve( &menu::framework::c_window::section_t::lua_slider_int ),
            "slider_float",
            sol::resolve( &menu::framework::c_window::section_t::lua_slider_float ),
            "button",
            sol::resolve( &menu::framework::c_window::section_t::lua_button ),
            "add_conditional",
            sol::resolve( &menu::framework::c_window::section_t::lua_add_conditional ),
            "find_child",
            sol::resolve( &menu::framework::c_window::section_t::lua_get_child )
        );

        state.new_usertype< menu::framework::c_window::navigation_t >(
            "navigation_t",
            sol::constructors( ),
            "add_section",
            sol::resolve( &menu::framework::c_window::navigation_t::lua_add_section ),
            "find_section",
            sol::resolve( &menu::framework::c_window::navigation_t::lua_get_section )
        );

        state.new_usertype< menu::framework::c_window >(
            "c_window",
            "push_navigation",
            sol::resolve( &menu::framework::c_window::lua_push ),
            "find_navigation",
            sol::resolve( &menu::framework::c_window::lua_get_navigation ),
            "get_height",
            sol::resolve( &menu::framework::c_window::get_height ),
            "get_width",
            sol::resolve( &menu::framework::c_window::get_width ),
            "is_opened",
            sol::resolve( &menu::framework::c_window::is_opened ),
            "get_name",
            sol::resolve( &menu::framework::c_window::get_name ),
            "set_position",
            sol::resolve( &menu::framework::c_window::set_position )
        );

        state.create_named_table(
            "menu",
            "get_main_window",
            []( ) -> menu::framework::c_window* { return g_window; }
        );
    }

}
