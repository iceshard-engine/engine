#pragma once
#include <core/string_view.hxx>
#include <asset_system/asset.hxx>

namespace asset
{

    struct AssetConfig : Asset
    {
        AssetConfig() noexcept = default;

        constexpr AssetConfig(core::StringView<> view) noexcept
            : Asset{ view, AssetType::Config }
        {
        }
    };

    class AssetConfigObject
    {
    public:
        AssetConfigObject(core::allocator& alloc, AssetData data) noexcept;
        ~AssetConfigObject() noexcept;

        auto StringValue(core::StringView<> key) const noexcept -> core::StringView<>;

    private:
        struct ConfigValue;
        core::pod::Hash<ConfigValue*> _values;
    };

} // namespace asset
