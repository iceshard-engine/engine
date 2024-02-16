/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/task.hxx>
#include <ice/asset_type.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_types.hxx>
#include <ice/resource_compiler.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    class AssetStorage;

    enum class AssetState : ice::u32;

    struct AssetTypeDefinition
    {
        ice::Span<ice::String const> resource_extensions;

        auto(*fn_asset_state)(
            void*,
            ice::AssetTypeDefinition const&,
            ice::Metadata const&,
            ice::URI const&
        ) noexcept -> ice::AssetState;

        // ice::Task<bool>(*fn_asset_oven)(
        //     void*,
        //     ice::Allocator&,
        //     ice::ResourceTracker const&,
        //     ice::LooseResource const&,
        //     ice::Data,
        //     ice::Memory&
        // ) noexcept;

        ice::Task<bool>(*fn_asset_loader)(
            void*,
            ice::Allocator&,
            ice::AssetStorage&,
            ice::Metadata const&,
            ice::Data,
            ice::Memory&
        ) noexcept;

        void* ud_asset_state;
        void* ud_asset_oven;
        void* ud_asset_loader;
    };

    class AssetTypeArchive
    {
    public:
        virtual ~AssetTypeArchive() = default;

        virtual auto asset_types() const noexcept -> ice::Span<ice::AssetType const> = 0;

        virtual auto find_definition(
            ice::AssetType_Arg type
        ) const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual auto find_compiler(
            ice::AssetType_Arg type
        ) const noexcept -> ice::ResourceCompiler const* = 0;

        virtual bool register_type(
            ice::AssetType_Arg type,
            ice::AssetTypeDefinition type_definition,
            ice::ResourceCompiler const* compiler = nullptr
        ) noexcept = 0;
    };

    auto create_asset_type_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::AssetTypeArchive>;

} // namespace ice
