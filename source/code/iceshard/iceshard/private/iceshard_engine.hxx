#pragma once
#include <ice/engine.hxx>
#include <ice/asset_system.hxx>
#include <ice/entity/entity_index.hxx>
#include <ice/input/input_types.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include "world/iceshard_world_manager.hxx"

namespace ice
{

    class AssetSystem;
    class EngineDevUI;

    class IceshardEngine final : public ice::Engine
    {
    public:
        IceshardEngine(
            ice::Allocator& alloc,
            ice::AssetSystem& asset_system,
            ice::EngineDevUI* devui
        ) noexcept;
        ~IceshardEngine() noexcept override;

        auto create_runner(
            ice::UniquePtr<ice::input::InputTracker> input_tracker,
            ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
        ) noexcept -> ice::UniquePtr<EngineRunner> override;

        auto entity_index() noexcept -> ice::EntityIndex& override;

        auto asset_system() noexcept -> ice::AssetSystem& override;

        auto world_manager() noexcept -> ice::WorldManager& override;

        auto developer_ui() noexcept -> ice::EngineDevUI& override;

    private:
        ice::memory::ProxyAllocator _allocator;
        ice::AssetSystem& _asset_system;
        ice::EntityIndex _entity_index;
        ice::IceshardWorldManager _world_manager;
        ice::EngineDevUI* const _devui;
    };

} // namespace ice
