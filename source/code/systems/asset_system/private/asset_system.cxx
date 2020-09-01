#include <asset_system/asset_system.hxx>
#include <asset_system/asset_resolver.hxx>
#include <resource/resource_messages.hxx>
#include <resource/resource_meta.hxx>

template<>
constexpr auto core::hash<asset::AssetResolverHandle>(asset::AssetResolverHandle handle) noexcept -> uint64_t
{
    return 0;
}

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
        , _asset_resolvers{ _allocator }
        , _asset_loaders{ _allocator }
        , _asset_loader_map{ _allocator }
    {
        add_resolver(core::memory::make_unique<asset::AssetResolver, asset::detail::ConfigAssetResolver>(_allocator));
    }

    auto AssetSystem::add_resolver(
        core::memory::unique_pointer<asset::AssetResolver> resolver
    ) noexcept -> AssetResolverHandle
    {
        _next_resolver_handle += 1;
        AssetResolverHandle resolver_handle{ _next_resolver_handle };

        _asset_resolvers.emplace(resolver_handle, std::move(resolver));
        return resolver_handle;
    }

    void AssetSystem::remove_resolver(asset::AssetResolverHandle resolver_handle) noexcept
    {
        auto const it = _asset_resolvers.find(resolver_handle);
        if (it != _asset_resolvers.end())
        {
            _asset_resolvers.erase(it);
        }
    }

    auto AssetSystem::add_loader(
        asset::AssetType asset_type,
        core::memory::unique_pointer<asset::AssetLoader> loader
    ) noexcept -> AssetLoaderHandle
    {
        _next_loader_handle += 1;
        AssetLoaderHandle resolver_handle{ _next_loader_handle };

        for (auto asset_type : loader->supported_asset_types())
        {
            _asset_loader_map[asset_type].push_back(loader.get());
        }

        _asset_loaders.emplace(resolver_handle, std::move(loader));
        return resolver_handle;
    }

    void AssetSystem::remove_loader(asset::AssetLoaderHandle loader_handle) noexcept
    {
        auto const it = _asset_loaders.find(loader_handle);
        if (it != _asset_loaders.end())
        {
            auto const& element_to_remove = it->second;

            for (auto asset_type : element_to_remove->supported_asset_types())
            {
                auto& loader_vector = _asset_loader_map[asset_type];

                auto loader_to_remove = std::find(loader_vector.begin(), loader_vector.end(), element_to_remove.get());
                loader_vector.erase(loader_to_remove);
            }

            _asset_loaders.erase(it);
        }
    }

    void AssetSystem::update() noexcept
    {
        // clang-format off
        core::message::filter<resource::message::ResourceAdded>(_resource_system.messages(), [&](resource::message::ResourceAdded const& msg) noexcept
            // clang-format on
            {
                auto native_name = msg.resource->name();
                auto extension_pos = core::string::find_first_of(native_name, '.');

                auto basename = core::string::substr(native_name, 0, extension_pos);
                auto extension = core::string::substr(native_name, extension_pos);

                auto it = _asset_resolvers.begin();
                auto const end = _asset_resolvers.end();

                AssetType resolved_asset_type = AssetType::Unresolved;
                while (resolved_asset_type == AssetType::Unresolved && it != end)
                {
                    resolved_asset_type = it->second->resolve_asset_type(extension, msg.resource->metadata());
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
                    AssetObject{ core::pod::array::size(_resource_database), std::move(reference) }
                );
                core::pod::array::push_back(
                    _resource_database,
                    AssetReference{ msg.resource->location(), msg.resource, AssetStatus::Available }
                );
                // clang-format on
            });
    }

    auto AssetSystem::update(Asset reference, resource::URI content_location) noexcept -> AssetStatus
    {
        auto* const resource = _resource_system.find(content_location);
        if (resource != nullptr)
        {
            auto const name_hash = core::hash(reference.name);

            int32_t removed_resource_index = -1;

            // Remove same reference
            auto* entry = core::pod::multi_hash::find_first(_asset_objects, name_hash);
            while (entry != nullptr)
            {
                if (entry->value.reference.type == reference.type)
                {
                    removed_resource_index = entry->value.resource_index;
                    core::pod::multi_hash::remove(_asset_objects, entry);
                    break;
                }
                entry = core::pod::multi_hash::find_next(_asset_objects, entry);
            }

            if (removed_resource_index >= 0)
            {
                core::pod::multi_hash::insert(
                    _asset_objects,
                    name_hash,
                    AssetObject{ core::pod::array::size(_resource_database), std::move(reference) }
                );
                core::pod::array::push_back(
                    _resource_database,
                    AssetReference{ resource->location(), resource, AssetStatus::Unloading }
                );
            }
            else
            {
                core::pod::multi_hash::insert(
                    _asset_objects,
                    name_hash,
                    AssetObject{ static_cast<uint64_t>(removed_resource_index), std::move(reference) }
                );
                _resource_database[removed_resource_index] = AssetReference{
                    resource->location(),
                    resource,
                    AssetStatus::Available
                };
            }
        }
        return resource == nullptr ? AssetStatus::Invalid : AssetStatus::Available;
    }

    auto AssetSystem::load(Asset ref, AssetData& result_data) noexcept -> AssetStatus
    {
        if (_asset_loader_map.contains(ref.type) == false || _asset_loader_map[ref.type].empty())
        {
            // No loader available
            return AssetStatus::Invalid;
        }

        // Get the requested asset object
        auto asset_object = core::pod::multi_hash::find_first(_asset_objects, static_cast<uint64_t>(ref.name.hash_value));
        while (asset_object != nullptr && asset_object->value.reference.type != ref.type)
        {
            asset_object = core::pod::multi_hash::find_next(_asset_objects, asset_object);
        }

        if (asset_object == nullptr)
        {
            // Asset does not exist
            return AssetStatus::Invalid;
        }

        // Try load the asset
        auto it = _asset_loader_map[ref.type].rbegin();
        auto const it_end = _asset_loader_map[ref.type].rend();

        AssetStatus load_status = AssetStatus::Invalid;
        while (it != it_end && load_status == AssetStatus::Invalid) // We can assume its either Invalid or Loaded
        {
            auto& asset_resources = _resource_database[asset_object->value.resource_index];

            // If the current asset is already unloading, release it
            // This should only happen in a single case, when an asset is reloading
            //  after an update for such an asset was requested.
            if (asset_resources.status == asset::AssetStatus::Unloading)
            {
                (*it)->release_asset(asset_object->value.reference);
            }

            if (asset_resources.resource_object == nullptr)
            {
                asset_resources.resource_object = _resource_system.find(asset_resources.content_location);
            }

            if (asset_resources.resource_object != nullptr)
            {
                load_status = (*it++)->load_asset(
                    asset_object->value.reference,
                    asset_resources.resource_object->metadata(),
                    asset_resources.resource_object->data(),
                    result_data
                );

                asset_resources.status = load_status;
            }
            else
            {
                asset_resources.status = AssetStatus::Invalid;
            }
        }

        return load_status;
    }

} // namespace asset
