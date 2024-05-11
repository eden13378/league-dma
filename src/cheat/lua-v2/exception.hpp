#pragma once

#include <stdexcept>
#include <fmt/format.h>

namespace lua {
    class CouldNotLoadScript
        : std::exception {
        [[nodiscard]] const char* what( ) const override{ return m_message.data( ); }

    public:
        explicit CouldNotLoadScript( std::string script )
            : std::exception( ),
            m_message( fmt::format( "could not load script: {}", script ) ){
        }

    private:
        std::string m_message;
    };
}
