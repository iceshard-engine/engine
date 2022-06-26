#pragma once
#include <ice/span.hxx>
#include <ice/task.hxx>
#include <ice/asset_type.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_types.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/func.hxx>

namespace ice
{

    class AssetStorage;

    enum class AssetState : ice::u32;

    struct AssetTypeDefinition
    {
        ice::Span<ice::Utf8String const> resource_extensions;

        ice::Fn<
            auto(
                ice::FnUserdata,
                ice::AssetTypeDefinition const&,
                ice::Metadata const&,
                ice::URI const&
            ) noexcept -> ice::AssetState
        > fn_asset_state;

        ice::Fn<
            ice::Task<bool>(
                ice::FnUserdata,
                ice::Allocator&,
                ice::ResourceTracker const&,
                ice::Resource_v2 const&,
                ice::Data,
                ice::Memory&
            ) noexcept
        > fn_asset_oven;

        ice::Fn<
            ice::Task<bool>(
                ice::FnUserdata,
                ice::Allocator&,
                ice::AssetStorage&,
                ice::Metadata const&,
                ice::Data,
                ice::Memory&
            ) noexcept
        > fn_asset_loader;
    };

    class AssetTypeArchive
    {
    public:
        virtual ~AssetTypeArchive() = default;

        virtual auto asset_types() const noexcept -> ice::Span<ice::AssetType const> = 0;

        virtual auto find_definition(
            ice::AssetType_Arg type
        ) const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual bool register_type(
            ice::AssetType_Arg type,
            ice::AssetTypeDefinition type_definition
        ) noexcept = 0;
    };

    auto create_asset_type_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetTypeArchive>;

} // namespace ice
