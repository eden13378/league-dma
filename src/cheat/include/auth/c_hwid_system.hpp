#pragma once
//
// Created by TORExitNode on 22.04.2020.
//
#include <optional>
#include <string>
#include <windows.h>
#include <nlohmann/json.hpp>


namespace user {
    class c_hwid_system {
    public:
        struct cpu_t {
            std::string name;
            uint16_t cores;
        };

        struct bios_t {
            std::string manufacturer;
            std::string vendor;
            std::string name;
        };

        struct hwid_t {
            cpu_t cpu;
            bios_t bios;
            std::string disk_serial;
            uint64_t memory;

            auto get( ) const -> nlohmann::json ;
        };

        c_hwid_system( ) {
            m_new_hwid = generate_new_hwid( );
        }

        auto get_new_hwid( ) const -> nlohmann::json { return m_new_hwid; }
        auto get_hwid( ) -> hwid_t;

        /**
         * \brief Generate a short form of the hwid from get_hwid()
         * \return hwid in short form
         */
        auto get_hwid_short_form( ) -> std::string;

    private:
        auto generate_new_hwid( ) -> nlohmann::json;

        static auto get_bios_info( ) -> std::optional< bios_t >;
        auto get_cpu_info( ) const -> std::optional< cpu_t >;
        static auto get_disk_serial( ) -> std::string;

        static auto get_mac_address( ) -> std::string;
        [[nodiscard]] auto get_cpu_cores( ) const -> uint32_t;
        [[nodiscard]] auto get_cpu_architecture( ) const -> uint16_t;
        static auto get_installed_memory_amount( ) -> uint64_t;

        nlohmann::json m_new_hwid{ };
        SYSTEM_INFO m_system_info{ };
    };
}
