/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types/ncount.hxx>

namespace ice
{

    struct nindex_invalid_t;

    struct nindex : public ice::nvalue
    {
        using nvalue::base_type;
        using nvalue::base_signed_type;

        // support for allocation sizes
        constexpr auto offset(this nindex self) noexcept -> ice::usize;

        constexpr nindex() noexcept = default;
        constexpr nindex(nvalue value) noexcept;
        constexpr nindex(base_type value) noexcept;
        constexpr nindex(base_type value, base_type width) noexcept;

        // special operators
        constexpr bool operator==(this ncount self, nindex_invalid_t) noexcept;
    };

    struct nindex_invalid_t : nindex {};

    inline constexpr auto nindex::offset(this nindex self) noexcept -> ice::usize
    {
        return { static_cast<ice::usize::base_type>(self._value) * self._width };
    }

    inline constexpr nindex::nindex(nvalue value) noexcept
        : nvalue{ value }
    { }

    inline constexpr nindex::nindex(base_type value) noexcept
        : nvalue{ 1, static_cast<base_signed_type>(value) }
    { }

    inline constexpr nindex::nindex(base_type value, base_type width) noexcept
        : nvalue{ width, static_cast<base_signed_type>(value) }
    { }

    inline constexpr bool nindex::operator==(this ncount self, nindex_invalid_t) noexcept
    {
        return self._width == 0;
    }

    static constexpr ice::nindex nindex_max{ ice::detail::nvalue_max_value() };
    static constexpr ice::nindex nindex_min{ ice::detail::nvalue_min_value() };
    static constexpr ice::nindex_invalid_t none_index{ };

} // namespace ice
