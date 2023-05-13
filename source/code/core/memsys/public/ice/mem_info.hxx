/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>
#include <ice/mem_align.hxx>

namespace ice
{

    template<typename T>
    constexpr ice::usize size_of = { sizeof(T) };

    template<typename T>
    constexpr ice::ualign align_of = static_cast<ice::ualign>(alignof(T));

    template<typename T>
    constexpr ice::meminfo meminfo_of = { ice::size_of<T>, ice::align_of<T> };

    //! \brief Multypling ice::meminfo by a scalar changes the size but keeps the alignment.
    constexpr auto operator*(ice::meminfo info, ice::u64 count) noexcept -> ice::meminfo
    {
        return ice::meminfo{ .size = ice::usize{ info.size.value * count }, .alignment = info.alignment };
    }

    //! \brief Adding two ice::meminfo values changes the size and and alignment.
    //!
    //! \remark The resulting alignment will contain the alignment of the added ice::meminfo.
    //! \remark The resulting size may grow more than the added ice::meminfo.
    //!     This is because the operator adds the required offset <em>(if any)</em> to the size.
    //!
    //! \param[inout] left The ice::meminfo object that is updated.
    //! \param[in] right The description of memory we want to append to the total size.
    //!
    //! \returns The offset at which the described memory would be located in a single memory block.
    constexpr auto operator+=(ice::meminfo& left, ice::meminfo right) noexcept -> ice::usize
    {
        // Align first
        ice::AlignResult const res = ice::align_to(left.size, right.alignment);
        left.size = res.value;

        ice::usize const result = left.size;
        // The following line is commented out on purpose.
        //   We need to keep the original alignment so any allocation made
        //   will mimic all the added meminfo values and respect their offsets.
        // left.alignment = right.alignment; // DON'T SET THIS
        left.size += right.size;
        return result;
    }

} // namespace ice
