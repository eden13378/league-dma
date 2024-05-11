#pragma once
#include "color.hpp"
#include "../include/imgui/imgui.h"
#include "../overlay/overlay.hpp"
#include "../sdk/sdk.hpp"
#include "../sdk/math/geometry.hpp"
#include "../sdk/math/math.hpp"

#if enable_lua
#include <sol/forward.hpp>
#endif

#undef min
#undef max


namespace renderer {
    class Renderer {
    public:
        ~Renderer( ) = default;

        struct Texture {
            int32_t                   width{ };
            int32_t                   height{ };
            ID3D11ShaderResourceView* texture{ };
            std::string               name{ };
        };

    public:
        auto update_draw_list( ImDrawList* list ) -> void;
        auto on_draw( ) const -> void;
        auto add_draw_callback( const std::function< void ( ) >& callback ) -> void;
        auto add_post_draw_callback( const std::function< void ( ) >& callback ) -> void;

        auto line(
            Vec2         start,
            Vec2         end,
            const Color& color,
            float        thicknesses
        ) const noexcept -> void;

        auto gradient(
            Vec2         position,
            const Vec2   size,
            const Color& color_top_left,
            const Color& color_top_right,
            const Color& color_bottom_right,
            const Color& color_bottom_left
        ) const noexcept -> void;

        auto box(
            Vec2         position,
            Vec2         size,
            const Color& color,
            float        rounding  = -1.f,
            float        thickness = 1.f
        ) const noexcept -> void;

        auto filled_box(
            const Vec2   position,
            const Vec2   size,
            const Color& color,
            const float  rounding = -1.f
        ) const noexcept -> void;

        auto set_clip_box(
            const Vec2 start,
            const Vec2 size
        ) const noexcept -> void;

        auto reset_clip_box( ) const noexcept -> void;

        auto circle(
            Vec2         position,
            const Color& color,
            float        radius,
            int          segments  = 15,
            float        thickness = 1.f
        ) const noexcept -> void;

        auto triangle(
            Vec2         left,
            Vec2         right,
            Vec2         bottom,
            const Color& color,
            float        thickness = 1.f
        ) const noexcept -> void;

        auto sector(
            const sdk::math::Sector& sector,
            const Color&             color,
            float                    segments  = 15.f,
            float                    thickness = 1.f
        ) const noexcept -> void;

        auto poly_line(
            const ImVec2* points,
            size_t        size,
            const Color&  color,
            float         thickness = 1.f
        ) const noexcept -> void;

        auto polygon(
            const sdk::math::Polygon& polygon,
            const Color&              color,
            float                     thickness = 1.f
        ) const noexcept -> void;

        auto filled_triangle(
            Vec2         left,
            Vec2         right,
            Vec2         bottom,
            const Color& color
        ) const noexcept -> void;

        auto filled_circle(
            Vec2         position,
            const Color& color,
            float        radius,
            int          segments = 15
        ) const noexcept -> void;

        enum E3dCircleFlags: int32_t {
            filled  = 1 << 0,
            outline = 1 << 1
        };

        auto get_3d_circle_points(
            const Vec3& position,
            float       radius,
            int         segments         = 50,
            float       draw_angle       = 360.f,
            Vec3        circle_direction = { }
        ) const noexcept -> std::vector< Vec3 >;

        auto line_3d(
            const Vec3& start,
            const Vec3& end,
            Color       color,
            float       thickness = 1.f
        ) const noexcept -> void;

        auto circle_3d(
            const Vec3& position,
            Color       color,
            const float radius,
            int32_t     flags     = outline,
            int32_t     segments  = -1,
            const float thickness = 1.f,
            float       angle     = 360.f,
            Vec3        direction = { }
        ) const noexcept -> void;

        auto blur_line(Vec2 start, Vec2 end, const Color &color, float thicknesses, int blur_layers = 3, float layer_size = 3.f) const noexcept -> void;

        auto draw_ellipse(
            const Vec3& position,
            float       radius,
            int         segments,
            float       draw_angle
        ) const noexcept -> void;

        auto circle_minimap(
            const Vec3& position,
            Color       color,
            const float radius,
            int32_t     segments  = -1,
            const float thickness = 1.f
        ) const noexcept -> void;

        auto text(
            const Vec2    position,
            const Color&  color,
            const ImFont* font,
            const char*   text,
            float         size
        ) const noexcept -> void;

        auto text_3d(
            const Vec3&   position,
            const Color&  color,
            const ImFont* font,
            const char*   t,
            float         size,
            bool          shadow = false
        ) const noexcept -> void;

        auto get_text_size(
            const std::string_view& text,
            ImFont*                 font,
            float                   text_size
        ) noexcept -> Vec2;

        auto text_shadow(
            Vec2         position,
            const Color& color,
            ImFont*      font,
            const char*  t,
            float        size
        ) const noexcept -> void;

        auto load_texture_from_memory( const std::vector< unsigned char >& data ) noexcept -> std::shared_ptr< Texture >;
        auto load_texture_from_memory( const unsigned int* bytes, int size ) noexcept -> std::shared_ptr< Texture >;
        auto load_texture_from_memory(
            const unsigned int* bytes,
            int                 size,
            hash_t              name
        ) noexcept -> std::shared_ptr< Texture >;

        auto load_texture_from_file( const std::string_view file ) noexcept -> std::shared_ptr< Texture >;
        auto image(
            const Vec2                        position,
            const Vec2                        size,
            const std::shared_ptr< Texture >& texture
        ) const noexcept -> std::expected<void, const char*>;

        auto image( const Vec2 position, const Vec2 size, const Texture* texture ) const noexcept -> std::expected<void,
            const char*>;
        auto get_screensize( ) const noexcept -> sdk::math::Vec2;
        auto rectangle_3d(
            const Vec3& start,
            const Vec3& end,
            float       radius,
            Color       color,
            int32_t     flags         = outline,
            float       thickness     = 1.f,
            bool        outline_front = true
        ) const noexcept -> void;

        auto polygon_3d(
            const class Polygon& polygon,
            Color                color,
            int32_t              flags     = outline,
            float                thickness = 1.f
        ) const noexcept -> void;

        auto load_texture_from_url( std::string url ) noexcept -> std::shared_ptr< Texture >;


#if enable_lua
        auto line_lua(
            const sol::object start,
            const sol::object end,
            const sol::object clr,
            const sol::object thicknesses
        ) const -> void;
        auto box_lua(
            const sol::object position,
            const sol::object size,
            const sol::object clr,
            const sol::object rounding,
            const sol::object thickness
        ) const -> void;
        auto filled_box_lua(
            const sol::object position,
            const sol::object size,
            const sol::object clr,
            const sol::object rounding
        ) const -> void;
        auto circle_lua(
            const sol::object position,
            const sol::object clr,
            const sol::object radius,
            const sol::object segments,
            const sol::object thickness
        ) const -> void;
        auto triangle_lua(
            const sol::object left,
            const sol::object right,
            const sol::object bottom,
            const sol::object clr,
            const sol::object thickness
        ) const -> void;
        auto filled_triangle_lua(
            const sol::object left,
            const sol::object right,
            const sol::object bottom,
            const sol::object clr
        ) const -> void;
        auto filled_circle_lua(
            const sol::object position,
            const sol::object clr,
            const sol::object radius,
            const sol::object segments
        ) const -> void;
        auto lua_line_3d(
            const sol::object start,
            const sol::object end,
            const sol::object clr,
            const sol::object thickness
        ) const -> void;
        auto circle_3d_lua(
            const sol::object position,
            const sol::object clr,
            const sol::object radius,
            const sol::object flags,
            const sol::object segments,
            const sol::object thickness
        ) const -> void;
        auto circle_minimap_lua(
            const sol::object position,
            const sol::object clr,
            const sol::object radius,
            const sol::object segments,
            const sol::object thickness
        ) const -> void;
        auto text_lua(
            const sol::object position,
            const sol::object clr,
            const sol::object text,
            const sol::object font,
            const sol::object size
        ) -> void;
        auto get_text_size_lua(
            const sol::object text,
            const sol::object font,
            const sol::object font_size
        ) -> sol::object;
        auto load_texture_from_file_lua( const sol::object file ) -> std::shared_ptr< Texture >;
        auto image_lua( const sol::object position, const sol::object size, const sol::object texture ) -> void;
        auto get_screensize_lua( ) const -> sol::object;

#endif

    private:
        static auto get_u32( const Color& color ) -> ImU32;

    private:
        ImDrawList*                                    m_draw_list{ nullptr };
        std::vector< std::function< void ( ) > >       m_callbacks{ };
        std::vector< std::function< void ( ) > >       m_post_callbacks{ };
        std::vector< std::shared_ptr< Texture > >      m_textures{ };
        std::mutex                                     m_texture_mutex;
        std::map< hash_t, std::shared_ptr< Texture > > m_texture_cache{ };
    };
}

using namespace renderer;

extern Renderer* g_render;
