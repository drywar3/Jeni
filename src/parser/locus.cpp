#include "parser/locus.hpp"

Locus::Locus(usize line, usize begin, usize end, usize fb, usize lb,
             Source_Id id)
    :
    line      (line),
    begin     (begin),
    end       (end),
    first_byte(fb),
    last_byte (lb),
    source_id (id)
{
}

usize Locus::length() const { return end - begin; }

Locus Locus::merge(const Locus& other) const
{
    MINI_ASSERT(source_id == other.source_id,
                "locations point to two different sources");
    return Locus(line, begin, other.end, first_byte,
                 other.last_byte, other.source_id);
}
