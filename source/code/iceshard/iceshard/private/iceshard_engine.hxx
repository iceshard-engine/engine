#pragma once
#include <ice/engine.hxx>
#include <ice/asset_system.hxx>
#include <ice/entity/entity_index.hxx>
#include "world/iceshard_world_manager.hxx"

namespace ice
{

    class AssetSystem;

    class IceshardEngine final : public ice::Engine
    {
    public:
        IceshardEngine(
            ice::Allocator& alloc,
            ice::AssetSystem& asset_system
        ) noexcept;
        ~IceshardEngine() noexcept override = default;

        auto create_runner(
            ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
        ) noexcept -> ice::UniquePtr<EngineRunner> override;

        auto entity_index() noexcept -> ice::EntityIndex& override;

        auto asset_system() noexcept -> ice::AssetSystem& override;

        auto world_manager() noexcept -> ice::WorldManager& override;

    private:
        ice::Allocator& _allocator;
        ice::AssetSystem& _asset_system;
        ice::EntityIndex _entity_index;
        ice::IceWorldManager _world_manager;
    };

} // namespace ice
