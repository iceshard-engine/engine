#pragma once
#include <ice/world/world.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/data_storage.hxx>

namespace ice
{

    class EngineRunner;

    struct WorldUpdateKey
    {
        ice::u32 reserved;
    };

    class IceshardWorld final : public ice::World
    {
    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::EntityStorage* entity_storage
        ) noexcept;

        auto allocator() noexcept -> ice::Allocator& override;
        auto entity_storage() noexcept -> ice::EntityStorage& override;

        auto data_storage() noexcept -> ice::DataStorage& override;

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
        ice::HashedDataStorage _data_storage;
    };

} // namespace ice
