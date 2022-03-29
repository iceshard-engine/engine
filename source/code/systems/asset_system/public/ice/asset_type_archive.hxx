#pragma once
#include <ice/span.hxx>
#include <ice/asset_type.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_types.hxx>
#include <ice/userdata.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/func.hxx>

namespace ice
{

    struct AssetTypeDefinition
    {
        ice::Span<ice::Utf8String const> resource_extensions;

        ice::Fn<bool(ice::FnUserdata, ice::AssetTypeDefinition const&, ice::Metadata const&) noexcept> fn_asset_type_resolver;
        ice::Fn<bool(ice::FnUserdata, ice::Allocator&, ice::Resource_v2 const&, ice::Data, ice::Memory&) noexcept> fn_asset_oven;
        ice::Fn<bool(ice::FnUserdata, ice::Allocator&, ice::Metadata const&, ice::Data, ice::Memory&) noexcept> fn_asset_loader;
    };

    class AssetTypeArchive
    {
    public:
        virtual ~AssetTypeArchive() = default;

        virtual auto asset_types() const noexcept -> ice::Span<ice::AssetType_v2 const> = 0;

        virtual auto find_definition(
            ice::AssetType_v2_Arg type
        ) const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual bool register_type(
            ice::AssetType_v2_Arg type,
            ice::AssetTypeDefinition type_definition
        ) noexcept = 0;
    };

    auto create_asset_type_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetTypeArchive>;

} // namespace ice
