#pragma once
// #include "c_component.hpp"
#include "c_component_base.hpp"
#include "c_topbar.hpp"
#include "../menu_options.hpp"

namespace menu::framework::components {
    class c_topbar_base final : public c_component_base {
        bool m_was_hovered{ };

    public:
        ~c_topbar_base( ) override = default;

        explicit c_topbar_base( )
            : c_component_base( ){
            m_size.y                                     = 16.f;
            m_animation_states[ hover_close_animation ]  = AnimationState( .2f );
            m_animation_states[ topbar_hover_animation ] = AnimationState( .2f );
        }

        auto draw_as_child( ) const -> bool override{ return false; }

        auto get_size( ) const -> Vec2{
            return {
                m_size.x * g_config->misc.screen_scaling->get< float >( ),
                m_size.y * g_config->misc.screen_scaling->get< float >( )
            };
        }

        auto process_input( ) -> bool override;

        auto draw_component( ) -> void override{
            g_render->filled_box( m_position, m_size, colors::outline, 0.f );
            auto topbar_fade = get_animation_state( topbar_hover_animation )->get_progress( );
            if ( !m_has_mouse_down ) topbar_fade = 1.f - topbar_fade;
            g_render->filled_box(
                m_position,
                m_size,
                Color::black( ).alpha( static_cast< int32_t >( 75.f * topbar_fade ) ),
                0.f
            );

            const auto y_center = m_position.y + ( m_size.y / 2.f );

            // g_render->line( vec2( m_position.x + m_size.x - 8 - 4, y_center - 4 ), vec2( m_position.x + m_size.x - 8 + 4, y_center + 4 ), color::white( ), 1.f );
            // g_render->line( vec2( m_position.x + m_size.x - 8 - 4, y_center + 4 ), vec2( m_position.x + m_size.x - 8 + 4, y_center - 5 ), color::white( ), 1.f );

            // const auto hovered = is_hovered( vec2( m_position.x + m_size.x - 16, y_center - 8 ), vec2( 12.f, 12.f ) );

            // if ( hovered || get_animation_state( hover_close_animation )->get_progress( ) != 0.f ) {
            //     auto alpha = static_cast< int32_t >( 75.f * get_animation_state( hover_close_animation )->get_progress( ) );
            //
            //     if ( !hovered ) alpha = 75 - alpha;
            //
            //     g_render->filled_circle(
            //         vec2( m_position.x + m_size.x - 8, y_center ),
            //         color::white( ).alpha( alpha ),
            //         6.f,
            //         30
            //     );
            // }
        }
    };
}
