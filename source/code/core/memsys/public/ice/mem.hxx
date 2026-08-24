/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_utils.hxx>
#include <ice/mem_memory.hxx>
#include <malloc.h>
#include <new>

namespace ice
{

    //! \brief The result of an allocation from an ice::Allocator object.
    struct AllocResult
    {
        //! \brief A pointer holding the address of the allocated memory block.
        void* memory;

        //! \brief The final size of the allocated block.
        //! \details Can never be smaller of what was requested.
        ice::usize size;

        //! \brief The actual alignment of this allocation.
        //! \details Can never be smaller of what was requested.
        ice::ualign alignment;

        //! \brief Utility conversion to a Memory object.
        constexpr operator ice::Memory() const noexcept;
    };

    //! \brief A data type containing \b size and \b alignment information for a single allocation.
    //! \details The allocation will always be at least as big as the requested `alignment` value.
    struct AllocRequest
    {
        //! \brief Number of bytes to allocate.
        ice::usize size = 0_B;

        //! \brief Alignment requested for this allocation.
        //! \details The default alignment might be different on different platforms.
        ice::ualign alignment = ice::ualign::b_default;

        constexpr AllocRequest() noexcept = default;

        //! \brief Initializes the structure with \b size and an optional \b alignment value.
        constexpr AllocRequest(ice::usize size, ice::ualign alignment = ice::ualign::b_default) noexcept;

        //! \brief Initializes the structure from a \b meminfo value.
        constexpr AllocRequest(ice::meminfo memory_info) noexcept;

        //! \brief Initializes the structure from an alignment operation result. See ice::align_to for additional info.
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
