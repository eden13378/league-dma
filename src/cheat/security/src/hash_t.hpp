#pragma once

// this specific struct is taken from ducarii, but its everywhere on the internet (with same prime and basics as ducarii used)

using hash_t = uint32_t;

namespace xbt::security::hash {
    constexpr uint64_t basis = 0x811c9dc5;
    constexpr uint64_t prime = 0x1000193;

    /// <summary>
    /// Creates hash of text during compile-time
    /// </summary>
    /// <param name="txt">The text that is going to be hashed</param>
    /// <param name="value">The current hash value</param>
    /// <returns>Hashed text</returns>
    constexpr auto get_const( const char* txt, const hash_t value = basis ) noexcept -> hash_t {
        // Recursive hashing
        return txt[ 0 ] == '\0' ?
                   value :
                   get_const( &txt[ 1 ], ( value ^ hash_t( txt[ 0 ] ) ) * prime );
    }

    /// <summary>
    /// Creates hash of text during run-time
    /// </summary>
    /// <param name="txt">The text that is going to be hashed</param>
    /// <returns>Hashed text</returns>
    inline auto get( const char* txt ) -> hash_t {
        hash_t ret = basis;

        const auto length = strlen( txt );
        for ( auto i = 0u; i < length; ++i ) {
            /// OR character and multiply it with fnv1a prime
            ret ^= txt[ i ];
            ret *= prime;
        }

        return ret;
    }
}

/// <summary>
/// Creates hash of text during compile-time
/// </summary>
/// <param name="str">The text that is going to be hashed</param>
/// <returns>Hashed text</returns>
#define ct_hash( string ) \
[ ]( ) { \
constexpr hash_t ret = xbt::security::hash::get_const( string ); \
return ret; \
}( )

/// <summary>
/// Creates hash of text during run-time
/// </summary>
/// <param name="str">The text that is going to be hashed</param>
/// <returns>Hashed text</returns>
#define rt_hash( string ) xbt::security::hash::get( string )
