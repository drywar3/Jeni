#pragma once

#include "sema/entities/id.hpp"
namespace sema::type_id
{
    constexpr auto ERROR = Type_Id(0);

    constexpr auto INT8   = Type_Id(1);
    constexpr auto INT16  = Type_Id(2);
    constexpr auto INT32  = Type_Id(3);
    constexpr auto INT64  = Type_Id(4);

    constexpr auto UINT8  = Type_Id(5);
    constexpr auto UINT16 = Type_Id(6);
    constexpr auto UINT32 = Type_Id(7);
    constexpr auto UINT64 = Type_Id(8);

    constexpr auto FLOAT32 = Type_Id(9);
    constexpr auto FLOAT64 = Type_Id(10);

    constexpr auto CHAR  = Type_Id(11);
    constexpr auto VOID  = Type_Id(12);
    constexpr auto BOOL  = Type_Id(13);

    // always ensure this is the last id.
    constexpr auto LAST_  = Type_Id(14);
}
