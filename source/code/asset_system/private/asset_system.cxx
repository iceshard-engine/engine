#include <asset_system/asset_system.hxx>
#include <asset_system/asset_resolver.hxx>
#include <resource/resource_messages.hxx>
#include <resource/resource_meta.hxx>

namespace asset
{

    namespace detail
    {

        class ConfigAssetResolver : public AssetResolver
        {
        public:
            auto resolve_asset_type(core::StringView extension, resource::ResourceMetaView const&) noexcept -> asset::AssetType override
            {
                AssetType result = AssetType::Unresolved;
                if (core::string::equals(extension, ".json"))
                {
                    result = AssetType::Config;
                }
                return result;
            }
        };

    } // namespace detail

    AssetSystem::AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept
        : _allocator{ alloc }
        , _resource_system{ resource_system }
        , _resource_database{ _allocator }
        , _asset_objects{ _allocator }
    {
        add_resolver(core::memory::make_unique<asset::AssetResolver, asset::detail::ConfigAssetResolver>(_allocator));
    }

    void AssetSystem::add_resolver(core::memory::unique_pointer<asset::AssetResolver> resolver) noexcept
    {
        _asset_resolver.push_back(std::move(resolver));
    }

    void AssetSystem::update() noexcept
    {
        // clang-format off
        core::message::filter<resource::message::ResourceAdded>(_resource_system.messages(), [&](resource::message::ResourceAdded const& msg) noexcept
            // clang-format on
            {
                auto extension_pos = core::string::find_first_of(msg.native_name, '.');

                auto basename = core::string::substr(msg.native_name, 0, extension_pos);
                auto extension = core::string::substr(msg.native_name, extension_pos);

                auto it = _asset_resolver.begin();
                auto const end = _asset_resolver.end();

                AssetType resolved_asset_type = AssetType::Unresolved;
                while (resolved_asset_type == AssetType::Unresolved && it != end)
                {
                    resolved_asset_type = (*it)->resolve_asset_type(extension, create_meta_view(resource::ResourceMeta{ _allocator }));
                    it = std::next(it);
                }

                // Resource is not a known asset!
                if (resolved_asset_type == AssetType::Unresolved)
                {
                    // #todo verbose message?
                    return;
                }

                Asset reference{ basename, resolved_asset_type };

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
    }

    auto AssetSystem::update(Asset reference, resource::URI content_location) noexcept -> AssetStatus
    {
        auto* const resource = _resource_system.find(content_location);
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
            auto* const resource_object = _resource_system.find(asset_resources.content_location);
            data.content = resource_object->data();
            data.metadata = resource_object->metadata();

            // #todo Remove this const cast.
            const_cast<AssetObject&>(asset_object->value).status = AssetStatus::Loaded;
        }

        return asset_object == nullptr ? AssetStatus::Invalid : asset_object->value.status;
    }

} // namespace asset
