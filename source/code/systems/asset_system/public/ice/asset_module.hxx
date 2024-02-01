/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    class ModuleRegister;
    class AssetTypeArchive;

    void load_asset_type_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetTypeArchive& asset_type_archive
    ) noexcept;

    namespace detail::asset_system::v1
    {

        using RegisterTypesFn = void (ice::AssetTypeArchive&) noexcept;

        struct AssetTypeArchiveAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.asset-type-archive"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            RegisterTypesFn* register_types_fn;
            // TODO: Unregister or Reload function?
        };

    } // detail::engine::v1

} // namespace ice
