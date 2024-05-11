#pragma once

class InvalidHolderException final : public std::exception {
public:
    ~InvalidHolderException( ) noexcept override = default;
    [[nodiscard]] auto what( ) const -> const char* override{ return "object is invalid"; }
};

class InvalidMemoryAddressException final : public std::exception {
public:
    ~InvalidMemoryAddressException( ) noexcept override = default;
    [[nodiscard]] auto what( ) const -> const char* override{ return "invalid memory address"; }
};
