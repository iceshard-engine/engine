#pragma once
#include <ice/base.hxx>

namespace ice::memory
{

    //! \brief Advances the pointer by the given number of bytes.
    inline auto ptr_add(void* ptr, uint32_t bytes) noexcept -> void*;

    //! \brief Advances the pointer forward by the given number of bytes.
    inline auto ptr_add(void const* ptr, uint32_t bytes) noexcept -> void const*;

    //! \brief Decreases the pointer by the given number of bytes.
    inline auto ptr_sub(void* ptr, uint32_t bytes) noexcept -> void*;

    //! \brief Decreases the pointer by the given number of bytes.
    inline auto ptr_sub(void const* ptr, uint32_t bytes) noexcept -> void const*;

    //! \brief Aligns the pointer to the specified alignment.
    //! \details If necessary the alignment is done by advancing the pointer.
    inline auto ptr_align_forward(void* ptr, uint32_t align) noexcept -> void*;

    //! \brief Aligns the pointer to the specified alignment.
    //! \details If necessary the alignment is done by advancing the pointer.
    inline auto ptr_align_forward(void const* ptr, uint32_t align) noexcept -> void const*;

    //! \brief Returns the distance in bytes between pointers.
    inline auto ptr_distance(void* from, void* to) noexcept -> int32_t;

    //! \brief Returns the distance between pointers in bytes.
    inline auto ptr_distance(void const* from, void const* to) noexcept -> int32_t;


    inline auto ptr_add(void* ptr, uint32_t bytes) noexcept -> void*
    {
        return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) + bytes);
    }

    inline auto ptr_add(void const* ptr, uint32_t bytes) noexcept -> void const*
    {
        return reinterpret_cast<void const*>(reinterpret_cast<std::byte const*>(ptr) + bytes);
    }

    inline auto ptr_sub(void* ptr, uint32_t bytes) noexcept -> void*
    {
        return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) - bytes);
    }

    inline auto ptr_sub(void const* ptr, uint32_t bytes) noexcept -> void const*
    {
        return reinterpret_cast<void const*>(reinterpret_cast<std::byte const*>(ptr) - bytes);
    }

    inline auto ptr_align_forward(void* ptr, uint32_t alignment) noexcept -> void*
    {
        uintptr_t result = reinterpret_cast<uintptr_t>(ptr);
        if (uint32_t const mod = result % alignment; mod != 0)
        {
            result += (alignment - mod);
        }
        return reinterpret_cast<void*>(result);
    }

    inline auto ptr_align_forward(void const* ptr, uint32_t alignment) noexcept -> void const*
    {
        uintptr_t result = reinterpret_cast<uintptr_t>(ptr);
        if (uint32_t const mod = result % alignment; mod != 0)
        {
            result += (alignment - mod);
        }
        return reinterpret_cast<void*>(result);
    }

    inline auto ptr_distance(void* from, void* to) noexcept -> int32_t
    {
        return static_cast<int32_t>(
            reinterpret_cast<std::byte*>(to) - reinterpret_cast<std::byte*>(from)
        );
    }

    inline auto ptr_distance(void const* from, void const* to) noexcept -> int32_t
    {
        return static_cast<int32_t>(
            reinterpret_cast<std::byte const*>(to) - reinterpret_cast<std::byte const*>(from)
        );
    }

} // namespace ice::memory::utils
