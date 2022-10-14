#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/stringid.hxx>
#include <ice/string_types.hxx>
#include <ice/resource_flags.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr bool Constant_UseAssetTypeDebugDefinition = ice::build::is_debug || ice::build::is_develop;

        template<bool DebugImpl>
        struct AssetType
        {
            using ArgType = AssetType;

            ice::u64 identifier;
        };

        template<>
        struct AssetType<true>
        {
            using ArgType = const AssetType&;

            ice::u64 identifier;
            ice::String name;
        };

        constexpr auto make_asset_type(ice::String name) noexcept
        {
            ice::detail::murmur2_hash::mm2_x64_64 const result =
                ice::detail::murmur2_hash::cexpr_murmur2_x64_64(name, 0xAA44EELL);

            if constexpr (Constant_UseAssetTypeDebugDefinition)
            {
                return ice::detail::AssetType<Constant_UseAssetTypeDebugDefinition>
                {
                    .identifier = result.h[0],
                    .name = name
                };
            }
            else
            {
                return ice::detail::AssetType<Constant_UseAssetTypeDebugDefinition>{
                    .identifier = result.h[0]
                };
            }
        }

        struct AssetTypePicker
        {
            using AssetType_Type = AssetType<ice::build::is_debug || ice::build::is_develop>;

            using AssetType_Arg = AssetType_Type::ArgType;
        };

    } // namespace detail

    using AssetType = ice::detail::AssetTypePicker::AssetType_Type;
    using AssetType_Arg = ice::detail::AssetTypePicker::AssetType_Arg;

    constexpr auto make_asset_type(ice::String name) noexcept -> ice::AssetType
    {
        return ice::detail::make_asset_type(name);
    }

    constexpr auto asset_type_hint(ice::detail::AssetType<false>) noexcept -> ice::String
    {
        return "<no_debug_info_available>";
    }

    constexpr auto asset_type_hint(ice::detail::AssetType<true> const& type) noexcept -> ice::String
    {
        return type.name;
    }

} // namespace ice
