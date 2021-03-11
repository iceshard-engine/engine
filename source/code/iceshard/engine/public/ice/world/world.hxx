#pragma once
#include <ice/entity/entity_storage.hxx>

namespace ice
{


    class EngineRunner;

    class WorldTrait;

    struct WorldUpdateKey;

    class World
    {
    public:
        virtual auto entity_storage() noexcept -> ice::EntityStorage& = 0;

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

} // namespace ice
