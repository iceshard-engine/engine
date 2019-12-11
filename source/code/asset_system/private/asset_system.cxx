#include <asset_system/asset_system.hxx>
#include <resource/resource_messages.hxx>

namespace asset
{

    namespace detail
    {

        //auto find_json_asset(resource::ResourceSystem& resource_system) noexcept -> Asset
        //{

        //}

    } // namespace detail

    AssetSystem::AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept
        : _allocator{ alloc }
        , _resource_system{ resource_system }
        , _resource_database{ _allocator }
        , _asset_objects{ _allocator }
    {
    }

    void AssetSystem::update() noexcept
    {
        // clang-format off
        core::message::filter<resource::message::ResourceAdded>(_resource_system.messages(), [&](resource::message::ResourceAdded const& msg) noexcept
            {
                auto name_end = core::string::begin(msg.native_name);
                auto name_begin = name_end;
                auto const end = core::string::end(msg.native_name);

                while (name_end != end && *name_end != '.')
                {
                    name_end++;
                }

                auto basename = core::StringView<>{ name_begin, name_end };
                auto extension = core::StringView<>{ name_end, end };

                Asset reference{ basename, AssetType::Resource };
                if (core::string::equals(extension, ".json"))
                {
                    reference.type = AssetType::Config;
                }

                // clang-format off
                core::pod::multi_hash::insert(
                    _asset_objects,
                    static_cast<uint64_t>(reference.name.hash_value),
                    AssetObject{ AssetStatus::Available, core::pod::array::size(_resource_database), std::move(reference) }
                );
                core::pod::array::push_back(
                    _resource_database,
                    AssetReference{ msg.location }
                );
            // clang-format on
        });
        // clang-format on
    }

    auto AssetSystem::update(Asset reference, resource::URI content_location) noexcept -> AssetStatus
    {
        auto const* const resource = _resource_system.find(content_location);
        if (resource != nullptr)
        {
            // clang-format off
            core::pod::multi_hash::insert(
                _asset_objects,
                static_cast<uint64_t>(reference.name.hash_value),
                AssetObject{ AssetStatus::Available, core::pod::array::size(_resource_database), std::move(reference) }
            );
            core::pod::array::push_back(
                _resource_database,
                AssetReference{ resource->location() }
            );
            // clang-format on
        }
        return resource == nullptr ? AssetStatus::Invalid : AssetStatus::Available;
    }

    auto AssetSystem::load(Asset reference, AssetData& data) noexcept -> AssetStatus
    {
        auto asset_object = core::pod::multi_hash::find_first(_asset_objects, static_cast<uint64_t>(reference.name.hash_value));
        while (asset_object != nullptr && asset_object->value.reference.type != reference.type)
        {
            asset_object = core::pod::multi_hash::find_next(_asset_objects, asset_object);
        }

        if (asset_object != nullptr)
        {
            auto const& asset_resources = _resource_database[asset_object->value.resource];
            data.content = _resource_system.find(asset_resources.content_location)->data();
            data.metadata = core::data_view{ nullptr, 0 };

            const_cast<AssetObject&>(asset_object->value).status = AssetStatus::Loaded;
        }

        return asset_object == nullptr ? AssetStatus::Invalid : asset_object->value.status;
    }

} // namespace asset
