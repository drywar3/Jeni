#pragma once

#include <mini.cc/string.h>
#include <mini.cc/array.h>
#include <mini.cc/string_view.h>
#include <mini.cc/scope_guard.h>
#include <optional>
#include <expected>

// An indicator to show that a value was moved into a function.
// and freeing the value is no longer responsibility of the caller.
#define MOVE

using String = mini::String;
using Alloc  = Mini_Allocator;
using Sv     = mini::StringView;

template<typename T>
using Arr = mini::Array<T>;

template<typename T>
using Opt = std::optional<T>;

template<typename T, typename E>
using Expect = std::expected<T, E>;

template<typename E>
using Unexpect = std::unexpected<E>;

constexpr auto None = std::nullopt;

template<typename T, typename... Args>
static inline T *allocate(Alloc alloc, Args... args)
{
    T *mem = MINI_ALLOC(alloc, T);
    new (mem) T(args...);
    return mem;
}

#define defer  MINI_DEFER
#define assert MINI_ASSERT
#define todo   MINI_TODO
