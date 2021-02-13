#pragma once
#include <ice/engine.hxx>
#include <ice/asset_system.hxx>

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

        auto create_runner() noexcept -> ice::UniquePtr<EngineRunner> override;

    private:
        ice::Allocator& _allocator;
        ice::AssetSystem& _asset_system;
    };

} // namespace ice
