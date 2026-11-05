#pragma once

#include <cstring>

#include "misc/map.hpp"
#include "misc/misc.hpp"
#include "source.hpp"

struct Locus {
    usize line = 1;
    usize begin = 1, end = 1;
    usize first_byte = 0, last_byte = 0;
    Source_Id source_id{0};

    Locus() = default;
    Locus(usize line, usize begin, usize end, usize fb, usize lb,
          Source_Id id);
    usize length() const;
    Locus merge(const Locus& other) const;
};

/*
 * equality operator required for [std::unordered_map] lookup.
 * uses [std::strcmp] for [file_path] pointer equality fallback.
 */
inline bool operator==(const Locus &a, const Locus &b)
{
    if (a.line != b.line || a.begin != b.begin || a.end != b.end ||
        a.first_byte != b.first_byte || a.last_byte != b.last_byte ||
        a.source_id != b.source_id) {
        return false;
    }
    return true;
}

/*
 * specialization of [std::hash] for [Locus]
 */
template <> struct Hash<Locus> {
    std::size_t operator()(const Locus &loc) const noexcept
    {
        std::size_t seed = 0;

        hash_combine(seed, Hash<usize>{}(loc.line));
        hash_combine(seed, Hash<usize>{}(loc.begin));
        hash_combine(seed, Hash<usize>{}(loc.end));
        hash_combine(seed, Hash<usize>{}(loc.first_byte));
        hash_combine(seed, Hash<usize>{}(loc.last_byte));
        hash_combine(seed, Hash<usize>{}((usize)loc.source_id));

        return seed;
    }
};

template <> struct std::hash<Locus> {
    std::size_t operator()(const Locus &loc) const noexcept
    {
        std::size_t seed = 0;

        hash_combine(seed, Hash<usize>{}(loc.line));
        hash_combine(seed, Hash<usize>{}(loc.begin));
        hash_combine(seed, Hash<usize>{}(loc.end));
        hash_combine(seed, Hash<usize>{}(loc.first_byte));
        hash_combine(seed, Hash<usize>{}(loc.last_byte));
        hash_combine(seed, Hash<usize>{}((usize)loc.source_id));

        return seed;
    }
};
