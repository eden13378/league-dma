#pragma once

#include "../../include/clipper/clipper.hpp"
#include "vec3.hpp"

constexpr auto circle_line_segment_n = 16;

namespace sdk::math {
    class Polygon {
    public:
        std::vector< Vec3 > points;

        auto add( const Vec3& point ) -> void{ points.push_back( point ); }

#if enable_lua
        auto add_lua( const sol::object point ) -> void;
#endif

        auto is_inside( const Vec3& point ) -> bool{ return !is_outside( point ); }

        auto is_outside( const Vec3& point ) const -> bool{
            const auto p = ClipperLib::IntPoint(
                static_cast< long long >( point.x ),
                static_cast< long long >( point.z )
            );
            return PointInPolygon( p, to_clipper_path( ) ) != 1;
        }

        auto point_in_polygon( const Vec3& point ) -> int{
            const auto p = ClipperLib::IntPoint(
                static_cast< long long >( point.x ),
                static_cast< long long >( point.z )
            );
            return PointInPolygon( p, to_clipper_path( ) );
        }

#if enable_lua
        auto point_in_polygon_lua( const sol::object point ) -> void;
#endif

        auto to_clipper_path( ) const -> std::vector< ClipperLib::IntPoint >{
            std::vector< ClipperLib::IntPoint > result;

            for ( const auto& point : points ) {
                result.emplace_back(
                    static_cast< long long >( point.x ),
                    static_cast< long long >( point.z )
                );
            }

            return result;
        }
    };

    class Arc {
    public:
        Vec3  end;
        int   hitbox;
        Vec3  start;
        float distance;

        Arc( const Vec3& start, const Vec3& end, const int hitbox )
            : end( end ),
            hitbox( hitbox ),
            start( start ),
            distance( start.dist_to( end ) ){
        }

        static auto circle_circle_intersection(
            const Vec3& center1,
            const Vec3& center2,
            const float radius1,
            const float radius2
        ) -> std::vector< Vec3 >{
            std::vector< Vec3 > result;

            const auto d = center1.dist_to( center2 );

            if ( d > radius1 + radius2 || d <= abs( radius1 - radius2 ) ) return result;

            const auto a         = ( radius1 * radius1 - radius2 * radius2 + d * d ) / ( 2 * d );
            const auto h         = sqrt( radius1 * radius1 - a * a );
            const auto direction = ( center2 - center1 ).normalize( );
            const auto pa        = center1 + direction * a;
            const auto s1        = pa + direction.perpendicular( ) * h;
            const auto s2        = pa - direction.perpendicular( ) * h;

            result.push_back( s1 );
            result.push_back( s2 );
            return result;
        }

        auto to_polygon( int offset = 0 ) const -> Polygon{
            offset += hitbox;
            auto result = Polygon( );

            const auto inner_radius = -0.1562f * distance + 687.31f;
            auto       outer_radius = 0.35256f * distance + 133.0f;

            outer_radius = outer_radius / cos( 2 * m_pi / circle_line_segment_n );

            const auto inner_centers = circle_circle_intersection( start, end, inner_radius, inner_radius );
            const auto outer_centers = circle_circle_intersection( start, end, outer_radius, outer_radius );

            const auto inner_center = inner_centers[ 0 ];
            const auto outer_center = outer_centers[ 0 ];

            auto direction = ( end - outer_center ).normalize( );
            auto end       = ( start - outer_center ).normalize( );
            auto max_angle = direction.angle_between( end ) * m_pi / 180.0f;

            auto step = -max_angle / circle_line_segment_n;

            for ( auto i = 0; i < circle_line_segment_n; i++ ) {
                const auto angle = step * i;
                auto       point = outer_center + direction.rotated( angle ) * ( outer_radius + 15 + static_cast<
                    float >( offset ) );
                result.add( point );
            }

            direction = ( start - inner_center ).normalize( );
            end       = ( end - inner_center ).normalize( );
            max_angle = direction.angle_between( end ) * m_pi / 180.0f;
            step      = max_angle / circle_line_segment_n;

            for ( auto i = 0; i < circle_line_segment_n; i++ ) {
                const auto angle = step * i;
                auto       point = inner_center + direction.rotated( angle ) * fmax(
                    0.f,
                    inner_radius - static_cast< float >( offset ) - 20.f
                );
                result.add( point );
            }

            return result;
        }
    };

    class Circle {
    public:
        Vec3  center;
        float radius;

        Circle( const Vec3& center, const float radius )
            : center( center ),
            radius( radius ){
        }

        auto to_polygon( const int offset = 0, const float override_width = -1, const int segments = 0
        ) const -> Polygon{
            auto       result           = Polygon( );
            const auto circle_line_segs = segments != 0 ?
                                              segments :
                                              circle_line_segment_n;
            const auto out_radius = override_width > 0 ?
                                        override_width :
                                        ( static_cast< long double >( offset ) + radius ) / cos(
                                            2 * m_pi / static_cast< long double >(
                                                circle_line_segs
                                            )
                                        );

            const double step  = 2 * m_pi / static_cast< double >( circle_line_segs );
            double       angle = radius;
            for ( auto i = 0; i <= circle_line_segs; i++ ) {
                angle += step;
                auto point = Vec3(
                    center.x + static_cast< float >( out_radius ) * static_cast< float >( cos( angle ) ),
                    this->center.y,
                    center.z + static_cast< float >( out_radius ) * static_cast< float >( sin( angle ) )
                );
                result.add( point );
            }

            return result;
        }
    };

    class Rectangle {
    public:
        Vec3  direction;
        Vec3  perpendicular;
        Vec3  r_end;
        Vec3  r_start;
        float width;

        Rectangle( const Vec3& start, const Vec3& end, const float width_start ){
            r_start       = start;
            r_end         = end;
            width         = width_start;
            direction     = ( end - start ).normalize( );
            perpendicular = direction.perpendicular( );
        }

        auto to_polygon( const int offset = 0, const float override_width = -1 ) const -> Polygon{
            auto result = Polygon( );

            result.add(
                r_start +
                perpendicular * ( override_width > 0.f ?
                                      override_width :
                                      width + static_cast< float >( offset ) ) -
                direction * static_cast< float >( offset )
            );
            result.add(
                r_start -
                perpendicular * ( override_width > 0.f ?
                                      override_width :
                                      width + static_cast< float >( offset ) ) -
                direction * static_cast< float >( offset )
            );
            result.add(
                r_end -
                perpendicular * ( override_width > 0.f ?
                                      override_width :
                                      width + static_cast< float >( offset ) ) +
                direction * static_cast< float >( offset )
            );
            result.add(
                r_end +
                perpendicular * ( override_width > 0.f ?
                                      override_width :
                                      width + static_cast< float >( offset ) ) +
                direction * static_cast< float >( offset )
            );

            return result;
        }

        auto intersection( const Circle& circle ) const -> std::vector< Vec3 >{
            float t;

            const auto dx = r_end.x - r_start.x;
            const auto dy = r_end.z - r_start.z;

            const auto a = dx * dx + dy * dy;
            const auto b = 2 * ( dx * ( r_start.x - circle.center.x ) + dy * ( r_start.z - circle.center.z ) );
            const auto c = ( r_start.x - circle.center.x ) * ( r_start.x - circle.center.x ) +
                ( r_start.z - circle.center.z ) * ( r_start.z - circle.center.z ) -
                circle.radius * circle.radius;
            const auto det = b * b - 4 * a * c;

            if ( abs( a ) <= FLT_EPSILON || det < 0 ) return { };

            if ( det == 0 ) {
                t = -b / ( 2 * a );
                return std::vector( { Vec3( r_start.x + t * dx, 0.f, r_start.z + t * dy ) } );
            }

            t         = ( -b + sqrt( det ) ) / ( 2 * a );
            auto int1 = Vec3( r_start.x + t * dx, 0.f, r_start.z + t * dy );
            t         = ( -b - sqrt( det ) ) / ( 2 * a );

            return { int1, Vec3( r_start.x + t * dx, 0.f, r_start.z + t * dy ) };
        }
    };

    class Ring {
    public:
        Vec3  center;
        float radius;
        float ring_radius;

        Ring( const Vec3& center, const float radius, const float ring_radius )
            : center( center ),
            radius( radius ),
            ring_radius( ring_radius ){
        }

        auto to_polygon( const int offset = 0 ) const -> Polygon{
            Polygon result;

            const auto out_radius = ( static_cast< float >( offset ) +
                this->radius + this->ring_radius ) / cosf(
                2 * m_pi / circle_line_segment_n
            );
            const auto inner_radius = this->radius - this->ring_radius - static_cast< float >( offset );

            for ( auto i = 0; i <= circle_line_segment_n; i++ ) {
                const auto angle = static_cast< float >( i ) * 2 * m_pi / circle_line_segment_n;
                auto       point = Vec3(
                    this->center.x - out_radius * cosf( angle ),
                    this->center.y,
                    this->center.z - out_radius * sinf( angle )
                );

                result.add( point );
            }
            for ( auto i = 0; i <= circle_line_segment_n; i++ ) {
                const auto angle = static_cast< float >( i ) * 2 * m_pi / circle_line_segment_n;
                auto       point = Vec3(
                    this->center.x - inner_radius * cosf( angle ),
                    this->center.y,
                    this->center.z - inner_radius * sinf( angle )
                );

                result.add( point );
            }
            return result;
        }
    };

    class Sector {
    public:
        float angle;
        Vec3  center;
        Vec3  direction;
        float radius;

        Sector( const Vec3& center, const Vec3& direction, const float angle, const float radius )
            : angle( angle ),
            center( center ),
            direction( direction ),
            radius( radius ){
        }

        auto lua_to_polygon_new( const int offset ) const -> Polygon{ return to_polygon_new( offset ); }

        auto to_polygon_new( const float offset ) const -> Polygon{
            return to_polygon_new( static_cast< int32_t >( offset ) );
        }

        auto to_polygon_new( const int32_t offset = 0 ) const -> Polygon{
            auto result = Polygon( );
            result.add( center );
            const auto max_target = center.extend( direction, radius + static_cast< float >( offset ) );

            const auto dx = max_target.x - center.x;
            const auto dz = max_target.z - center.z;

            // debug_log( "offset [{}, {}]", dx, dz );

            // (624.9607 / 650) * 57.2957795131
            // 1.29232946 * 57.2957795131

            auto angle_deg = asin( fabs( dz ) / ( radius + static_cast< float >( offset ) ) ) * ( 180.f / m_pi );

            if ( dx >= 0.f ) {
                if ( dz < 0.f ) {
                    // +, -
                    angle_deg = 90.f + angle_deg;
                } else {
                    // +, +
                    angle_deg = 90.f - angle_deg;
                }
            } else {
                if ( dz >= 0.f ) {
                    // -, +
                    angle_deg = 270.f + angle_deg;
                } else {
                    // -, -
                    angle_deg = 270.f - angle_deg;
                }
            }
            const auto min_angle_deg = angle_deg - ( angle / 2.f );
            //
            const auto angle_step = angle / circle_line_segment_n;
            for ( auto i = 0; i <= circle_line_segment_n; i++ ) {
                const auto angle_rad = ( min_angle_deg + ( angle_step * static_cast< float >( i ) ) ) * ( m_pi /
                    180.f );
                result.add(
                    Vec3(
                        center.x + sin( angle_rad ) * ( radius + static_cast< float >( offset ) ),
                        center.y,
                        center.z + cos( angle_rad ) * ( radius + static_cast< float >( offset ) )
                    )
                );
            }

            return result;
        }

        auto to_polygon( const int offset = 0 ) const -> Polygon{
            auto       result     = Polygon( );
            const auto out_radius = ( radius + static_cast< float >( offset ) ) / cos(
                2 * m_pi / circle_line_segment_n
            );

            result.add( center );
            const auto side1 = direction.rotated( -angle * 0.5f );

            for ( auto i = 0; i <= circle_line_segment_n; i++ ) {
                const auto direction = side1.rotated(
                    static_cast< float >( i ) * angle / circle_line_segment_n
                ).normalize( );
                result.add(
                    Vec3(
                        center.x + out_radius * direction.x,
                        center.y,
                        center.z + out_radius * direction.z
                    )
                );
            }

            return result;
        }

        auto contains( Vec3 world_pos ) const -> bool{ return false; }
    };

    class Geometry {
    public:
        static auto clip_polygons( const std::vector< Polygon >& polygons
        ) -> std::vector< std::vector< ClipperLib::IntPoint > >{
            std::vector< std::vector< ClipperLib::IntPoint > > subj( polygons.size( ) );
            std::vector< std::vector< ClipperLib::IntPoint > > clip( polygons.size( ) );

            for ( auto polygon : polygons ) {
                subj.push_back( polygon.to_clipper_path( ) );
                clip.push_back( polygon.to_clipper_path( ) );
            }

            auto                solution = std::vector< std::vector< ClipperLib::IntPoint > >( );
            ClipperLib::Clipper c;
            c.AddPaths( subj, ClipperLib::PolyType::ptSubject, true );
            c.AddPaths( clip, ClipperLib::PolyType::ptClip, true );
            c.Execute(
                ClipperLib::ClipType::ctUnion,
                solution,
                ClipperLib::PolyFillType::pftNegative,
                ClipperLib::PolyFillType::pftNonZero
            );

            return solution;
        }

        static auto cut_path( std::vector< Vec3 > path, const float distance ) -> std::vector< Vec3 >{
            std::vector< Vec3 > result;
            auto                l_distance = distance;

            if ( distance < 0 ) {
                path[ 0 ] = path[ 0 ] + ( path[ 1 ] - path[ 0 ] ).normalize( ) * distance;
                return path;
            }

            for ( auto i = 0u; i < path.size( ) - 1; i++ ) {
                const auto dist = path[ i ].dist_to( path[ i + 1 ] );
                if ( dist > l_distance ) {
                    result.push_back( path[ i ] + ( path[ i + 1 ] - path[ i ] ).normalize( ) * l_distance );
                    for ( auto j = i + 1u; j < path.size( ); j++ ) result.push_back( path[ j ] );
                    break;
                }
                l_distance -= dist;
            }
            return !result.empty( ) ?
                       result :
                       std::vector{ path.back( ) };
        }

        static auto path_length( const std::vector< Vec3 >& path ) -> float{
            auto distance = 0.0f;
            for ( auto i = 0u; i < path.size( ) - 1; i++ ) distance += path[ i ].dist_to( path[ i + 1 ] );
            return distance;
        }

        static auto vector_movement_collision(
            const Vec3& start_point1,
            const Vec3& end_point1,
            const float v1,
            const Vec3& start_point2,
            const float v2,
            float& t1,
            const float delay = 0.f
        ) -> Vec3{
            const auto s_p1_x = start_point1.x;
            const auto s_p1_y = start_point1.z;
            const auto e_p1_x = end_point1.x;
            const auto e_p1_y = end_point1.z;
            const auto s_p2_x = start_point2.x;
            const auto s_p2_y = start_point2.z;

            const auto d    = e_p1_x - s_p1_x, e = e_p1_y - s_p1_y;
            const auto dist = sqrt( d * d + e * e );
            t1              = std::numeric_limits< float >::infinity( );;
            const auto s    = abs( dist ) > FLT_EPSILON ?
                                  v1 * d / dist :
                                  0.f,
                       k = ( abs( dist ) > FLT_EPSILON ) ?
                               v1 * e / dist :
                               0.f;

            const auto r = s_p2_x - s_p1_x, j = s_p2_y - s_p1_y;
            const auto c = r * r + j * j;

            if ( dist > 0.f ) {
                if ( abs( v1 - FLT_MAX ) < FLT_EPSILON ) {
                    const auto t = dist / v1;
                    t1           = v2 * t >= 0.f ?
                                       t :
                                       std::numeric_limits< float >::infinity( );
                } else if ( abs( v2 - FLT_MAX ) < FLT_EPSILON ) t1 = 0.f;
                else {
                    const auto a = s * s + k * k - v2 * v2, b = -r * s - j * k;

                    if ( abs( a ) < FLT_EPSILON ) {
                        if ( abs( b ) < FLT_EPSILON ) {
                            t1 = ( abs( c ) < FLT_EPSILON ) ?
                                     0.f :
                                     std::numeric_limits< float >::infinity( );
                        } else {
                            const auto t = -c / ( 2 * b );
                            t1           = ( v2 * t >= 0.f ) ?
                                               t :
                                               std::numeric_limits< float >::infinity( );
                        }
                    } else {
                        const auto sqr = b * b - a * c;
                        if ( sqr >= 0 ) {
                            const auto nom = sqrt( sqr );
                            auto       t   = ( -nom - b ) / a;
                            t1             = v2 * t >= 0.f ?
                                                 t :
                                                 std::numeric_limits< float >::infinity( );
                            t             = ( nom - b ) / a;
                            const auto t2 = ( v2 * t >= 0.f ) ?
                                                t :
                                                std::numeric_limits< float >::infinity( );

                            if ( !isnan( t2 ) && !isnan( t1 ) ) {
                                if ( t1 >= delay && t2 >= delay ) t1 = fmin( t1, t2 );
                                else if ( t2 >= delay ) t1 = t2;
                            }
                        }
                    }
                }
            } else if ( abs( dist ) < FLT_EPSILON ) t1 = 0.f;

            return ( !isnan( t1 ) ) ?
                       Vec3( s_p1_x + s * t1, 0, s_p1_y + k * t1 ) :
                       Vec3( 0, 0, 0 );
        }

        static auto position_after(
            const std::vector< Vec3 >& self,
            const float t,
            const int speed,
            const float                                        delay = 0
        ) -> Vec3{
            auto distance = static_cast< int32_t >( fmax( 0, t - delay ) ) * speed / 1000;

            for ( size_t i = 0u; i <= self.size( ) - 2; i++ ) {
                auto       from = self[ i ];
                auto       to   = self[ i + 1 ];
                const auto d    = static_cast< int >( to.dist_to( from ) );
                if ( d > distance ) return from + ( to - from ).normalize( ) * static_cast< float >( distance );
                distance -= d;
            }

            return self[ self.size( ) - 1 ];
        }

        static auto to_polygon( const std::vector< ClipperLib::IntPoint >& v ) -> Polygon{
            auto p = Polygon( );
            for ( const auto point : v ) {
                p.add(
                    Vec3( static_cast< float >( point.X ), 0.f, static_cast< float >( point.Y ) )
                );
            }
            return p;
        }

        static auto to_polygons(
            const std::vector< std::vector< ClipperLib::IntPoint > >& v
        ) -> std::vector< Polygon >{
            std::vector< Polygon > result;
            for ( const auto item : v ) result.push_back( to_polygon( item ) );
            return result;
        }
    };
}
