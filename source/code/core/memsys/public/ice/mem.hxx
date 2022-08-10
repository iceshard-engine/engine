#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_utils.hxx>

namespace ice
{

    struct alloc_request
    {
        ice::usize size = 0_B;
        ice::ualign alignment = ice::ualign::b_default;

        constexpr alloc_request() noexcept = default;
        constexpr alloc_request(ice::usize size, ice::ualign alignment = ice::ualign::b_default) noexcept;
        constexpr alloc_request(ice::meminfo memory_info) noexcept;

        template<typename T> requires (std::is_pointer_v<T> == false)
        constexpr alloc_request(ice::align_result<T> align_result) noexcept;
    };

    struct alloc_result
    {
        void* result;
        ice::usize size;
        ice::ualign alignment;
    };

    auto alloc(ice::usize request) noexcept -> ice::alloc_result;
    void release(ice::alloc_result alloc_result) noexcept;

    auto alloc_aligned(ice::alloc_request request) noexcept -> ice::alloc_result;
    void release_aligned(ice::alloc_result alloc_result) noexcept;

    auto memcpy(void* dest, void const* source, ice::usize size) noexcept -> void*;

    // Additional overloads
    void release(void* pointer) noexcept;
    void release_aligned(void* pointer, ice::ualign alignment) noexcept;

    constexpr alloc_request::alloc_request(ice::usize size, ice::ualign alignment) noexcept
        : size{ size }
        , alignment{ alignment }
    {
    }

    constexpr alloc_request::alloc_request(ice::meminfo memory_info) noexcept
        : alloc_request{ memory_info.size, memory_info.alignment }
    {
    }

    template<typename T> requires (std::is_pointer_v<T> == false)
    constexpr alloc_request::alloc_request(ice::align_result<T> align_result) noexcept
        : alloc_request{ align_result.value, align_result.alignment }
    {
    }

} // namespace ice
