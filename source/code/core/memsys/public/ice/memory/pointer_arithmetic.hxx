#pragma once
#include <ice/base.hxx>

namespace ice::memory
{

    //! \brief Advances the pointer by the given number of bytes.
    inline auto ptr_add(void* ptr, ice::u64 bytes) noexcept -> void*;

    //! \brief Advances the pointer forward by the given number of bytes.
    inline auto ptr_add(void const* ptr, ice::u64 bytes) noexcept -> void const*;

    //! \brief Decreases the pointer by the given number of bytes.
    inline auto ptr_sub(void* ptr, ice::u64 bytes) noexcept -> void*;

    //! \brief Decreases the pointer by the given number of bytes.
    inline auto ptr_sub(void const* ptr, ice::u64 bytes) noexcept -> void const*;

    //! \brief Aligns the pointer to the specified alignment.
    //! \details If necessary the alignment is done by advancing the pointer.
    inline auto ptr_align_forward(void* ptr, ice::u64 align) noexcept -> void*;

    //! \brief Aligns the pointer to the specified alignment.
    //! \details If necessary the alignment is done by advancing the pointer.
    inline auto ptr_align_forward(void const* ptr, ice::u64 align) noexcept -> void const*;

    //! \brief Returns the distance in bytes between pointers.
    inline auto ptr_distance(void* from, void* to) noexcept -> ice::i32;

    //! \brief Returns the distance between pointers in bytes.
    inline auto ptr_distance(void const* from, void const* to) noexcept -> ice::i32;


    inline auto ptr_add(void* ptr, ice::u64 bytes) noexcept -> void*
    {
        return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) + bytes);
    }

    inline auto ptr_add(void const* ptr, ice::u64 bytes) noexcept -> void const*
    {
        return reinterpret_cast<void const*>(reinterpret_cast<std::byte const*>(ptr) + bytes);
    }

    inline auto ptr_sub(void* ptr, ice::u64 bytes) noexcept -> void*
    {
        return reinterpret_cast<void*>(reinterpret_cast<std::byte*>(ptr) - bytes);
    }

    inline auto ptr_sub(void const* ptr, ice::u64 bytes) noexcept -> void const*
    {
        return reinterpret_cast<void const*>(reinterpret_cast<std::byte const*>(ptr) - bytes);
    }

    inline auto ptr_align_forward(void* ptr, ice::u64 alignment) noexcept -> void*
    {
        ice::uptr result = reinterpret_cast<ice::uptr>(ptr);
        if (ice::u64 const mod = result % alignment; mod != 0)
        {
            result += (alignment - mod);
        }
        return reinterpret_cast<void*>(result);
    }

    inline auto ptr_align_forward(void const* ptr, ice::u64 alignment) noexcept -> void const*
    {
        ice::uptr result = reinterpret_cast<ice::uptr>(ptr);
        if (ice::u64 const mod = result % alignment; mod != 0)
        {
            result += (alignment - mod);
        }
        return reinterpret_cast<void*>(result);
    }

    inline auto ptr_distance(void* from, void* to) noexcept -> ice::i32
    {
        return static_cast<ice::i32>(
            reinterpret_cast<std::byte*>(to) - reinterpret_cast<std::byte*>(from)
        );
    }

    inline auto ptr_distance(void const* from, void const* to) noexcept -> ice::i32
    {
        return static_cast<ice::i32>(
            reinterpret_cast<std::byte const*>(to) - reinterpret_cast<std::byte const*>(from)
        );
    }

} // namespace ice::memory::utils
