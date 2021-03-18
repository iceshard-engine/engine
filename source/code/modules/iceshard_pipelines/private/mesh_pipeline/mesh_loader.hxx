#pragma once
#include <ice/asset_loader.hxx>

namespace ice
{

    class IceshardMeshLoader final : public ice::AssetLoader
    {
    public:
        auto load(
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) noexcept -> ice::AssetStatus override;
    };

} // namespace iceshard
