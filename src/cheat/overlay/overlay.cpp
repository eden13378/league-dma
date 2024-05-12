#include "pch.hpp"


#include "overlay.hpp"

#include <stdexcept>

#define NOMINMAX
#include <Windows.h>

#include <d3d11_1.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <dwmapi.h>
#include <tchar.h>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "debug_overlay.hpp"
#include "../config/c_config_system.hpp"
#include <stb_image.hpp>
#include "../include/imgui/imgui.h"
#include "../include/imgui/imgui_impl_dx11.h"
#include "../include/imgui/imgui_impl_win32.h"
// #include "../lua/c_lua.hpp"
#include "../menu/menu.hpp"
#include "../renderer/c_fonts.hpp"
#include "../renderer/c_renderer.hpp"
#include "..\utils\timer.hpp"
#include "../utils/utils.hpp"
#pragma comment(lib, "dwmapi.lib")

#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

// Data
static ID3D11Device*           g_pd3dDevice           = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*         g_pSwapChain           = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool           create_device_d3d( HWND hWnd );
void           cleanup_device_d3d( );
void           create_render_target( );
void           cleanup_render_target( );
LRESULT WINAPI wnd_proc( HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param );

namespace overlay {
    auto start( ) -> void{
        SetProcessDPIAware( );
        SetProcessDpiAwareness( PROCESS_SYSTEM_DPI_AWARE );

        // Create application window
        WNDCLASSEX wc = {
            sizeof( WNDCLASSEX ),
            CS_CLASSDC,
            wnd_proc,
            0L,
            0L,
            GetModuleHandle( nullptr ),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            _T( "  " ),
            nullptr
        };

        const auto size = g_render->get_screensize( );

        ::RegisterClassEx( &wc );
        HWND hwnd = CreateWindowW(
            wc.lpszClassName,
            _T(" "),
            WS_POPUP,
            0,
            0,
            static_cast< int32_t >( size.x ),
            static_cast< int32_t >(size.y),
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        // Initialize Direct3D
        if ( !create_device_d3d( hwnd ) ) {
            cleanup_device_d3d( );
            ::UnregisterClass( wc.lpszClassName, wc.hInstance );
            return;
        }

        SetWindowLongPtr( hwnd, GWL_STYLE, WS_VISIBLE );
        SetWindowLongPtr( hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT );
        // SetWindowPos ( hwnd, HWND_NOTOPMOST, 0, 0, size.x, size.y, SWP_SHOWWINDOW );

        long wstyle = GetWindowLong( hwnd, GWL_EXSTYLE );
        wstyle &= ~( WS_VISIBLE );

        wstyle |= WS_EX_TOOLWINDOW;
        wstyle &= ~( WS_EX_APPWINDOW );

        ShowWindow( hwnd, SW_HIDE );
        SetWindowLong( hwnd, GWL_EXSTYLE, wstyle );
        ShowWindow( hwnd, SW_SHOW );

        MARGINS margins = { -1 }; // With DWM
        DwmExtendFrameIntoClientArea( hwnd, &margins );
        // DwmSetWindowAttribute ( hwnd, DMW )

        // Show the window
        ShowWindow( hwnd, SW_SHOWDEFAULT );
        UpdateWindow( hwnd );

        // Setup Dear ImGui context
        ImGui::CreateContext( );
        ImGuiIO& io = ImGui::GetIO( );
        ( void )io;

        io.IniFilename = nullptr;

        auto& style = ImGui::GetStyle( );

        // Setup Dear ImGui style
        ImGui::StyleColorsDark( );

        style.WindowRounding    = 0.0f;
        style.ChildRounding     = 0.0f;
        style.FrameRounding     = 2.5f;
        style.GrabRounding      = 1.5f;
        style.PopupRounding     = 0.0f;
        style.ScrollbarRounding = 0.0f;

        style.WindowBorderSize = 0.0f;

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init( hwnd );
        ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );


        g_fonts->initialize( );
        // c_font_manager::get( ).initialize( );

        ImVec4 clear_color = ImVec4( 0.f, 0.f, 0.f, 0.f );

        // Main loop
        MSG msg;
        ZeroMemory( &msg, sizeof(msg) );

        static bool last_menu_state = false;

        g_window->initialize( );

        int32_t count = 0;
        while ( msg.message != WM_QUIT && app->should_run( ) ) {
            g_threading->render_thread = std::this_thread::get_id( );
            utils::Timer timer;

            count++;
            SetWindowPos(
                hwnd,
                HWND_TOPMOST,
                0,
                0,
                static_cast< int32_t >( size.x ),
                static_cast< int32_t >( size.y ),
                SWP_SHOWWINDOW
            );

            if ( ::PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) ) {
                TranslateMessage( &msg );
                ::DispatchMessage( &msg );
                continue;
            }

            // make overlay not catch mouse events when menu is closed
            if ( last_menu_state != g_window->is_opened( ) ) {
                long wstyle2 = GetWindowLong( hwnd, GWL_EXSTYLE );
                wstyle2 |= WS_EX_TRANSPARENT;

                if ( g_window->is_opened( ) ) {
                    wstyle2 &= ~WS_EX_TRANSPARENT;
                    SetFocus( nullptr );
                    SetForegroundWindow( nullptr );
                } else {
                    wstyle2 |= WS_EX_TRANSPARENT;

#if __DEBUG
                    if ( app->memory ) {
                        for ( auto ident : app->memory->get_process( )->get_windows( ) ) {
                            SetFocus( ident );
                            SetForegroundWindow( ident );
                        }
                    }
#endif
                }

                ShowWindow( hwnd, SW_HIDE );
                SetWindowLong( hwnd, GWL_EXSTYLE, wstyle2 );
                ShowWindow( hwnd, SW_SHOW );

                last_menu_state = g_window->is_opened( );
            }

            ImGui_ImplDX11_NewFrame( );
            ImGui_ImplWin32_NewFrame( );
            ImGui::NewFrame( );


            g_render->update_draw_list( ImGui::GetBackgroundDrawList( ) );
            // #if enable_lua
            //             // g_lua->on_draw( );
            // #endif
            g_render->on_draw( );
            g_debug_overlay->draw( );
            if ( count % 128 ) {
                for ( auto var : g_config_system->get_bool_vars( ) ) {
                    auto keybind = var->get_keybind( );
                    if ( keybind ) { keybind->on_change = [var]( bool v ) -> void{ var->get< bool >( ) = v; }; }
                }
            }
            if ( g_config_system ) {
                for ( const auto& keybind : g_config_system->get_keybinds( ) ) {
                    if ( typeid( config::keybind_t ).hash_code( ) != keybind->get_config_formatter_ref( )->get( ).
                                                                              type( ).hash_code( ) )
                        continue;

                    if ( keybind ) keybind->get< config::keybind_t >( ).update( );
                }
            }

            ImGui::Render( );

            if ( !io.Fonts->IsBuilt( ) ) io.Fonts->Build( );

            // g_debug_overlay.post_chart_time( _( "Render" ), timer.get_ms_since_start( ).count( ) / 1000000.f );
            // g_debug_overlay.post_function_time( _( "Render" ), timer.get_ms_since_start( ).count( ) );

            g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, nullptr );
            g_pd3dDeviceContext->ClearRenderTargetView(
                g_mainRenderTargetView,
                reinterpret_cast< float* >( &clear_color )
            );

            // auto list = ImGui::GetDrawData( );
            ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
            // if ( !io.Fonts->IsBuilt( ) ) io.Fonts->Build( );

            if ( g_config->misc.limit_overlay_fps->get< bool >( ) ) {
                auto ms = std::chrono::duration_cast< std::chrono::milliseconds >( timer.get_ms_since_start( ) ).
                    count( );

                auto delta = 16 - ms;

                if ( delta > 2 ) std::this_thread::sleep_for( std::chrono::milliseconds( delta ) );
                g_pSwapChain->Present( 0, 0 );
            } else {
                g_pSwapChain->Present( g_config->misc.enable_vsync__->get< bool >( ) ? 1 : 0, 0 ); // Present with vsync
            }
        }

        debug_log( "overlay cleanup" );

        // Cleanup
        ImGui_ImplDX11_Shutdown( );
        ImGui_ImplWin32_Shutdown( );
        ImGui::DestroyContext( );

        cleanup_device_d3d( );
        DestroyWindow( hwnd );
        ::UnregisterClass( wc.lpszClassName, wc.hInstance );
    }

    auto initialize( ) -> void{
        debug_fn_call( )

        // if ( !g_config->misc.use_multi_core_runtime->get< bool >( ) ) { start( ); } else {
            auto render_thread = std::thread( start );
            // utils::set_thread_name( &render_thread, _( "r" ) );
            render_thread.detach( );
        // }
    }

    auto create_texture(
        ID3D11ShaderResourceView** out_srv,
        int                        image_width,
        int                        image_height,
        unsigned char*             image_data
    ) -> bool{
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory( &desc, sizeof(desc) );
        desc.Width            = image_width;
        desc.Height           = image_height;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags   = 0;

        ID3D11Texture2D*       pTexture = nullptr;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem          = image_data;
        subResource.SysMemPitch      = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;

        if ( !g_pd3dDevice ) return false;

        g_pd3dDevice->CreateTexture2D( &desc, &subResource, &pTexture );

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory( &srvDesc, sizeof(srvDesc) );
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView( pTexture, &srvDesc, out_srv );
        if ( !pTexture ) return false;
        pTexture->Release( );

        stbi_image_free( image_data );
        return true;
    }

    auto load_texture_from_file(
        const char*                filename,
        ID3D11ShaderResourceView** out_srv,
        int*                       out_width,
        int*                       out_height
    ) -> bool{
        int32_t        width;
        int32_t        height;
        unsigned char* image_data = stbi_load( filename, &width, &height, nullptr, 4 );
        if ( !image_data ) return false;

        if ( !create_texture( out_srv, width, height, image_data ) ) return false;
        *out_width  = width;
        *out_height = height;

        return true;
    }

    auto load_texture_from_memory(
        const unsigned char*       buffer,
        const int32_t              len,
        ID3D11ShaderResourceView** out_srv,
        int*                       out_width,
        int*                       out_height
    ) -> bool{
        int32_t        width;
        int32_t        height;
        unsigned char* image_data = stbi_load_from_memory( buffer, len, &width, &height, nullptr, 4 );
        if ( !image_data ) return false;

        if ( !create_texture( out_srv, width, height, image_data ) ) return false;
        *out_width  = width;
        *out_height = height;

        return true;
    }
}

bool create_device_d3d( HWND hWnd ){
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount       = 2;
    sd.BufferDesc.Width  = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // sd.BufferDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    sd.BufferDesc.RefreshRate.Numerator   = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;

    sd.SampleDesc.Count   = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed           = TRUE;
    sd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;// D3D11_CREATE_DEVICE_SINGLETHREADED;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL       featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[ 3 ] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    if ( D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        3,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    ) != S_OK )
        return false;

    create_render_target( );
    return true;
}

void cleanup_device_d3d( ){
    cleanup_render_target( );
    if ( g_pSwapChain ) {
        g_pSwapChain->Release( );
        g_pSwapChain = nullptr;
    }
    if ( g_pd3dDeviceContext ) {
        g_pd3dDeviceContext->Release( );
        g_pd3dDeviceContext = nullptr;
    }
    if ( g_pd3dDevice ) {
        g_pd3dDevice->Release( );
        g_pd3dDevice = nullptr;
    }
}

void create_render_target( ){
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
    g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_mainRenderTargetView );
    pBackBuffer->Release( );
}

void cleanup_render_target( ){
    if ( g_mainRenderTargetView ) {
        g_mainRenderTargetView->Release( );
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// Win32 message handler
LRESULT WINAPI wnd_proc( const HWND h_wnd, const UINT msg, WPARAM w_param, const LPARAM l_param ){
    if ( g_windows ) g_windows->wnd_proc_handler( h_wnd, msg, w_param, l_param );

    if ( ImGui_ImplWin32_WndProcHandler( h_wnd, msg, w_param, l_param ) ) return true;

    switch ( msg ) {
    case WM_SIZE:
        if ( g_pd3dDevice != nullptr && w_param != SIZE_MINIMIZED ) {
            cleanup_render_target( );
            g_pSwapChain->ResizeBuffers(
                0,
                ( UINT )LOWORD( l_param ),
                ( UINT )HIWORD( l_param ),
                DXGI_FORMAT_UNKNOWN,
                0
            );
            create_render_target( );
        }
        return 0;
    case WM_SYSCOMMAND:
        if ( ( w_param & 0xfff0 ) == SC_KEYMENU ) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage( 0 );
        return 0;
    }
    return ::DefWindowProc( h_wnd, msg, w_param, l_param );
}
