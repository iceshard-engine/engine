#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    class EntityIndex;

    class WorldManager;

    class EngineRunner;

    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        virtual auto create_runner(
            ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
        ) noexcept -> ice::UniquePtr<EngineRunner> = 0;

        virtual auto entity_index() noexcept -> ice::EntityIndex& = 0;

        virtual auto world_manager() noexcept -> ice::WorldManager& = 0;
    };

} // namespace ice
