#include "iceshard_world_manager.hxx"
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{



    IceshardWorldManager::IceshardWorldManager(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _worlds{ _allocator }
    {
    }

    IceshardWorldManager::~IceshardWorldManager() noexcept
    {
        for (auto const& entry : _worlds)
        {
            _allocator.destroy(entry.value);
        }
    }

    auto IceshardWorldManager::create_world(
        ice::StringID_Arg name,
        ice::ecs::EntityStorage* entity_storage
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_worlds, name_hash) == false,
            "A World with name {} already exists!",
            ice::stringid_hint(name)
        );

        IceshardWorld* const world = _allocator.make<IceshardWorld>(_allocator, entity_storage);
        ice::pod::hash::set(
            _worlds,
            name_hash,
            world
        );
        return world;
    }

    auto IceshardWorldManager::find_world(
        ice::StringID_Arg name
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(name);
        return ice::pod::hash::get(_worlds, name_hash, nullptr);
    }

    void IceshardWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::IceshardWorld* const world = ice::pod::hash::get(_worlds, name_hash, nullptr);

        if (world != nullptr)
        {
            _allocator.destroy(world);
            ice::pod::hash::remove(_worlds, name_hash);
        }
    }

    auto IceshardWorldManager::worlds() const noexcept -> ice::pod::Hash<ice::IceshardWorld*> const&
    {
        return _worlds;
    }

} // namespace ice
