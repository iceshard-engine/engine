#include <ice/asset_type_archive.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    bool unknown_asset_type_resolver(void*, ice::AssetTypeDefinition const&, ice::Metadata const&) noexcept
    {
        return false;
    }

    struct InternalAssetType
    {
        ice::AssetType_v2 type = ice::make_asset_type(u8"<unknown>");
        ice::AssetTypeDefinition definition{
            .fn_asset_type_resolver = unknown_asset_type_resolver
        };
    };

    class SimpleAssetTypeArchive final : public ice::AssetTypeArchive
    {
    public:
        SimpleAssetTypeArchive(ice::Allocator& alloc) noexcept;

        auto asset_types() const noexcept -> ice::Span<ice::AssetType_v2 const>;

        bool register_type(
            ice::AssetType_v2_Arg type,
            ice::AssetTypeDefinition type_definition
        ) noexcept override;

        auto find_definition(
            ice::AssetType_v2_Arg type
        ) const noexcept -> ice::AssetTypeDefinition const& override;

    private:
        ice::pod::Array<ice::AssetType_v2> _types;
        ice::pod::Hash<ice::InternalAssetType> _definitions;
    };

    SimpleAssetTypeArchive::SimpleAssetTypeArchive(ice::Allocator& alloc) noexcept
        : _types{ alloc }
        , _definitions{ alloc }
    {
    }

    auto SimpleAssetTypeArchive::asset_types() const noexcept -> ice::Span<ice::AssetType_v2 const>
    {
        return _types;
    }

    bool SimpleAssetTypeArchive::register_type(
        ice::AssetType_v2_Arg type,
        ice::AssetTypeDefinition type_definition
    ) noexcept
    {
        ice::u64 const type_hash = type.identifier;
        bool const type_not_defined = ice::pod::hash::has(_definitions, type_hash) == false;

        ICE_ASSERT(
            type_not_defined == true,
            "Identifier [{}] of asset type '{}' already has an associated definition!",
            type.identifier,
            "<utf8_strings_not_supported_yet>"
            //ice::asset_type_hint(type)
        );

        if (type_not_defined)
        {
            ice::pod::array::push_back(_types, type);
            ice::pod::hash::set(
                _definitions,
                type_hash,
                InternalAssetType{
                    .type = type,
                    .definition = ice::move(type_definition)
                }
            );
        }
        return type_not_defined;
    }

    auto SimpleAssetTypeArchive::find_definition(
        ice::AssetType_v2_Arg type
    ) const noexcept -> ice::AssetTypeDefinition const&
    {
        static ice::InternalAssetType empty_type{};

        ice::InternalAssetType const& internal_type = ice::pod::hash::get(_definitions, type.identifier, empty_type);
        return internal_type.definition;
    }

    auto create_asset_type_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetTypeArchive>
    {
        return ice::make_unique<ice::AssetTypeArchive, ice::SimpleAssetTypeArchive>(alloc, alloc);
    }

} // namespace ice
