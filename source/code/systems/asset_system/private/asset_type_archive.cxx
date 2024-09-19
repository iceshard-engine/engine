/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/config.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto default_asset_state(void*, ice::AssetCategoryDefinition const&, ice::Config const& metadata, ice::URI const&) noexcept -> ice::AssetState
    {
        bool is_pre_baked = false;
        ice::config::get(metadata, "asset.state.baked", is_pre_baked);
        return is_pre_baked ? AssetState::Baked : AssetState::Raw;
    }

    struct InternalAssetCategory
    {
        ice::AssetCategory category = ice::make_asset_category("<unknown>");
        ice::AssetCategoryDefinition definition{ };
        ice::ResourceCompiler compiler;
        bool has_compiler;
    };

    class SimpleAssetCategoryArchive final : public ice::AssetCategoryArchive
    {
    public:
        SimpleAssetCategoryArchive(ice::Allocator& alloc) noexcept;

        auto categories() const noexcept -> ice::Span<ice::AssetCategory const> override;

        auto find_definition(
            ice::AssetCategory_Arg category
        ) const noexcept -> ice::AssetCategoryDefinition const& override;

        auto find_compiler(
            ice::AssetCategory_Arg category
        ) const noexcept -> ice::ResourceCompiler const* override;

        bool register_category(
            ice::AssetCategory_Arg category,
            ice::AssetCategoryDefinition category_definition,
            ice::ResourceCompiler const* compiler
        ) noexcept override;

    private:
        ice::Array<ice::AssetCategory> _types;
        ice::HashMap<ice::InternalAssetCategory> _definitions;
    };

    SimpleAssetCategoryArchive::SimpleAssetCategoryArchive(ice::Allocator& alloc) noexcept
        : _types{ alloc }
        , _definitions{ alloc }
    {
    }

    auto SimpleAssetCategoryArchive::categories() const noexcept -> ice::Span<ice::AssetCategory const>
    {
        return _types;
    }

    bool SimpleAssetCategoryArchive::register_category(
        ice::AssetCategory_Arg category,
        ice::AssetCategoryDefinition type_definition,
        ice::ResourceCompiler const* compiler
    ) noexcept
    {
        ice::u64 const type_hash = category.identifier;
        bool const type_not_defined = ice::hashmap::has(_definitions, type_hash) == false;

        ICE_ASSERT(
            type_not_defined == true,
            "Identifier [{}] of asset category '{}' already has an associated definition!",
            category.identifier,
            ice::asset_category_hint(category)
        );

        if (type_not_defined)
        {
            if (type_definition.fn_asset_state == nullptr)
            {
                type_definition.fn_asset_state = default_asset_state;
            }

            ice::ResourceCompiler asset_compiler{};
            if (compiler != nullptr && compiler->fn_supported_resources)
            {
                for (ice::String ext : compiler->fn_supported_resources(type_definition.asset_params))
                {
                    for (ice::String asset_ext : type_definition.resource_extensions)
                    {
                        // Only use the compiler if at least one extension is covered.
                        if (ext == asset_ext)
                        {
                            asset_compiler = *compiler;
                            break;
                        }
                    }
                }
            }

            ice::array::push_back(_types, category);
            ice::hashmap::set(
                _definitions,
                type_hash,
                InternalAssetCategory{
                    .category = category,
                    .definition = ice::move(type_definition),
                    .compiler = asset_compiler,
                    .has_compiler = asset_compiler.fn_supported_resources != nullptr
                }
            );
        }
        return type_not_defined;
    }

    auto SimpleAssetCategoryArchive::find_definition(
        ice::AssetCategory_Arg category
    ) const noexcept -> ice::AssetCategoryDefinition const&
    {
        static ice::InternalAssetCategory empty_type{};

        ice::InternalAssetCategory const& internal_type = ice::hashmap::get(_definitions, category.identifier, empty_type);
        return internal_type.definition;
    }

    auto SimpleAssetCategoryArchive::find_compiler(
        ice::AssetCategory_Arg category
    ) const noexcept -> ice::ResourceCompiler const*
    {
        static ice::InternalAssetCategory empty_type{};

        ice::InternalAssetCategory const& internal_type = ice::hashmap::get(_definitions, category.identifier, empty_type);
        return internal_type.has_compiler ? &internal_type.compiler : nullptr;
    }

    auto create_asset_category_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetCategoryArchive>
    {
        return ice::make_unique<ice::SimpleAssetCategoryArchive>(alloc, alloc);
    }

} // namespace ice
