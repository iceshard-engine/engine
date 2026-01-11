/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_utils.hxx>
#include <ice/mem_memory.hxx>
#include <new>

namespace ice
{

    struct AllocRequest
    {
        ice::usize size = 0_B;
        ice::ualign alignment = ice::ualign::b_default;

        constexpr AllocRequest() noexcept = default;
        constexpr AllocRequest(ice::usize size, ice::ualign alignment = ice::ualign::b_default) noexcept;
        constexpr AllocRequest(ice::meminfo memory_info) noexcept;

        template<typename T> requires (std::is_pointer_v<T> == false)
        constexpr AllocRequest(ice::AlignResult<T> align_result) noexcept;
    };

    struct AllocResult
    {
        void* memory;
        ice::usize size;
        ice::ualign alignment;

        constexpr operator ice::Memory() const noexcept;
    };

    auto alloc(ice::usize size) noexcept -> ice::AllocResult;
    void release(void* pointer) noexcept;

    auto alloc_aligned(ice::usize size, ice::ualign alignment) noexcept -> ice::AllocResult;
    void release_aligned(void* pointer) noexcept;

    auto memcpy(void* dest, void const* source, ice::usize size) noexcept -> void*;
    auto memcpy(void* dest, ice::Data source) noexcept -> void*;
    auto memcpy(ice::Memory memory, ice::Data data) noexcept -> ice::Memory;

    auto memset(ice::Memory memory, ice::u8 value) noexcept -> ice::Memory;

    constexpr AllocRequest::AllocRequest(ice::usize size, ice::ualign alignment) noexcept
        : size{ size }
        , alignment{ alignment }
    {
    }

    constexpr AllocRequest::AllocRequest(ice::meminfo memory_info) noexcept
        : AllocRequest{ memory_info.size, memory_info.alignment }
    {
    }

    template<typename T> requires (std::is_pointer_v<T> == false)
    constexpr AllocRequest::AllocRequest(ice::AlignResult<T> align_result) noexcept
        : AllocRequest{ align_result.value, align_result.alignment }
    {
    }

    constexpr AllocResult::operator ice::Memory() const noexcept
    {
        return ice::Memory{
            .location = memory,
            .size = size,
            .alignment = alignment
        };
    }

} // namespace ice
