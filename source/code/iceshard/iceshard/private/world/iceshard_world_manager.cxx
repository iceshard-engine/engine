#include "iceshard_world_manager.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class TemporaryWorldTraitTracker : public ice::WorldTraitTracker
    {
    public:
        TemporaryWorldTraitTracker(ice::IceshardWorld& world) noexcept
            : _world{ world }
        {
        }

        auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTrait* override
        {
            return _world.find_trait(name);
        }

    private:
        ice::IceshardWorld& _world;
    };

    IceshardWorldManager::IceshardWorldManager(
        ice::Allocator& alloc,
        ice::WorldTraitArchive const& trait_archive
    ) noexcept
        : _allocator{ alloc }
        , _trait_archive{ trait_archive }
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
        ice::Allocator& alloc,
        ice::WorldTemplate const& world_template
    ) const noexcept -> ice::World*
    {
        ice::IceshardWorld* world = alloc.make<ice::IceshardWorld>(_allocator, world_template.entity_storage);
        ice::TemporaryWorldTraitTracker const trait_tracker{ *world };

        for (ice::StringID_Arg trait_name : world_template.traits)
        {
            ice::WorldTraitDescription const* description = _trait_archive.find_trait(trait_name);
            if (description != nullptr)
            {
                world->add_owning_trait(
                    trait_name,
                    description->factory(alloc, trait_tracker)
                );
            }
        }

        return world;
    }

    auto IceshardWorldManager::create_world(
        ice::WorldTemplate const& world_template
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(world_template.name);
        ICE_ASSERT(
            ice::pod::hash::has(_worlds, name_hash) == false,
            "A World with name {} already exists!",
            ice::stringid_hint(world_template.name)
        );

        IceshardWorld* const world = static_cast<ice::IceshardWorld*>(create_world(_allocator, world_template));
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
