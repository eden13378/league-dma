#pragma once

#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>

#include "../security/src/hash_t.hpp"

namespace sdk::memory {
    class Process {
        class Module {
        public:
            Module( )  = default;
            ~Module( ) = default;

            explicit Module( size_t base, size_t size, const hash_t name )
                : m_name( name ){
                m_base = reinterpret_cast< uintptr_t >( reinterpret_cast< void* >( base ) );
                m_size = size;
            }

            [[nodiscard]] auto get_base( ) const -> uintptr_t{ return m_base; }
            [[nodiscard]] auto get_size( ) const -> DWORD{ return m_size; }
            [[nodiscard]] auto get_name( ) const -> hash_t{ return m_name; }

        private:
            uintptr_t m_base;
            DWORD     m_size;
            hash_t    m_name;
        };

    public:
        Process( ) = default;
        explicit Process( const std::string& process_name );

        explicit Process( void* process_handle, const unsigned long pid )
            : m_handle( process_handle ),
            m_pid( pid ){ cache_modules( ); }

        [[nodiscard]] auto get_handle( ) const -> void*{ return m_handle; }
        /**
         * \brief Get pid of process.
         * \return process id
         */
        [[nodiscard]] auto get_pid( ) const -> unsigned long{ return m_pid; }
        /**
         * \brief Cache modules of process.
         */
        auto cache_modules( ) -> void;
        /**
         * \brief Get all modules of process.
         * \return list of all modules
         */
        [[nodiscard]] auto get_modules( ) const -> const std::map< hash_t, std::shared_ptr< Module > >&{
            return m_modules;
        }

        auto get_windows( ) const -> std::vector< HWND >{ return m_windows; }

        /**
         * \brief Try to inject dll in the remote process.
         * \param path dll path
         * \return whether injection was successful or not.
         */
        auto inject_dll( const char* path ) const -> bool;

        [[nodiscard]] auto is_running( ) const -> bool;

        /**
        * \brief Get process module data by name.
        * \param name module name
        * \return process module if found
        */
        auto get_module( const hash_t name ) const -> std::shared_ptr< Module >{
            const auto found = m_modules.find( name );

            // check if module is valid
            if ( found == m_modules.end( ) ) return nullptr;

            return found->second;
        }

        /* -- operators -- */
        explicit operator bool( ) const{ return !!m_handle; }

    private:
        static auto utf8_encode( const std::wstring& wstr ) -> std::string{
            if ( wstr.empty( ) ) return { };
            const int size_needed = WideCharToMultiByte(
                CP_UTF8,
                0,
                &wstr[ 0 ],
                static_cast< int >( wstr.size( ) ),
                nullptr,
                0,
                nullptr,
                nullptr
            );
            std::string str_to( size_needed, 0 );
            WideCharToMultiByte(
                CP_UTF8,
                0,
                &wstr[ 0 ],
                static_cast< int >( wstr.size( ) ),
                &str_to[ 0 ],
                size_needed,
                nullptr,
                nullptr
            );
            return str_to;
        };

    private:
        void*                                         m_handle{ };
        unsigned long                                 m_pid{ };
        std::vector< HWND >                           m_windows{ };
        std::map< hash_t, std::shared_ptr< Module > > m_modules;
        std::string m_process_name{};
    };
}
