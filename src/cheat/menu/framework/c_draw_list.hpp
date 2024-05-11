#pragma once
#include <algorithm>
#include <any>
#include <cstdint>
#include <vector>
#include <xstring>

#include "../../include/imgui/imgui.h"
#include "../../renderer/color.hpp"
#include "../../renderer/c_renderer.hpp"
#include "../../sdk/math/vec2.hpp"
#include "../../sdk/math/vec3.hpp"

namespace menu::framework {
    class c_draw_list {
    private:
        enum class EDrawItem {
            box,
            filled_box,
            line,
            text,
            shadow_text,
            circle,
            clip_box,
            texture,
            filled_circle,
            gradient_box
        };

        struct Text {
            std::string text;
            sdk::math::Vec2 position;
            ImFont* font;
            Color color;
            float size;
        };

        struct ClipBox {
            bool reset;
            Vec2 start;
            Vec2 size;
        };

        struct Line {
            sdk::math::Vec2 start;
            sdk::math::Vec2 end;
            Color color;
            float thickness;
        };

        struct Box {
            sdk::math::Vec2 start;
            sdk::math::Vec2 size;
            Color color;
            float thickness;
            float rounding;
        };

        struct Circle {
            sdk::math::Vec2 start;
            Color color;
            float radius;
            int32_t segments;
            float thickness;
        };

        struct DrawItem {
            int32_t z_index;
            EDrawItem type;
            std::any data;
        };

        struct Image {
            std::shared_ptr< Renderer::Texture > texture;
            Vec2 position;
            Vec2 size;
        };

        struct Gradient {
            Vec2 start;
            Vec2 end;
            Color top_left;
            Color top_right;
            Color bottom_right;
            Color bottom_left;
        };

    public:
        auto line( const sdk::math::Vec2 start, const sdk::math::Vec2 end, const Color& clr, const float thickness, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::line, Line( start, end, clr, thickness ) ) );
            m_z_depth++;
        }

        auto box( const sdk::math::Vec2 start, const sdk::math::Vec2 size, const Color& clr, const float rounding = 0.f, const float thickness = 1.f, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::box, Box( start, size, clr, thickness, rounding ) ) );
            m_z_depth++;
        }

        auto filled_box( const sdk::math::Vec2 start, const sdk::math::Vec2 size, const Color& clr, const float rounding = 0.f, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::filled_box, Box( start, size, clr, 0.f, rounding ) ) );
            m_z_depth++;
        }

        auto circle( const sdk::math::Vec2 start, const Color& clr, const float radius, const int32_t segments, float thickness = 1.f, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::circle, Circle( start, clr, radius, segments, thickness ) ) );
            m_z_depth++;
        }

        auto filled_circle( const sdk::math::Vec2 start, const Color& clr, const float radius, const int32_t segments, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::filled_circle, Circle( start, clr, radius, segments ) ) );
            m_z_depth++;
        }

        auto text( const sdk::math::Vec2 position, const Color& color, ImFont* font, std::string_view text, float size, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::text, Text( std::string( text ), position, font, color, size ) ) );
            m_z_depth++;
        }

        auto text_shadow( const sdk::math::Vec2 position, const Color& color, ImFont* font, const char* text, float size, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::shadow_text, Text( std::string( text ), position, font, color, size ) ) );
            m_z_depth++;
        }

        auto render( ) -> void{
            std::ranges::sort(
                m_draw_items,
                []( const DrawItem& first, const DrawItem& second ) -> bool{
                    return first.z_index < second.z_index;
                }
            );
            for ( const auto& item : m_draw_items ) {
                switch ( item.type ) {
                case EDrawItem::box:
                {
                    auto box = std::any_cast< Box >( item.data );
                    g_render->box( box.start, box.size, box.color, box.rounding, box.thickness );
                }
                break;
                case EDrawItem::filled_box:
                {
                    auto box = std::any_cast< Box >( item.data );
                    g_render->filled_box( box.start, box.size, box.color, box.rounding );
                }
                break;
                case EDrawItem::line:
                {
                    auto line = std::any_cast< Line >( item.data );
                    g_render->line( line.start, line.end, line.color, line.thickness );
                }
                break;
                case EDrawItem::text:
                {
                    auto text = std::any_cast< Text >( item.data );
                    g_render->text( text.position, text.color, text.font, text.text.data( ), text.size );
                }
                break;
                case EDrawItem::shadow_text:
                {
                    auto text = std::any_cast< Text >( item.data );
                    g_render->text_shadow( text.position, text.color, text.font, text.text.data( ), text.size );
                }
                break;
                case EDrawItem::circle:
                {
                    auto circle = std::any_cast< Circle >( item.data );
                    g_render->circle( circle.start, circle.color, circle.radius, circle.segments, circle.thickness );
                }
                break;
                case EDrawItem::filled_circle:
                {
                    auto circle = std::any_cast< Circle >( item.data );
                    g_render->filled_circle( circle.start, circle.color, circle.radius, circle.segments );
                }
                break;
                case EDrawItem::clip_box:
                {
                    auto box = std::any_cast< ClipBox >( item.data );

                    if ( box.reset ) g_render->reset_clip_box( );
                    else g_render->set_clip_box( box.start, box.size );
                }
                break;
                case EDrawItem::texture:
                {
                    auto image = std::any_cast< Image >( item.data );

                    g_render->image( image.position, image.size, image.texture );
                }
                break;
                case EDrawItem::gradient_box:
                {
                    auto r = std::any_cast< Gradient >( item.data );

                    g_render->gradient( r.start, r.end, r.top_left, r.top_right, r.bottom_right, r.bottom_left );
                }
                break;
                default: ;
                }
            }
            m_draw_items.clear( );
            m_z_depth = 0;
        }

        auto set_clip_box( const Vec2 start, const Vec2 size, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::clip_box, ClipBox( false, start, size ) ) );
            m_z_depth++;
        }

        auto reset_clip_box( int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::clip_box, ClipBox( true, Vec2( ), Vec2( ) ) ) );
            m_z_depth++;
        }

        auto image( const Vec2 position, const Vec2 size, const std::shared_ptr< Renderer::Texture > texture, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back( DrawItem( z_depth, EDrawItem::texture, Image( texture, position, size ) ) );
            m_z_depth++;
        }

        auto gradient( Vec2 position, const Vec2 size, const Color& color_top_left, const Color& color_top_right, const Color& color_bottom_right, const Color& color_bottom_left, int32_t z_depth = -1 ) -> void{
            if ( z_depth == -1 ) z_depth = m_z_depth;
            m_draw_items.push_back(
                DrawItem(
                    z_depth,
                    EDrawItem::gradient_box,
                    Gradient(
                        position,
                        size,
                        color_top_left,
                        color_top_right,
                        color_bottom_right,
                        color_bottom_left
                    )
                )
            );
            m_z_depth++;
        }

        auto get_z_depth( ) const -> int32_t{
            return m_z_depth;
        }

    private:
        std::vector< DrawItem > m_draw_items;
        int32_t m_z_depth{ };
    };
}
