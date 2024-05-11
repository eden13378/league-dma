#pragma once
#include "c_component.hpp"
#include "../../../renderer/c_renderer.hpp"
#include "../c_window.hpp"

#include "..\..\..\utils\keybind_system.hpp"
#include "../../../renderer/c_fonts.hpp"
#include "../../../config/c_config_system.hpp"

namespace menu::framework::components {
    template <typename t>
    concept numeric_type = requires( t a ) { std::is_arithmetic_v< t >; };

    constexpr auto slider_animation = ct_hash( "slider_animation" );

    constexpr auto slider_height = 6;
    // constexpr auto slider_width = 100;
    constexpr auto slider_position_height = 16;


    template <numeric_type t>
    class c_slider : public c_component {
    public:
        ~c_slider( ) override = default;

        explicit c_slider( std::string label, const t min, const t max, const t step, std::shared_ptr< config::ConfigVar > value )
            : c_component( ),
              m_max( max ),
              m_min( min ),
              m_value( value ),
              m_step( step ){
            m_label = label;
            set_height( 46.f );

            m_animation_states[ slider_animation ] = AnimationState( .5f, AnimationState::ease_in );
        }

        auto process_input( ) -> bool override{
            c_component::process_input( );
            if ( is_right_click_menu_opened( ) ) return true;
            if ( m_parent->is_any_right_click_menu_opened( ) ) return false;
            if ( is_hovered( ) ) {
                if ( !m_was_hovered )
                    get_animation_state( hover_background_animation )->start( );
                m_was_hovered = true;
            } else if ( m_was_hovered ) {
                get_animation_state( hover_background_animation )->start( );
                m_was_hovered = false;
            }

            const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
            const auto slider_position = m_position + Vec2(
                0.f,
                ( get_height( ) - text_height - slider_height ) / 2.f + text_height + m_parent->get_padding( ) - 2.f
            );

            const auto padding = m_parent->get_padding( );

            const auto progress_text = std::is_integral_v< t > ?
                                           std::format( ( "{}" ), m_value->get< t >( ) ) :
                                           std::format( ( "{:.1f}" ), m_value->get< float >( ) );
            const auto progress_text_size = g_render->get_text_size( progress_text, g_fonts->get_default( ), constants::font_size );
            const Vec2 slider_size = progress_text_size + Vec2( padding * 2.f, padding );

            const auto height = get_height( );

            if ( c_component::is_hovered(
                    Vec2( m_position.x, slider_position.y - height / 2.f + 3.f ) + Vec2( 0.f, height / 2.f - slider_size.y / 2.f ),
                    Vec2( get_size(  ).x, slider_size.y )
                )
            ) {
                if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_down ) {
                    m_has_mouse_down = true;
                }
            }

            if ( m_parent->get_mouse_state( ) == c_base_window::EMouseState::left_up ) m_has_mouse_down = false;

            if ( m_has_mouse_down ) {
                const auto delta = std::clamp( std::abs( ( m_position.x + get_size(  ).x ) ) - m_parent->get_cursor_position( ).x, 0.f, get_size(  ).x );
                auto value = m_min + static_cast< t >(
                    std::is_integral< t >::value ?
                        std::round( ( m_max - m_min ) * ( 1.f - delta / get_size(  ).x ) ) :
                        ( m_max - m_min ) * ( 1.f - delta / get_size(  ).x ) );

                if ( m_last_value != value ) {
                    auto closest_delta = std::numeric_limits< t >::max( );
                    auto calculated_value = m_min;
                    
                    auto low = m_min - m_step;
                    auto high = m_max + m_step;

                    for ( auto i = low; i < high; i += m_step ) {
                        auto current_delta = std::abs( value - i );

                        if ( current_delta < closest_delta ) {
                            closest_delta = current_delta;
                            calculated_value = i;
                        }
                    }

                    const auto rounded = std::round( m_last_slider_position );
                    if ( std::abs( std::abs( m_animation_start_slider_offset ) - std::abs( rounded ) ) > 5.f ) {
                        m_animation_start_slider_offset = rounded;
                        get_animation_state( slider_animation )->start( );
                    }
                    m_last_value = calculated_value;
                    m_value->get< t >( ) = calculated_value;
                }
            }

            return false;
        }

        auto draw_component( ) -> void override{
            const auto an_prog = get_animation_state( hover_background_animation )->get_progress( );
            if ( is_hovered( ) || an_prog != 0.f ) {
                auto alpha = static_cast< int32_t >( 50.f * an_prog );
                if ( !is_hovered( ) ) alpha = 50 - alpha;
                g_render->filled_box( m_position, get_size(  ), Color::black( ).alpha( alpha ), 3.f );
            }

            // drawing label
            const auto text_height = g_render->get_text_size( m_label, g_fonts->get_default( ), constants::font_size ).y;
            g_render->text( m_position + Vec2( 0, m_parent->get_padding( ) ), Color::white( ), g_fonts->get_default( ), m_label.data( ), constants::font_size );

            const auto progress_text = std::is_integral_v< t > ?
                                           std::format( "{}", m_value->get< t >( ) ) :
                                           std::format( "{:.1f}", m_value->get< float >( ) );
            const auto progress_text_size = g_render->get_text_size( progress_text, g_fonts->get_default( ), constants::font_size );

            const auto padding = m_parent->get_padding( );

            // calculating progress box size
            const Vec2 slider_size = progress_text_size + Vec2( padding * 2.f, padding );

            const auto slider_position = m_position + Vec2(
                0.f,
                ( get_height( ) - text_height - slider_height ) / 2.f + text_height + m_parent->get_padding( ) - 2.f
            );

            g_render->filled_box( slider_position, Vec2( get_size(  ).x, slider_height ), Color::black( ).alpha( 128 ) );
            const auto current_value_offset = calculate_current_slider_position( slider_size.x - padding );
            const auto height = get_height( );

            // drawing progress text + box
            g_render->filled_box( Vec2( slider_position.x, slider_position.y - height / 2.f + 3.f ) + Vec2( current_value_offset, height / 2.f - slider_size.y / 2.f ), slider_size, Color::black( ) );
            g_render->box( Vec2( slider_position.x, slider_position.y - height / 2.f + 3.f ) + Vec2( current_value_offset, height / 2.f - slider_size.y / 2.f ), slider_size, Color::white( ).alpha( 35 ) );

            g_render->text(
                Vec2( slider_position.x + padding, slider_position.y - height / 2.f + 1 + 3.f ) + Vec2( current_value_offset, height / 2.f - slider_size.y / 2.f ),
                Color::white( ),
                g_fonts->get_default( ),
                progress_text.data( ),
                constants::font_size
            );

            m_last_slider_position = current_value_offset + 6.f;
        }

        //
        // auto is_hovered( ) const -> bool override {
        //     return false;
        // }

        auto get_height( ) const -> float override{
            return get_size(  ).y;
        }

    private:
        auto get_normalized_value( ) const -> t{
            return std::clamp( m_value->get< t >( ), m_min, m_max ) - m_min;
        }

        auto get_steps( ) const -> t{
            return m_max - m_min;
        }

        auto get_pixels_per_step( ) const -> float{
            return get_normalized_value( ) / static_cast< float >( get_steps( ) );
        }

        auto calculate_slider_end_position( float slider_size ) const -> float{
            return get_pixels_per_step( ) * ( get_size(  ).x - slider_size - slider_height / 2.f );
        }

        auto calculate_current_slider_position( float slider_size ) const -> float{
            return std::clamp(
                m_animation_start_slider_offset + ( calculate_slider_end_position( slider_size ) - m_animation_start_slider_offset )
                * get_animation_state( slider_animation )->get_progress( ),
                0.f,
                get_size(  ).x - m_parent->get_padding( ) * 4.f
            );
        }

    private:
        t m_max{ };
        t m_min{ };

    protected:
        std::shared_ptr< config::ConfigVar > m_value{ };

    private:
        t m_step{ };
        float m_animation_start_slider_offset{ 0.f };
        float m_last_slider_position{ 0.f };
        t m_last_value{ };
    };

    class c_slider_float final : public c_slider< float > {
    public:
        c_slider_float( std::string label, const float min, const float max, const float step, std::shared_ptr< config::ConfigVar > config_var )
            : c_slider( label, min, max, step, config_var ){
        }

        auto get_type( ) const -> EComponent override{
            return EComponent::slider_float;
        }

        auto get_value( ) const -> float{
            return m_value->get< float >( );
        }

        auto set_value( const float value ) const -> void{
            m_value->get< float >( ) = value;
        }
    };

    class c_slider_int final : public c_slider< int32_t > {
    public:
        c_slider_int( std::string label, const int32_t min, const int32_t max, const int32_t step, std::shared_ptr< config::ConfigVar > config_var )
            : c_slider( label, min, max, step, config_var ){
        }

        auto get_type( ) const -> EComponent override{
            return EComponent::slider_int;
        }

        auto get_value( ) const -> int32_t{
            return m_value->get< int32_t >( );
        }

        auto set_value( const int32_t value ) const -> void{
            m_value->get< int32_t >( ) = value;
        }
    };
}
