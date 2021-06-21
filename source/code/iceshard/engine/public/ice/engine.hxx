#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/input/input_types.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    class AssetSystem;

    class EntityIndex;

    class WorldManager;

    class EngineRunner;

    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        virtual auto create_runner(
            ice::UniquePtr<ice::input::InputTracker> input_tracker,
            ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
        ) noexcept -> ice::UniquePtr<EngineRunner> = 0;

        virtual auto entity_index() noexcept -> ice::EntityIndex& = 0;

        virtual auto asset_system() noexcept -> ice::AssetSystem& = 0;

        virtual auto world_manager() noexcept -> ice::WorldManager& = 0;
    };

} // namespace ice
