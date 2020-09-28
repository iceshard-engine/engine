#pragma once
#include <resource/resource_meta.hxx>
#include <resource/resource_system.hxx>
#include <asset_system/asset.hxx>

namespace asset
{

    struct ResourceMetaView;

    enum class AssetCompilationStatus : uint32_t
    {
        Success = 0x0,
        Failed = 0x1,
        Failed_InvalidData,
        Failed_MissingDependencies,
    };

    struct AssetCompilationResult
    {
        AssetCompilationResult(core::allocator& alloc) noexcept;
        ~AssetCompilationResult() noexcept = default;

        core::Buffer data;
        resource::ResourceMeta metadata;
        core::pod::Array<Asset> dependencies;
    };

    class AssetCompiler
    {
    public:
        virtual ~AssetCompiler() noexcept = default;

        virtual auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& = 0;

        virtual auto compile_asset(
            core::allocator& alloc,
            resource::ResourceSystem& resource_system,
            asset::Asset asset,
            core::data_view resource_data,
            resource::ResourceMetaView const& asset_meta,
            AssetCompilationResult& result_out
        ) noexcept -> AssetCompilationStatus = 0;
    };

} // namespace asset
