/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container/contiguous_container.hxx>

#include <ice/container_logic.hxx>
#include <ice/string_types.hxx>
#include <ice/span.hxx>
#include <ice/array.hxx>
#include <ice/queue.hxx>
#include <ice/hashmap.hxx>
#include <array>

namespace ice
{


    //! \brief A view into data created by a hashmap object.
    //!
    //! \note No modification of data is allowed through this type.
    //! \note Because this is only a view into a hashmap there is no difference in 'logic' so it's unused.
    template<typename Type, ice::ContainerLogic = ice::ContainerLogic::Trivial>
    struct HashMapView
    {
        using Entry = typename ice::HashMap<Type>::Entry;
        using ValueType = Type;

        ice::u32 _capacity;
        ice::u32 _count;

        ice::u32 const* _hashes;
        Entry const* _entries;
        Type const* _data;
    };


    // TODO: Introduce our own type and create proper concepts for function access.
    template<typename T, ice::u32 Size>
    using StaticArray = std::array<T, Size>;

} // namespace ice
