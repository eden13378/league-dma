#pragma once

namespace crash_detector {
    auto check( ) noexcept -> std::expected<void, const char*>;
    auto clean( ) noexcept -> void;
}
