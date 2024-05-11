#include "pch.hpp"

#include "render_manager.hpp"

#include "../globals.hpp"


auto sdk::game::RenderManager::get_view_projection_matrix( D3DXMATRIX* matrix ) const -> void{
    if ( !g_render_matrix ) return;

    static auto last_render_matrix_update = std::chrono::steady_clock::now( );
    const auto  delta                     = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::steady_clock::now( ).time_since_epoch( ) - last_render_matrix_update.time_since_epoch( )
    );

    if ( delta >= std::chrono::microseconds( 750 ) ) {
        g_render_matrix.force_update( );
        last_render_matrix_update = std::chrono::steady_clock::now( );
    }

    const auto _projection_matrix = D3DXMATRIX( g_render_matrix->projection_matrix );

    const auto        up             = D3DXVECTOR3( 0, 0, 1.f );
    const D3DXVECTOR3 cameraPosition = g_pw_hud->camera_position( );
    auto              lookAt         = g_pw_hud->camera_look_at( );
    lookAt.y                         = 0;
    D3DXMATRIX viewMatrix{ };
    D3DXMatrixLookAtLH( &viewMatrix, &cameraPosition, &lookAt, &up );

    D3DXMatrixMultiply( matrix, &viewMatrix, &_projection_matrix );
}

auto sdk::game::RenderManager::get_valid_view_projection_matrix( ) const -> D3DXMATRIX{
    static D3DXMATRIX matrix{ };
    const auto        backup = matrix;

    get_view_projection_matrix( &matrix );

    if ( matrix.m[ 2 ][ 3 ] <= 0.f || matrix.m[ 2 ][ 2 ] <= 0.f ) matrix = backup;

    return matrix;
}
