#pragma once
#include <core/string_view.hxx>
#include <core/pod/hash.hxx>
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

        bool Has(core::StringView<> key) const noexcept;

        auto StringValue(core::StringView<> key) const noexcept -> core::StringView<>;

        auto ObjectValue(core::StringView<> key) const noexcept -> AssetConfigObject;

    private:
        struct ConfigImpl;

        AssetConfigObject(core::allocator& alloc, ConfigImpl* data) noexcept;

    private:
        core::allocator& _allocator;
        ConfigImpl* _implementation = nullptr;
    };

} // namespace asset
