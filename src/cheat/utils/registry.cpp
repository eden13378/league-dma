#include "pch.hpp"
#include "registry.hpp"


auto registry::get_string_reg_key(
    const HKEY         key,
    const std::string& str_value_name,
    std::string&       str_value,
    const std::string& str_default_value
) -> std::expected< long, ERegistryError >{
try {
    str_value = str_default_value;
    char        sz_buffer[ 512 ];
    DWORD       dw_buffer_size = sizeof sz_buffer;
    const ULONG error          = ( RegQueryValueExA )(
        key,
        str_value_name.data( ),
        nullptr,
        nullptr,
        reinterpret_cast< LPBYTE >( sz_buffer ),
        &dw_buffer_size
    );

    if ( ERROR_SUCCESS == error ) {
        str_value.reserve( strlen( sz_buffer ) );
        str_value = sz_buffer;
    } else return std::unexpected( ERegistryError::none_success_code );

    return error;
} catch ( ... ) {
    return std::unexpected(ERegistryError::unknown);
}
}
