#pragma once
#include <ice/entity/entity_storage.hxx>
#include <ice/data_storage.hxx>

namespace ice
{

    class WorldTrait;
    class WorldPortal;

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

        virtual void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept = 0;

        virtual void remove_trait(
            ice::StringID_Arg name
        ) noexcept = 0;

    protected:
        virtual ~World() noexcept = default;
    };

} // namespace ice
