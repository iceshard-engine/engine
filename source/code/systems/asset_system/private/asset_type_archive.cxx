/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset_type_archive.hxx>
#include <ice/asset.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto default_asset_state(void*, ice::AssetTypeDefinition const&, ice::Metadata const& metadata, ice::URI const&) noexcept -> ice::AssetState
    {
        bool is_pre_baked = false;
        ice::meta_read_bool(metadata, "asset.state.pre-baked"_sid, is_pre_baked);
        return is_pre_baked ? AssetState::Baked : AssetState::Raw;
    }

    struct InternalAssetType
    {
        ice::AssetType type = ice::make_asset_type("<unknown>");
        ice::AssetTypeDefinition definition{ };
        ice::ResourceCompiler compiler;
        bool has_compiler;
    };

    class SimpleAssetTypeArchive final : public ice::AssetTypeArchive
    {
    public:
        SimpleAssetTypeArchive(ice::Allocator& alloc) noexcept;

        auto asset_types() const noexcept -> ice::Span<ice::AssetType const> override;

        auto find_definition(
            ice::AssetType_Arg type
        ) const noexcept -> ice::AssetTypeDefinition const& override;

        auto find_compiler(
            ice::AssetType_Arg type
        ) const noexcept -> ice::ResourceCompiler const* override;

        bool register_type(
            ice::AssetType_Arg type,
            ice::AssetTypeDefinition type_definition,
            ice::ResourceCompiler const* compiler
        ) noexcept override;

    private:
        ice::Array<ice::AssetType> _types;
        ice::HashMap<ice::InternalAssetType> _definitions;
    };

    SimpleAssetTypeArchive::SimpleAssetTypeArchive(ice::Allocator& alloc) noexcept
        : _types{ alloc }
        , _definitions{ alloc }
    {
    }

    auto SimpleAssetTypeArchive::asset_types() const noexcept -> ice::Span<ice::AssetType const>
    {
        return _types;
    }

    bool SimpleAssetTypeArchive::register_type(
        ice::AssetType_Arg type,
        ice::AssetTypeDefinition type_definition,
        ice::ResourceCompiler const* compiler
    ) noexcept
    {
        ice::u64 const type_hash = type.identifier;
        bool const type_not_defined = ice::hashmap::has(_definitions, type_hash) == false;

        ICE_ASSERT(
            type_not_defined == true,
            "Identifier [{}] of asset type '{}' already has an associated definition!",
            type.identifier,
            "<utf8_strings_not_supported_yet>"
            //ice::asset_type_hint(type)
        );

        if (type_not_defined)
        {
            if (type_definition.fn_asset_state == nullptr)
            {
                type_definition.fn_asset_state = default_asset_state;
            }

            ice::ResourceCompiler asset_compiler{};
            if (compiler != nullptr)
            {
                asset_compiler = *compiler;
            }

            ice::array::push_back(_types, type);
            ice::hashmap::set(
                _definitions,
                type_hash,
                InternalAssetType{
                    .type = type,
                    .definition = ice::move(type_definition),
                    .compiler = asset_compiler,
                    .has_compiler = compiler != nullptr
                }
            );
        }
        return type_not_defined;
    }

    auto SimpleAssetTypeArchive::find_definition(
        ice::AssetType_Arg type
    ) const noexcept -> ice::AssetTypeDefinition const&
    {
        static ice::InternalAssetType empty_type{};

        ice::InternalAssetType const& internal_type = ice::hashmap::get(_definitions, type.identifier, empty_type);
        return internal_type.definition;
    }

    auto SimpleAssetTypeArchive::find_compiler(
        ice::AssetType_Arg type
    ) const noexcept -> ice::ResourceCompiler const*
    {
        static ice::InternalAssetType empty_type{};

        ice::InternalAssetType const& internal_type = ice::hashmap::get(_definitions, type.identifier, empty_type);
        return internal_type.has_compiler ? &internal_type.compiler : nullptr;
    }

    auto create_asset_type_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetTypeArchive>
    {
        return ice::make_unique<ice::SimpleAssetTypeArchive>(alloc, alloc);
    }

} // namespace ice
