#pragma once

#define NOMINMAX
#define SENTRY_BUILD_STATIC 1
#define WIN32_LEAN_AND_MEAN 1
#include <sentry.h>

// indows imports
// #include <Windows.h>
#include <D3D11.h>

typedef long NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#include "DMALibrary/nt/structs.h" 


//DMA
#include "DmALibrary/libs/vmmdll.h"
 
// add headers that you want to pre-compile here
#include "DmALibrary/framework.h"
#include <Windows.h>
#include <TlHelp32.h>


#include <Winsock2.h>
#include <cstdio>
#include <sstream>  

#define DEBUG_INFO
#ifdef DEBUG_INFO
#define LOG(fmt, ...) std::printf(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) std::wprintf(fmt, ##__VA_ARGS__)
#else
#define LOG
#define LOGW
#endif

#define THROW_EXCEPTION 
#ifdef THROW_EXCEPTION
#define THROW(fmt, ...) throw std::runtime_error(fmt, ##__VA_ARGS__)
#endif


// std lib imports
#include <memory_resource>
#include <source_location>
#include <vector>
#include <string>
#include <iostream>
#include <deque>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <format>
#include <filesystem>
#include <mutex>
#include <array>
#include <unordered_map>
#include <map>
#include <optional>
#include <cstdint>
#include <any>
#include <chrono>
#include <fstream>
#include <stdexcept>

// audio playback
#include <xaudio2.h>

// renderer
#include <imgui/imgui.h>

// auth
#include <auth/c_user.hpp>

// other imports
#include <curl/curl.h>
#include <curl/easy.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

// menu precompile
#include "menu/framework/components.hpp"
#include "menu/framework/framework.hpp"

#include "sdk/math/geometry.hpp"
#include "sdk/math/vec2.hpp"
#include "sdk/math/vec3.hpp"

#include "utils\dynamic.hpp"
#include "utils\destroy_watcher.hpp"
// #include "sdk/memory/process.hpp"
#include "utils\timer.hpp"
#include "utils/holder.hpp"

#include <ntdll.hpp>

#include "security/src/hash_t.hpp"
