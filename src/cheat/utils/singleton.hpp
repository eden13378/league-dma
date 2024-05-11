#pragma once

namespace utils {
    template <class t>
    class Singleton {
    public:
        static auto inst( ) -> t*{
            static auto instance = std::make_unique< t >( );
            return instance.get( );
        }
    };
}
