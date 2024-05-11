#pragma once
#include <expected>

#include "process.hpp"

#include "../../DMALibrary/Memory/Memory.h"

#include "../../runtime/app.hpp"
#include "../security/src/hash_t.hpp"

#include "../sdk/g_unload.hpp"

#pragma comment(lib, "ntdll.lib")

using u_long = unsigned long;

namespace sdk::memory {
    class Memory {
    public:
        Memory( ) = default;

        explicit Memory( std::string process_name )
            : m_process( std::make_unique< Process >( process_name ) ){
        }

        explicit Memory( std::unique_ptr< Process > process )
            : m_process( std::move( process ) ){
        }

        template <typename t>
        auto read( const intptr_t address, u_long* amount_read = nullptr ) const -> std::expected< t, const char* >{
            try {
                if ( !m_process ) return { };
                t v{ };

                SIZE_T _amount_read{ };

                if ( DMA.Read( address, &v, sizeof( t ) )
                ) {
                    if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                    return v;
                }

                if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                if ( static_cast< size_t >( _amount_read ) != sizeof( t ) ) {
                    return std::unexpected(
                        "memory read error"
                    );
                }

                return v;
            }
            catch ( std::exception& ex ) {
                return std::unexpected( ex.what( ) );
            } catch ( ... ) {
                return std::unexpected( "memory read error" );
            }
        }

        template <typename t>
        auto read( const intptr_t address, t& value_out, u_long* amount_read = nullptr ) const noexcept -> bool{
            try {
                SIZE_T _amount_read;

                DMA.Read( address, &value_out, sizeof( t ) );

                // if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                // if ( static_cast< int >( _amount_read ) == 0 ) return false;

                return true;
            }
            catch ( ... ) {
                return false;
            }
        }

        template <typename t>
        auto read( const intptr_t address, t* value_out, u_long* amount_read = nullptr ) const noexcept -> bool{
            try {
                if ( !m_process ) return false;
                SIZE_T _amount_read;

                if ( !DMA.Read( address, value_out, sizeof( t ) )
                ) {
                    // if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                    return false;
                }

                // if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                // if ( static_cast< size_t >( _amount_read ) != sizeof( t ) ) return false;

                return true;
            }
            catch ( ... ) {
                return false;
            }
        }

        template <typename t>
        auto read_amount(
            const intptr_t address,
            t& out,
            const size_t items_amount,
            u_long* amount_read = nullptr
        ) const noexcept -> bool{
            try {
                SIZE_T _amount_read;

                if ( !DMA.Read( address, &out, sizeof( t ) * items_amount )
                ) {
                    return false;
                }

                // if ( amount_read ) *amount_read = static_cast< unsigned long >( _amount_read );
                // if ( static_cast< size_t >( _amount_read ) != sizeof( t ) ) return false;

                return true;
            }
            catch ( ... ) {
                return false;
            }
        }

        template <typename t>
        auto read_amount(
            const intptr_t address,
            t* out,
            const size_t items_amount,
            u_long* amount_read = nullptr
        ) const noexcept -> bool{
            try {
                SIZE_T _amount_read;
                if ( !DMA.Read( address, out, sizeof( t ) * items_amount )
                ) {
                    return false;
                }

                return true;
            }
            catch ( ... ) {
                return false;
            }
        }

        template <typename t>
        auto write( const uintptr_t address, t value ) const noexcept -> bool{
            try {
                if ( !app->should_run( ) ) return false;
                if ( address == 0 ) return false;

                return NT_SUCCESS(
                    ZwWriteVirtualMemory(
                        m_process->get_handle( ),
                        reinterpret_cast< LPVOID >( address ),
                        &value,
                        sizeof( t ),
                        nullptr
                    )
                );
            }
            catch ( ... ) {
                return false;
            }
        }

        auto write( uintptr_t address, PVOID value, SIZE_T size ) const noexcept -> bool{
            try {
                if ( !app->should_run( ) ) return false;
                if ( address == 0 ) return false;

                DMA.Write( address, value, size );
                return false;
                // return NT_SUCCESS(
                //     ZwWriteVirtualMemory(
                //         m_process->get_handle(),
                //         reinterpret_cast<LPVOID> (address),
                //         value,
                //         size,
                //         nullptr
                //     )
                // );
            }
            catch ( ... ) {
                return false;
            }
        }

        auto get_process( ) const -> Process*{
            return m_process.get( );
        }

    private:
        std::unique_ptr< Process > m_process;
    };
}
