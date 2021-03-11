#pragma once
#include <ice/world/world_manager.hxx>
#include "iceshard_world.hxx"

namespace ice
{

    class IceWorldManager final : public ice::WorldManager
    {
    public:
        IceWorldManager(
            ice::Allocator& alloc
        ) noexcept;
        ~IceWorldManager() noexcept override;

        auto create_world(
            ice::StringID_Arg name,
            ice::EntityStorage* entity_storage
        ) noexcept -> World* override;

        auto find_world(
            ice::StringID_Arg name
        ) noexcept -> World* override;

        void destroy_world(
            ice::StringID_Arg name
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::IceWorld*> _worlds;
    };

} // namespace ice
