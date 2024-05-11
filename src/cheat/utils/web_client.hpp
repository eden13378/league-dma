#pragma once

class WebClient {
public:
    [[nodiscard]] auto               get( std::string url ) const -> std::optional< std::string >;
    [[nodiscard]] auto post( std::string url, std::string body ) const -> std::optional< std::string >;

#if enable_lua
    auto lua_get( const sol::object url ) -> sol::object;
    auto lua_post( const sol::object url, const sol::object body ) -> sol::object;
    auto lua_set_timeout( const sol::object timeout ) -> void;
    auto lua_add_default_header( const sol::object name, const sol::object content ) -> void;
#endif

    auto add_default_header( std::string name, std::string content ) -> void;
    auto set_timeout( const int32_t timeout ) -> void{ m_timeout = timeout; }

private:
    int32_t                                              m_timeout{ 10 };
    std::vector< std::pair< std::string, std::string > > m_default_headers{ };
};
