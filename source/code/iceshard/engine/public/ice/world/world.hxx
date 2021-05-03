#pragma once
#include <ice/engine_request.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/data_storage.hxx>

namespace ice
{

    class EngineRunner;

    class WorldTrait;

    struct WorldUpdateKey;

    enum class WorldState
    {
        Idle,
        Active,
    };

    class World
    {
    public:
        virtual auto allocator() noexcept -> ice::Allocator& = 0;
        virtual auto entity_storage() noexcept -> ice::EntityStorage& = 0;

        virtual auto data_storage() noexcept -> ice::DataStorage& = 0;

        virtual void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept = 0;

        virtual void remove_trait(
            ice::StringID_Arg name
        ) = 0;

        virtual void update(
            ice::EngineRunner& runner,
            WorldUpdateKey
        ) noexcept = 0;

    protected:
        virtual ~World() noexcept = default;
    };

    static constexpr StringID Request_ActivateWorld = "ice.request.activate_world"_sid;
    static constexpr StringID Request_DeactivateWorld = "ice.request.deactivate_world"_sid;

} // namespace ice
