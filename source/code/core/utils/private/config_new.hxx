/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>

namespace ice
{

    inline auto calc_varstring_size_total(ice::u32 size) noexcept -> ice::u32
    {
        ice::u32 bytes = 0;
        while(size > 0x7f)
        {
            size >>= 7;
            bytes += 1;
        }
        return (bytes + 1) + size;
    }

    inline auto read_varstring_size(void const* data, ice::u32& out_bytes) noexcept -> ice::u32
    {
        ice::u32 result = 0;
        ice::u8 const* varb = reinterpret_cast<ice::u8 const*>(data);
        out_bytes = 1;
        while(*varb & 0x80)
        {
            result += *varb;
            result <<= 7;
            varb += 1;
            out_bytes += 1;
        }
        return result + *varb;
    }

    inline auto write_varstring_size(void* data, ice::u32 size) noexcept -> ice::u32
    {
        ice::u32 bytes = 0;
        ice::u8* varb = reinterpret_cast<ice::u8*>(data);
        while(size > 0x7f)
        {
            varb[bytes] = (size & 0x7f) | 0x80;
            size >>= 7;
            bytes += 1;
        }
        varb[bytes] = size & 0x7f;
        return bytes + 1;
    }

} // namespace ice
