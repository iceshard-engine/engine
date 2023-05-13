/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world_manager.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/container/hashmap.hxx>
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
        for (IceshardWorld* world : _worlds)
        {
            _allocator.destroy(world);
        }
    }

    auto IceshardWorldManager::create_world(
        ice::Allocator& alloc,
        ice::WorldTemplate const& world_template
    ) const noexcept -> ice::World*
    {
        ice::IceshardWorld* world = alloc.create<ice::IceshardWorld>(alloc, world_template.entity_storage);
        ice::TemporaryWorldTraitTracker const trait_tracker{ *world };

        for (ice::StringID_Arg trait_name : world_template.traits)
        {
            ice::WorldTraitDescription const* description = _trait_archive.find_trait(trait_name);
            if (description != nullptr)
            {
                world->add_owning_trait(
                    trait_name,
                    description->factory(description->factory_userdata, alloc, trait_tracker)
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
            ice::hashmap::has(_worlds, name_hash) == false,
            "A World with name {} already exists!",
            ice::stringid_hint(world_template.name)
        );

        bool const valid_trait_list = _trait_archive.validate_trait_list(world_template.traits);
        ICE_ASSERT(
            valid_trait_list,
            "World template trait list breaks dependency requirements!"
        );

        if (valid_trait_list)
        {
            IceshardWorld* world = static_cast<ice::IceshardWorld*>(create_world(_allocator, world_template));
            ice::hashmap::set(
                _worlds,
                name_hash,
                world
            );
            return world;
        }

        return nullptr;
    }

    auto IceshardWorldManager::find_world(
        ice::StringID_Arg name
    ) noexcept -> World*
    {
        ice::u64 const name_hash = ice::hash(name);
        return ice::hashmap::get(_worlds, name_hash, nullptr);
    }

    void IceshardWorldManager::destroy_world(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::IceshardWorld* const world = ice::hashmap::get(_worlds, name_hash, nullptr);

        if (world != nullptr)
        {
            _allocator.destroy(world);
            ice::hashmap::remove(_worlds, name_hash);
        }
    }

    auto IceshardWorldManager::worlds() const noexcept -> ice::Span<ice::IceshardWorld* const>
    {
        return ice::hashmap::values(_worlds);
    }

} // namespace ice
