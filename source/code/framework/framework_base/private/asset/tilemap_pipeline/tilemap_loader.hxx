#pragma once
#include <ice/asset_loader.hxx>

namespace ice
{

    class IceTiledAssetLoader : public ice::AssetLoader
    {
    public:
        auto load(
            ice::AssetSolver& asset_solver,
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) const noexcept -> ice::AssetStatus override;
    };

} // namespace ice