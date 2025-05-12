/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/data_storage.hxx>
#include <ice/assert.hxx>

namespace ice
{

#if 0
    HashedDataStorage::HashedDataStorage(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _named_data{ _allocator }
    {
    }

    HashedDataStorage::~HashedDataStorage() noexcept
    {
        for (ice::AllocResult const& entry : ice::hashmap::values(_named_data))
        {
            _allocator.deallocate(entry);
        }
    }

    auto HashedDataStorage::named_data(ice::StringID_Arg name) noexcept -> void*
    {
        return ice::hashmap::get(_named_data, ice::hash(name), AllocResult{ }).memory;
    }

    auto HashedDataStorage::named_data(ice::StringID_Arg name) const noexcept -> void const*
    {
        return ice::hashmap::get(_named_data, ice::hash(name), AllocResult{ }).memory;
    }

    auto HashedDataStorage::allocate_named_data(
        ice::StringID_Arg name,
        ice::meminfo type_meminfo
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_named_data, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        ice::AllocResult const alloc_result = _allocator.allocate(type_meminfo);
        ice::hashmap::set(_named_data, name_hash, alloc_result);
        return alloc_result.memory;
    }

    auto HashedDataStorage::allocate_named_array(
        ice::StringID_Arg name,
        ice::meminfo type_meminfo,
        ice::ucount count
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_named_data, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        ice::meminfo array_meminfo = ice::meminfo_of<ice::ucount> * 2;
        ice::usize const offset = array_meminfo += type_meminfo * count;

        ice::AllocResult const object_ptr = _allocator.allocate(array_meminfo);
        ice::hashmap::set(_named_data, name_hash, object_ptr);

        // We work on memory that starts with the 'ucount' value followed by the array starting location.
        ice::ucount* array_info = reinterpret_cast<ice::ucount*>(object_ptr.memory);
        array_info[0] = count;
        array_info[1] = ice::ucount(offset.value); // data offset (it's safe to assume it's small)
        return array_info;
    }

    void HashedDataStorage::release_named_data(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_named_data, name_hash) == true,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        ice::AllocResult const object_ptr = ice::hashmap::get(_named_data, name_hash, AllocResult{ });
        _allocator.deallocate(object_ptr);
        ice::hashmap::remove(_named_data, name_hash);
    }
#endif

} // namespace ice
