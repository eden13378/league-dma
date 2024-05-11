#pragma once
#include "c_component.hpp"
#include "../menu_options.hpp"

namespace menu::framework::components {
    constexpr auto hover_close_animation  = ct_hash( "hover_close" );
    constexpr auto topbar_hover_animation = ct_hash( "hover" );

    class Topbar final : public c_component {
        bool m_was_hovered{ };

    public:
        ~Topbar( ) override = default;

        auto get_type( ) const -> EComponent override{ return EComponent::topbar; }

        explicit Topbar( )
            : c_component( ){
            set_height( 16.f );
            m_animation_states[ hover_close_animation ]  = AnimationState( .2f );
            m_animation_states[ topbar_hover_animation ] = AnimationState( .2f );
        }

        explicit Topbar( std::function< void( ) > on_close )
            : Topbar( ){ m_on_close = std::move( on_close ); }

        auto draw_as_child( ) const -> bool override{ return false; }

        auto process_input( ) -> bool override;

        auto draw_component( ) -> void override{
            auto size = get_size( );

            if ( g_config ) size.x *= g_config->misc.screen_scaling->get< float >( );

            g_render->filled_box( m_position, size, colors::outline, 0.f );
            auto topbar_fade = get_animation_state( topbar_hover_animation )->get_progress( );
            if ( !m_has_mouse_down ) topbar_fade = 1.f - topbar_fade;
            g_render->filled_box(
                m_position,
                size,
                Color::black( ).alpha( static_cast< int32_t >( 75.f * topbar_fade ) ),
                0.f
            );

            const auto y_center = m_position.y + ( size.y / 2.f );

            if ( m_on_close ) {
                g_render->line(
                    Vec2( ( m_position.x + size.x - 8.f - 4.f ), ( y_center - 4.f ) ),
                    Vec2( ( m_position.x + size.x - 8.f + 4.f ), ( y_center + 4.f ) ),
                    Color::white( ),
                    1.f
                );
                g_render->line(
                    Vec2( ( m_position.x + size.x - 8.f - 4.f ), ( y_center + 4.f ) ),
                    Vec2( ( m_position.x + size.x - 8.f + 4.f ), ( y_center - 4.f ) ),
                    Color::white( ),
                    1.f
                );

                const auto hovered = c_component::is_hovered(
                    Vec2( m_position.x + size.x - 16, y_center - 8 ),
                    Vec2( 12.f, 12.f )
                );

                if ( hovered || get_animation_state( hover_close_animation )->get_progress( ) != 0.f ) {
                    auto alpha = static_cast< int32_t >( 75.f * get_animation_state( hover_close_animation )->
                        get_progress( ) );

                    if ( !hovered ) alpha = 75 - alpha;

                    g_render->filled_circle(
                        Vec2( m_position.x + size.x - 8, y_center ),
                        Color::white( ).alpha( alpha ),
                        6.f,
                        30
                    );
                }
            }
        }

        auto is_hovered( ) const -> bool override;

    private:
        std::optional< std::function< void( ) > > m_on_close{ };
    };
}
