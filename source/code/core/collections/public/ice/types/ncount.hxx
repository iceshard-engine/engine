/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/types/nvalue.hxx>

namespace ice
{

    struct ncount_invalid_t;

    struct ncount : public ice::nvalue
    {
        using nvalue::base_type;
        using nvalue::base_signed_type;

        // support for allocation sizes
        constexpr auto bytes(this ncount self) noexcept -> ice::usize;

        constexpr ncount() noexcept = default;
        constexpr ncount(nvalue value) noexcept;
        constexpr ncount(base_type value) noexcept;
        constexpr ncount(base_type value, base_type width) noexcept;

        // special operators
        constexpr bool operator==(this ncount self, ncount_invalid_t) noexcept;

        // implicit type conversions
        constexpr operator ice::usize(this ncount self) noexcept { return self.bytes(); }
    };

    struct ncount_invalid_t : ncount {};

    inline constexpr auto ncount::bytes(this ncount self) noexcept -> ice::usize
    {
        return { static_cast<ice::usize::base_type>(self._value) * self._width };
    }

    inline constexpr ncount::ncount(nvalue value) noexcept
        : nvalue{ value }
    { }

    inline constexpr ncount::ncount(base_type value) noexcept
        : nvalue{ 1, static_cast<base_signed_type>(value) }
    { }

    inline constexpr ncount::ncount(base_type value, base_type width) noexcept
        : nvalue{ width, static_cast<base_signed_type>(value) }
    { }

    inline constexpr bool ncount::operator==(this ncount self, ncount_invalid_t) noexcept
    {
        return self._width == 0;
    }

    static constexpr ice::ncount ncount_max{ ice::detail::nvalue_max_value() };
    static constexpr ice::ncount ncount_min{ ice::detail::nvalue_min_value() };
    static constexpr ice::ncount_invalid_t ncount_none{ };

} // namespace ice
