#include <ice/data_storage.hxx>
#include <ice/assert.hxx>

namespace ice
{

    HashedDataStorage::HashedDataStorage(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _named_data{ _allocator }
    {
    }

    HashedDataStorage::~HashedDataStorage() noexcept
    {
        for (auto const& entry : _named_data)
        {
            _allocator.deallocate(entry.value);
        }
    }

    auto HashedDataStorage::named_data(ice::StringID_Arg name) noexcept -> void*
    {
        return ice::pod::hash::get(_named_data, ice::hash(name), nullptr);
    }

    auto HashedDataStorage::named_data(ice::StringID_Arg name) const noexcept -> void const*
    {
        return ice::pod::hash::get(_named_data, ice::hash(name), nullptr);
    }

    auto HashedDataStorage::allocate_named_data(
        ice::StringID_Arg name,
        ice::u32 size,
        ice::u32 alignment
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_data, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        void* object_ptr = _allocator.allocate(size, alignment);
        ice::pod::hash::set(_named_data, name_hash, object_ptr);
        return object_ptr;
    }

    auto HashedDataStorage::allocate_named_array(
        ice::StringID_Arg name,
        ice::u32 element_size,
        ice::u32 alignment,
        ice::u32 count
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_data, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        // TODO: To be refactored with the planned introduction of a proper 'size' type.
        ICE_ASSERT(alignment >= sizeof(ice::u32), "Cannot store array size in fron of the array!");

        void* object_ptr = _allocator.allocate(alignment + element_size * count, alignment);
        *reinterpret_cast<ice::u32*>(object_ptr) = count;
        ice::pod::hash::set(_named_data, name_hash, object_ptr);
        return object_ptr;
    }

    void HashedDataStorage::release_named_data(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_data, name_hash) == true,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        void* data = ice::pod::hash::get(_named_data, name_hash, nullptr);
        _allocator.deallocate(data);
        ice::pod::hash::remove(_named_data, name_hash);
    }


} // namespace ice
