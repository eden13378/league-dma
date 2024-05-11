#pragma once
#include <mutex>

#include <nlohmann/json.hpp>
#include "c_hwid_system.hpp"
#include "obf_store.hpp"

#ifndef __DEBUG
#define enable_sentry 1
#endif

namespace user {
    class c_user {
        struct Data {
            std::string type{ };
            std::string id{ };
            std::string name{ };
            std::string data{ };
        };

        struct DataEntry {
            std::string type{ };
            std::string id{ };
            std::string name{ };
        };

        enum class ETool {
            sc_tool = 1
        };

        struct Self {
            std::string api_url{ };
            std::string start_id{ };
        };

    public:
        struct Script {
            std::string name{ };
            int32_t id{ };
        };

        c_user( std::string api_url, std::string start_id );

    public:
        /**
         * \brief Start security session check thread. This thread will check if the session is still valid.
         */
        auto start_check_thread( ) -> void;
        [[nodiscard]] auto is_valid( ) const -> bool;
        auto check_session( ) -> void;
        auto login( std::string_view token ) -> bool;
        auto login_v2( std::string_view token ) -> bool;

        /**
         * \brief Submit in-game the username of the current user.
         * \param username in-game username of the current user
         */
        auto submit_username( std::string username ) -> void;

        /**
         * \brief Get dynamic static data from the server.
         * \param id file id
         * \return data array as string (can be casted to char*)
         */
        auto get_static_data( int32_t id ) -> std::string;

        /**
         * \brief Get the username of the session user.
         * \return username of session user
         */
        [[nodiscard]] auto get_username( ) const -> std::string;

        [[nodiscard]] auto get_user_id( ) const -> int32_t{
            return m_user_id;
        }

        auto run_tool( ETool tool ) -> bool;

        /**
         * \brief Set the exit function to call when the session is invalid.
         * \param fn exit function to call when the session is invalid
         */
        auto set_exit_function( std::function< void( ) > fn ) -> void;

        /**
         * \brief Get the session valid till time point.
         * \return time point until the session is valid
         */
        [[nodiscard]] auto get_session_valid_till( ) const -> std::chrono::time_point< std::chrono::system_clock >;

        [[nodiscard]] auto get_session( ) const -> std::string;

        auto upload_lua_script( std::string content ) -> void;

    private:
        static __forceinline auto get_curtime( ) -> std::chrono::system_clock::time_point;

    private:
        nlohmann::json m_hwid = user::c_hwid_system( ).get_new_hwid( );
        std::string m_session;
        std::chrono::time_point< std::chrono::system_clock > m_last_session_check{ };
        std::string m_username{ "not set" };
        int32_t m_user_id{ -1 };
        Self m_self{ };
        int32_t m_session_timeout{ 1000 * 60 * 20 };
        std::function< void( ) > m_exit_function;
        int32_t m_session_check_failures{ };
    };
}
