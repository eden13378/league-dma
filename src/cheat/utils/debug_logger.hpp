#pragma once
#include "../build.hpp"

#if enable_debug_logs
#include "../build.hpp"

#if use_fmt
#include <fmt/format.h>
#define debug_fn_call( ) { auto l = std::source_location::current(); auto file = std::string(l.file_name()); auto c = 0; for(auto i=0u; i < file.size(); ++i) { if(file[i] == '\\') c = i; } c++; app->logger->info( fmt::format( "{}:{} calling '{}'", file.substr(c, file.size() - c), l.line(), l.function_name() ) ); }
#define debug_log( f, ... ) 0;
#else
#include <format>
#define debug_fn_call( ) { if( app && app->logger) { auto l = std::source_location::current(); auto file = std::string(l.file_name()); auto c = 0; for(auto i=0u; i < file.size(); ++i) { if(file[i] == '\\') c = i; } c++; app->logger->info( std::format( "{}:{} calling '{}'", file.substr(c, file.size() - c), l.line(), l.function_name() ) ); } }
#define debug_log( f, ... ) if( app && app->logger) app->logger->info( std::format( f, __VA_ARGS__ ) )
#endif

#else
#define debug_log( f, ... ) 
#define debug_fn_call( ) 
#endif
