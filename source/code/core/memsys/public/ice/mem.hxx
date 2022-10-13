#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_utils.hxx>
#include <ice/mem_memory.hxx>

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
        constexpr AllocRequest(ice::align_result<T> align_result) noexcept;
    };

    struct AllocResult
    {
        void* result;
        ice::usize size;
        ice::ualign alignment;

        constexpr operator ice::Memory() noexcept
        {
            return ice::Memory{
                .location = result,
                .size = size,
                .alignment = alignment
            };
        }
    };

    auto alloc(ice::usize request) noexcept -> ice::AllocResult;
    void release(ice::Memory memory) noexcept;

    auto alloc_aligned(ice::AllocRequest request) noexcept -> ice::AllocResult;
    void release_aligned(ice::Memory memory) noexcept;

    auto memcpy(void* dest, void const* source, ice::usize size) noexcept -> void*;
    auto memcpy(ice::Memory memory, ice::Data data) noexcept -> ice::Memory;

    // Additional overloads
    void release(void* pointer) noexcept;
    void release_aligned(void* pointer, ice::ualign alignment) noexcept;

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
    constexpr AllocRequest::AllocRequest(ice::align_result<T> align_result) noexcept
        : AllocRequest{ align_result.value, align_result.alignment }
    {
    }

} // namespace ice
