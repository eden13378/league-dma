#pragma once

#define NOMINMAX
#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

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

// #include <ntdll.hpp>

#include "security/src/hash_t.hpp"

// indows imports
#include <Windows.h>
#include <TlHelp32.h>
#include <D3D11.h>
