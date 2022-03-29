#pragma once
#include <ice/allocator.hxx>
#include <ice/base.hxx>
#include <ice/stringid.hxx>
#include <ice/string_types.hxx>
#include <ice/resource_flags.hxx>

namespace ice
{

    struct Asset_v2;

    namespace detail
    {

        static constexpr bool Constant_UseAssetTypeDebugDefinition = ice::build::is_debug || ice::build::is_develop;

        template<bool DebugImpl>
        struct AssetType_v2
        {
            using ArgType = AssetType_v2;

            ice::u64 identifier;
        };

        template<>
        struct AssetType_v2<true>
        {
            using ArgType = const AssetType_v2&;

            ice::u64 identifier;
            ice::Utf8String name;
        };

        constexpr auto make_asset_type(ice::Utf8String name) noexcept
        {
            ice::detail::murmur2_hash::mm2_x64_64 const result =
                ice::detail::murmur2_hash::cexpr_murmur2_x64_64(name, 0xAA44EELL);

            if constexpr (Constant_UseAssetTypeDebugDefinition)
            {
                return ice::detail::AssetType_v2<true>
                {
                    .identifier = result.h[0],
                    .name = name
                };
            }
            else
            {
                return ice::detail::AssetType_v2<false>{
                    .identifier = result.h[0]
                };
            }
        }

        struct AssetTypePicker
        {
            using AssetType = AssetType_v2<ice::build::is_debug || ice::build::is_develop>;

            using AssetType_Arg = AssetType_v2<ice::build::is_debug || ice::build::is_develop>::ArgType;
        };

    } // namespace detail

    using AssetType_v2 = ice::detail::AssetTypePicker::AssetType;
    using AssetType_v2_Arg = ice::detail::AssetTypePicker::AssetType_Arg;

    constexpr auto make_asset_type(ice::Utf8String name) noexcept -> ice::AssetType_v2
    {
        return ice::detail::make_asset_type(name);
    }

    constexpr auto asset_type_hint(ice::detail::AssetType_v2<false>) noexcept -> ice::Utf8String
    {
        return u8"<no_debug_info_available>";
    }

    constexpr auto asset_type_hint(ice::detail::AssetType_v2<true> const& type) noexcept -> ice::Utf8String
    {
        return type.name;
    }

} // namespace ice
