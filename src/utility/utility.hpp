/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_UTILITY_UTILITY_HPP
#define GA_UTILITY_UTILITY_HPP

#include <execution>
#include <cassert>
#include <cstddef>


#define GA_ASSERT(condition, msg) assert((condition) && msg)
#define GA_THROW(exception_type, msg) throw exception_type(msg)

#define GA_UNUSED(...) (void)(sizeof(__VA_ARGS__))

#ifdef __GNUC__
#   define GA_UNREACHABLE() assert(false); __builtin_unreachable()
#elif defined(_MSC_VER)
#   define GA_UNREACHABLE() assert(false); __assume(false)
#else
#   define GA_UNREACHABLE() assert(false);
#endif


#ifndef GA_EXCUTION_UNSEQ
#   define GA_EXECUTION_UNSEQ std::execution::par_unseq
#endif

#ifndef GA_EXECUTION_SEQ
#   define GA_EXECUTION_SEQ std::execution::par
#endif


#ifdef GA_EXPORTS
#   define GA_API __declspec(dllexport)
#else
#   define GA_API __declspec(dllimport)
#endif


namespace genetic_algorithm
{
    constexpr std::size_t operator ""_sz(unsigned long long arg)
    {
        return static_cast<std::size_t>(arg);
    }

    constexpr std::ptrdiff_t operator ""_pd(unsigned long long arg)
    {
        return static_cast<std::ptrdiff_t>(arg);
    }

} // namespace genetic_algorithm

#endif // !GA_UTILITY_UTILITY_HPP