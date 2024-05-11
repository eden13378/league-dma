#pragma once
#define NOMINMAX

namespace utils {
    auto enable_debug_privilege( ) -> bool;
    auto get_focused_process_pid( ) -> unsigned long;
    auto start_process( const std::string& path, const std::string& args ) -> bool;

    auto read_file_to_string( const std::string& path ) -> std::expected< std::string, const char* >;

    auto set_thread_name( uint32_t thread_id, const char* thread_name ) -> void;
    auto set_thread_name( std::thread* thread, const char* thread_name ) -> void;
    auto set_thread_name( std::thread::id thread, const char* thread_name ) -> void;

    auto set_current_thread_priority( int priority ) -> bool;
}
