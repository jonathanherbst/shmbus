#pragma once

#include <cstdint>

typedef std::uint8_t byte;

inline void* byte_next(void* ptr, std::ptrdiff_t n)
{
    return static_cast<byte*>(ptr) + n;
}

inline const void* byte_next(const void* ptr, std::ptrdiff_t n)
{
    return static_cast<const byte*>(ptr) + n;
}

inline std::ptrdiff_t byte_diff(const void* a, const void* b)
{
    return static_cast<const byte*>(a) - static_cast<const byte*>(b);
}
