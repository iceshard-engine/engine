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

    bool AssetConfigObject::Has(core::StringView key) const noexcept
    {
        return _implementation->value.HasMember(core::string::data(key));
    }

    auto AssetConfigObject::StringValue(core::StringView key) const noexcept -> core::StringView
    {
        auto const* cstr = core::string::data(key);
        IS_ASSERT(_implementation->value.HasMember(cstr), "Key does not exist in config file.");
        IS_ASSERT(_implementation->value[cstr].IsString(), "Not a string value!");
        return _implementation->value[cstr].GetString();
    }

    auto AssetConfigObject::ObjectValue(core::StringView key) const noexcept -> AssetConfigObject
    {
        auto const* cstr = core::string::data(key);
        IS_ASSERT(_implementation->value.HasMember(cstr), "Key does not exist in config file.");
        IS_ASSERT(_implementation->value[cstr].IsObject(), "Not a object value!");
        return { _allocator, _allocator.make<ConfigImpl>(_implementation->document, _implementation->value[cstr]) };
    }

    AssetConfigObject::AssetConfigObject(core::allocator& alloc, ConfigImpl* data) noexcept
        : _allocator{ alloc }
        , _implementation{ data }
    {
    }

} // namespace asset
