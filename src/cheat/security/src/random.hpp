﻿#pragma once

namespace xbt::security::random {
    constexpr auto time = __TIME__;
    constexpr auto seed = static_cast< unsigned >( time[ 7 ] ) + static_cast< unsigned >( time[ 6 ] ) * 10 +
        static_cast< unsigned >( time[ 4 ] ) * 60 +
        static_cast< unsigned >( time[ 3 ] ) * 600 + static_cast< unsigned >( time[ 1 ] ) * 3600 +
        static_cast< unsigned >( time[ 0 ] ) * 36000;

    template < int n >
    struct generator_t {
    private:
        static constexpr unsigned a = 16807;
        static constexpr unsigned m = 2147483647;

        static constexpr unsigned s = generator_t< n - 1 >::value;
        static constexpr unsigned lo = a * ( s & 0xFFFFu );
        static constexpr unsigned hi = a * ( s >> 16u );
        static constexpr unsigned lo2 = lo + ( ( hi & 0x7FFFu ) << 16u );
        static constexpr unsigned hi2 = hi >> 15u;
        static constexpr unsigned lo3 = lo2 + hi;

    public:
        static constexpr unsigned max = m;
        static constexpr unsigned value = lo3 > m ?
                                              lo3 - m :
                                              lo3;
    };

    template <>
    struct generator_t< 0 > {
        static constexpr unsigned value = seed;
    };

    template < int n, int m >
    struct _int {
        static constexpr auto value = generator_t< n + 1 >::value % m;
    };

    template < int n >
    struct _char {
        static const char value = static_cast< char >( 1 + _int< n, 0x7F - 1 >::value );
    };
}
