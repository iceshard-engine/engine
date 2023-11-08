/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>

namespace ice
{

    struct Memory
    {
        void* location;
        ice::usize size;
        ice::ualign alignment;
    };

    //! \return 'memory' object advanced by the number of 'offset' bytes, aligned to 'meminfo.alignment' with reduced 'size' member.
    //! \note The resulting 'size' member is not checked and the value may overflow.
    //! \note The 'align' member is updated to the meminfo alignment value.
    inline auto ptr_adv(ice::Memory mem, ice::meminfo meminfo) noexcept -> ice::Memory;

    //! \return 'memory' object advanced by the number of 'offset' bytes, aligned to 'alignment' with a reduced 'size' member.
    //! \note The resulting 'size' member is not checked and the value may overflow.
    //! \note The 'align' member is updated to the requested alignment value.
    inline auto ptr_adv(ice::Memory mem, ice::usize offset, ice::ualign align) noexcept -> ice::Memory;

    //! \return 'memory' object advanced by the number of 'offset' bytes and reduced 'size' member.
    //! \note The resulting 'size' member is not checked and the value may overflow.
    //! \note The 'align' member remains unchanged.
    inline auto ptr_add(ice::Memory mem, ice::usize offset) noexcept -> ice::Memory;


    inline auto ptr_adv(ice::Memory mem, ice::usize offset, ice::ualign alignment) noexcept -> ice::Memory
    {
        ice::AlignResult const aligned = ice::align_to(ice::ptr_add(mem.location, offset), alignment);
        return {
            .location = aligned.value,
            .size = { mem.size.value - (offset + aligned.padding).value },
            .alignment = aligned.alignment
        };
    }

    inline auto ptr_adv(ice::Memory mem, ice::meminfo meminfo) noexcept -> ice::Memory
    {
        return ptr_adv(mem, meminfo.size, meminfo.alignment);
    }

    inline auto ptr_add(ice::Memory mem, ice::usize offset) noexcept -> ice::Memory
    {
        ICE_ASSERT_CORE(mem.size >= offset);
        return Memory{
            .location = ice::ptr_add(mem.location, offset),
            .size = { mem.size.value - offset.value },
            .alignment = mem.alignment
        };
    }

} // namespace ice
