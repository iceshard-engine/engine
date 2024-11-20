/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
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
    auto mem_construct_n_at(ice::Memory memory, ice::ucount count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::ucount idx = 0; idx < count; ++idx)
        {
            new (target_mem + idx) T{ };
        }
        return target_mem;
    }

    template<typename T>
    auto mem_move_construct_n_at(ice::Memory memory, T* objects, ice::ucount count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::ucount idx = 0; idx < count; ++idx)
        {
            new (target_mem + idx) T{ ice::move(objects[idx]) };
        }
        return target_mem;
    }

    template<typename T>
    auto mem_move_n_to(T* target_objects, T* objects, ice::ucount count) noexcept -> T*
    {
        for (ice::ucount idx = 0; idx < count; ++idx)
        {
            target_objects[idx] = ice::move(objects[idx]);
        }
    }

    template<typename T>
    auto mem_move_n_to_reverse(T* target_objects, T* objects, ice::ucount count) noexcept -> T*
    {
        ICE_ASSERT_CORE(count > 0);

        // Moving in reverse so we don't override data if it's on the same memory block.
        do
        {
            count -= 1; // reduce by one so we can use it as the next index.
            target_objects[count] = ice::move(objects[count]);
        }
        while(count > 0);
        return target_objects;
    }

    template<typename T>
    auto mem_copy_construct_n_at(ice::Memory memory, T const* objects, ice::ucount count) noexcept -> T*
    {
        // TODO: Assert (align + size)
        T* target_mem = reinterpret_cast<T*>(memory.location);
        for (ice::ucount idx = 0; idx < count; ++idx)
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
    void mem_destruct_n_at(T* location, ice::ucount count) noexcept
    {
        for (ice::ucount idx = 0; idx < count; ++idx)
        {
            ice::mem_destruct_at(location + idx);
        }
    }

} // namespace ice
