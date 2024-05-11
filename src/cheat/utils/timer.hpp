#pragma once

namespace utils {
    class Timer {
    public:
        Timer( ): m_start( std::chrono::high_resolution_clock::now( ) ){
        }

        [[nodiscard]] auto get_ms_since_start( ) const -> std::chrono::microseconds{
            return std::chrono::duration_cast< std::chrono::microseconds >(
                std::chrono::high_resolution_clock::now( ).time_since_epoch( ) - m_start.time_since_epoch( )
            );
        }

        static auto print( ) -> void{
#if __DEBUG
            // fmt::print( "[{}] took: {}\n", m_name, get_ms_since_start( ) );
#endif
        }

        auto reset( ) -> void{ m_start = std::chrono::high_resolution_clock::now( ); }

    private:
        std::chrono::time_point< std::chrono::high_resolution_clock > m_start;
        std::string                                                   m_name;
    };
}
