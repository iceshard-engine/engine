/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/asset_types.hxx>
#include <ice/asset_category.hxx>
#include <ice/resource_compiler.hxx>

namespace ice
{

    struct AssetCategoryDefinition
    {
        constexpr auto valid() const noexcept { return ice::span::any(resource_extensions); }

        ice::String name;
        ice::Span<ice::Shard const> asset_params;
        ice::Span<ice::String const> resource_extensions;

        auto(*fn_asset_state)(
            void*,
            ice::AssetCategoryDefinition const&,
            ice::Config const&,
            ice::URI const&
        ) noexcept -> ice::AssetState;

        ice::Task<bool>(*fn_asset_loader)(
            void*,
            ice::Allocator&,
            ice::AssetStorage&,
            ice::Config const&,
            ice::Data,
            ice::Memory&
        ) noexcept;

        auto(*fn_extension_for_state)(
            void*,
            ice::AssetState
        ) noexcept -> ice::String;

        void* ud_asset_state;
        void* ud_asset_loader;
        void* ud_asset_extension;
    };

    class AssetCategoryArchive
    {
    public:
        virtual ~AssetCategoryArchive() = default;

        virtual auto categories() const noexcept -> ice::Span<ice::AssetCategory const> = 0;

        virtual auto find_definition(
            ice::AssetCategory_Arg category
        ) const noexcept -> ice::AssetCategoryDefinition const& = 0;

        virtual auto find_compiler(
            ice::AssetCategory_Arg category
        ) const noexcept -> ice::ResourceCompiler const* = 0;

        virtual bool register_category(
            ice::AssetCategory_Arg category,
            ice::AssetCategoryDefinition category_definition,
            ice::ResourceCompiler const* compiler = nullptr
        ) noexcept = 0;
    };

    auto create_asset_category_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetCategoryArchive>;

} // namespace ice
