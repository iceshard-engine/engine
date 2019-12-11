#include <asset_system/assets/asset_config.hxx>
#include <rapidjson/document.h>

namespace asset
{

    struct AssetConfigObject::ConfigImpl
    {
        ConfigImpl() noexcept
            : document{ std::make_shared<rapidjson::Document>() }
            , value{ *document.get() }
        {
        }

        ConfigImpl(std::shared_ptr<rapidjson::Document> doc, rapidjson::Value const& value) noexcept
            : document{ doc }
            , value{ value }
        {
        }

        std::shared_ptr<rapidjson::Document> document;
        rapidjson::Value const& value;
    };

    AssetConfigObject::AssetConfigObject(core::allocator& alloc, AssetData data) noexcept
        : _allocator{ alloc }
        , _implementation{ _allocator.make<AssetConfigObject::ConfigImpl>() }
    {
        _implementation->document->Parse(reinterpret_cast<char const*>(data.content.data()), data.content.size());
    }

    AssetConfigObject::~AssetConfigObject() noexcept
    {
        _allocator.destroy(_implementation);
    }

    bool AssetConfigObject::Has(core::StringView<> key) const noexcept
    {
        return _implementation->value.HasMember(key._data);
    }

    auto AssetConfigObject::StringValue(core::StringView<> key) const noexcept -> core::StringView<>
    {
        IS_ASSERT(_implementation->value.HasMember(key._data), "Key does not exist in config file.");
        IS_ASSERT(_implementation->value[key._data].IsString(), "Not a string value!");
        return _implementation->value[key._data].GetString();
    }

    auto AssetConfigObject::ObjectValue(core::StringView<> key) const noexcept -> AssetConfigObject
    {
        IS_ASSERT(_implementation->value.HasMember(key._data), "Key does not exist in config file.");
        IS_ASSERT(_implementation->value[key._data].IsObject(), "Not a object value!");
        return { _allocator, _allocator.make<ConfigImpl>(_implementation->document, _implementation->value[key._data]) };
    }

    AssetConfigObject::AssetConfigObject(core::allocator& alloc, ConfigImpl* data) noexcept
        : _allocator{ alloc }
        , _implementation{ data }
    {
    }

} // namespace asset
