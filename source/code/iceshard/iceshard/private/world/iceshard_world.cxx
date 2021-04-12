#include "iceshard_world.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/engine_runner.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::EntityStorage* entity_storage
    ) noexcept
        : _allocator{ alloc }
        , _entity_storage{ _entity_storage }
        , _traits{ _allocator }
    {
    }

    auto IceshardWorld::entity_storage() noexcept -> ice::EntityStorage&
    {
        return *_entity_storage;
    }

	void IceshardWorld::add_trait(
        ice::StringID_Arg name,
        ice::WorldTrait* trait
    ) noexcept
	{
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_traits, name_hash) == false,
            "World already contains a trait of name {}",
            ice::stringid_hint(name)
        );

        ice::pod::hash::set(
            _traits,
            name_hash,
            trait
        );
	}

    void IceshardWorld::remove_trait(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::pod::hash::remove(
            _traits,
            ice::hash(name)
        );
    }

    void IceshardWorld::update(
        ice::EngineRunner& runner,
        WorldUpdateKey
    ) noexcept
    {
        for (auto const& entry : _traits)
        {
            entry.value->on_update(
                runner.current_frame(),
                runner,
                *this
            );
        }
    }

    auto IceshardWorld::traits() noexcept -> ice::pod::Hash<ice::WorldTrait*>&
    {
        return _traits;
    }

} // namespace ice
