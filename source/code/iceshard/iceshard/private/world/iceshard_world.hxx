#pragma once
#include <ice/world/world.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/allocator.hxx>

namespace ice
{

    class EngineRunner;

    struct WorldUpdateKey
    {
        ice::u32 reserved;
    };

    class IceWorld final : public ice::World
    {
    public:
        IceWorld(
            ice::Allocator& alloc,
            ice::EntityStorage* entity_storage
        ) noexcept;

        auto entity_storage() noexcept -> ice::EntityStorage& override;

        void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept override;

        void remove_trait(
            ice::StringID_Arg name
        ) noexcept override;

        void update(
            ice::EngineRunner& runner,
            WorldUpdateKey
        ) noexcept override;

        auto traits() noexcept -> ice::pod::Hash<ice::WorldTrait*>&;

    private:
        ice::Allocator& _allocator;
        ice::EntityStorage* _entity_storage;

        ice::pod::Hash<ice::WorldTrait*> _traits;
    };

} // namespace ice
