/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os.hxx>
#include <ice/mem_info.hxx>
#include <ice/mem_utils.hxx>
#include <ice/mem_memory.hxx>

namespace ice
{

    struct AllocResult;
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

    struct ChunkedAllocRequest
    {
        ice::meminfo _request_meminfo{ };
        ice::u64 _chunk_count = 0;
        ice::usize _chunk_offsets[14];
        void** _chunk_pointers[15];

        constexpr explicit ChunkedAllocRequest() noexcept = default;

        template<typename T>
        constexpr auto include(T*& ptrref, ice::u64 count) noexcept;
        constexpr void finalize(ice::AllocResult result) const noexcept;
        constexpr void reset() noexcept;
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

    template<typename T>
    inline constexpr auto ChunkedAllocRequest::include(T*& ptrref, ice::u64 count) noexcept
    {
        ICE_ASSERT_CORE(_chunk_count < 14); // You sure you need that much? (Refactor if more are required)

        if (_chunk_count == 0)
        {
            _request_meminfo = ice::meminfo_of<T> * count;
        }
        else
        {
            ice::usize const offset = _request_meminfo += ice::meminfo_of<T> * count;
            _chunk_offsets[_chunk_count - 1] = offset;
        }
        _chunk_pointers[_chunk_count] = reinterpret_cast<void**>(ice::addressof(ptrref));
        _chunk_count += 1;
    }

    inline constexpr void ChunkedAllocRequest::finalize(ice::AllocResult result) const noexcept
    {
        ICE_ASSERT_CORE(result.size >= _request_meminfo.size);
        ICE_ASSERT_CORE(result.alignment >= _request_meminfo.alignment);
        *_chunk_pointers[0] = result.memory; // Assign the allocation result directly to the first pointer.
        // Assign the other pointers according to the stored offsets
        for (ice::u64 index = 1; index < _chunk_count; ++index)
        {
            *_chunk_pointers[index] = ice::ptr_add(result.memory, _chunk_offsets[index - 1]);
        }
    }

    inline constexpr void ChunkedAllocRequest::reset() noexcept
    {
        _request_meminfo = ice::meminfo{};
        _chunk_count = 0;
    }

} // namespace ice
