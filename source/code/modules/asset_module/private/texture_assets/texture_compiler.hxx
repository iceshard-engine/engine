#pragma once
#include <asset_system/asset_compiler.hxx>

namespace iceshard
{

    class StbTextureCompiler final : public asset::AssetCompiler
    {
    public:
        ~StbTextureCompiler() noexcept = default;

        auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& override;

        auto compile_asset(
            core::allocator& alloc,
            resource::ResourceSystem& resource_system,
            asset::Asset asset,
            core::data_view resource_data,
            resource::ResourceMetaView const& asset_meta,
            asset::AssetCompilationResult& result_out
        ) noexcept -> asset::AssetCompilationStatus override;
    };

} // namespace iceshard
