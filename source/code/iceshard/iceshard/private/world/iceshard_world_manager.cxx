#include "iceshard_world_manager.hxx"
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{



    IceWorldManager::IceWorldManager(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _worlds{ _allocator }
    {
    }

    IceWorldManager::~IceWorldManager() noexcept
    {
        for (auto const& entry : _worlds)
        {
            _allocator.destroy(entry.value);
        }
    }

    auto IceWorldManager::create_world(
        ice::StringID_Arg name,
        ice::EntityStorage* entity_storage
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_worlds, name_hash) == false,
            "A World with name {} already exists!",
            ice::stringid_hint(name)
        );

        IceWorld* const world = _allocator.make<IceWorld>(_allocator, entity_storage);
        ice::pod::hash::set(
            _worlds,
            name_hash,
            world
        );
        return world;
    }

    auto IceWorldManager::find_world(
        ice::StringID_Arg name
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(name);
        return ice::pod::hash::get(_worlds, name_hash, nullptr);
    }

    void IceWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::IceWorld* const world = ice::pod::hash::get(_worlds, name_hash, nullptr);

        if (world != nullptr)
        {
            _allocator.destroy(world);
            ice::pod::hash::remove(_worlds, name_hash);
        }
    }

} // namespace ice
