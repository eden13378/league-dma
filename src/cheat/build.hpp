#pragma once

#define use_fmt false
// #define _ITERATOR_DEBUG_LEVEL 0
#define profiling_overlay 0
#define lua_callback_debug 0

#define debug_overlay 0
#define enable_auth 0
#define enable_debug_logs 1
#define enable_one_thread_mode 1
#define enable_new_lua 0


#define enable_function_caller 1
// enable possibly unsafe internal features (debug only)
#define function_caller_experimental 0
#define function_caller_debug_log 1

/* feature macros */
#ifndef enable_lua
#define enable_lua 0
#endif
#define enable_sentry 1
#define enable_lua_decryption 1


// automatically disable sentry in debug
#if __DEBUG
#undef enable_sentry
#define enable_sentry 0
#else
#undef enable_function_caller
#define enable_function_caller 1
#undef function_caller_experimental
#define function_caller_experimental 0
#undef function_caller_debug_log
#define function_caller_debug_log 0
#undef debug_overlay
#define debug_overlay 0
#undef enable_auth
#define enable_auth 1
#undef enable_lua
#define enable_lua 0
#undef enable_one_thread_mode
#define enable_one_thread_mode 1
#undef enable_debug_logs
#define enable_debug_logs 0
#undef enable_lua_decryption
#define enable_lua_decryption 0
#endif

namespace build {
    // const char* name = "";
}
