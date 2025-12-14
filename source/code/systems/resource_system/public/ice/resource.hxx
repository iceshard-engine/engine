/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/expected.hxx>
#include <ice/resource_types.hxx>
#include <ice/string_types.hxx>
#include <ice/task.hxx>

namespace ice
{

    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        virtual auto uri() const noexcept -> ice::URI const& = 0;
        virtual auto flags() const noexcept -> ice::ResourceFlags = 0;

        virtual auto name() const noexcept -> ice::String = 0;
        virtual auto origin() const noexcept -> ice::String = 0;
    };

    //! \todo Rethink how loose resources and their named parts can be accessed.
    class LooseResource
    {
    public:
        virtual ~LooseResource() noexcept = default;

        virtual auto uri() const noexcept -> ice::URI const& = 0;

        virtual auto size() const noexcept -> ice::usize = 0;

        virtual auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

    auto allocate_resource_object_memory(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::usize size,
        ice::ualign align
    ) noexcept -> ice::Memory;

    void deallocate_resource_object_memory(
        ice::Allocator& alloc,
        ice::Resource* pointer
    ) noexcept;

    template<typename T, typename... Args>
    auto create_resource_object(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        Args&&... args
    ) noexcept -> T*
    {
        ice::Memory const memory = ice::allocate_resource_object_memory(alloc, provider, ice::size_of<T>, ice::align_of<T>);
        T* result = new (memory.location) T{ ice::forward<Args>(args)... };
        return result;
    }

    template<typename T>
    void destroy_resource_object(
        ice::Allocator& alloc,
        T* object
    ) noexcept
    {
        if (object != nullptr)
        {
            object->~T();
            ice::deallocate_resource_object_memory(alloc, object);
        }
    }

} // ice
