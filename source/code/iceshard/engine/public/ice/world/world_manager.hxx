#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

namespace ice
{

    class World;

    class WorldManager
    {
    public:
        virtual auto create_world(
            ice::StringID_Arg name,
            ice::ecs::EntityStorage* entity_storage
        ) noexcept -> World* = 0;

        virtual auto find_world(
            ice::StringID_Arg name
        ) noexcept -> World* = 0;

        virtual void destroy_world(
            ice::StringID_Arg world
        ) noexcept = 0;

    protected:
        virtual ~WorldManager() noexcept = default;
    };

} // namespace ice
