/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/mem_memory.hxx>

namespace ice
{

    template<typename T, typename... Args>
    auto mem_construct_at(ice::Memory memory, Args&&... args) noexcept -> T*
    {
        // TODO: Assert (align + size)
        return new (memory.location) T{ ice::forward<Args>(args)... };
    }

    template<typename T>
    auto mem_move_construct_at(ice::Memory memory, T&& other) noexcept -> T*
    {
        // TODO: Assert (align + size)
        return new (memory.location) T{ ice::move(other) };
    }

    template<typename T>
    auto mem_copy_construct_at(ice::Memory memory, T const& other) noexcept -> T*
    {
        // TODO: Assert (align + size)
        return new (memory.location) T{ other };
    }

    template<typename T>
    auto mem_construct_n_at(ice::Memory memory, ice::u32 count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            new (target_mem + idx) T{ };
        }
        return target_mem;
    }

    template<typename T>
    auto mem_move_construct_n_at(ice::Memory memory, T* objects, ice::u32 count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            new (target_mem + idx) T{ ice::move(objects[idx]) };
        }
        return target_mem;
    }

    template<typename T>
    auto mem_move_n_to(T* target_objects, T* objects, ice::u32 count) noexcept -> T*
    {
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            target_objects[idx] = ice::move(objects[idx]);
        }
        return target_objects;
    }

    template<typename T>
    auto mem_copy_construct_n_at(ice::Memory memory, T const* objects, ice::u32 count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            new (target_mem + idx) T{ objects[idx] };
        }
        return target_mem;
    }



    template<typename T>
    void mem_destruct_at(T* location) noexcept
    {
        location->~T();
    }

    template<typename T>
    void mem_destruct_n_at(T* location, ice::u32 count) noexcept
    {
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            ice::mem_destruct_at(location + idx);
        }
    }

} // namespace ice
