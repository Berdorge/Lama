#ifndef ASSERTIONS_HPP
#define ASSERTIONS_HPP

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

inline void failure(char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

#endif
