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
        AssetResolverHandle resolver_handle{ static_cast<uint32_t>(_asset_resolvers.size()) };
        _asset_resolvers.push_back(std::move(resolver));
        return resolver_handle;
    }

    void AssetSystem::remove_resolver(asset::AssetResolverHandle resolver_handle) noexcept
    {
        uint32_t const resolved_index = static_cast<uint32_t>(resolver_handle);
        if (resolved_index < _asset_resolvers.size())
        {
            auto const element_to_remove = std::next(_asset_resolvers.begin(), resolved_index);
            _asset_resolvers.erase(element_to_remove);
        }
    }

    auto AssetSystem::add_loader(
        asset::AssetType asset_type,
        core::memory::unique_pointer<asset::AssetLoader> loader
    ) noexcept -> AssetLoaderHandle
    {
        AssetLoaderHandle resolver_handle{ static_cast<uint32_t>(_asset_loaders.size()) };

        for (auto asset_type : loader->supported_asset_types())
        {
            _asset_loader_map[asset_type].push_back(loader.get());
        }

        _asset_loaders.push_back(std::move(loader));
        return resolver_handle;
    }

    void AssetSystem::remove_loader(asset::AssetLoaderHandle loader_handle) noexcept
    {
        uint32_t const resolved_index = static_cast<uint32_t>(loader_handle);
        if (resolved_index < _asset_loaders.size())
        {
            auto const element_to_remove = std::next(_asset_loaders.begin(), resolved_index);

            for (auto asset_type : (*element_to_remove)->supported_asset_types())
            {
                auto& loader_vector = _asset_loader_map[asset_type];

                auto loader_to_remove = std::find(loader_vector.begin(), loader_vector.end(), element_to_remove->get());
                loader_vector.erase(loader_to_remove);
            }

            _asset_loaders.erase(element_to_remove);
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
                    resolved_asset_type = (*it)->resolve_asset_type(extension, msg.resource->metadata());
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
                    AssetReference{ msg.resource->location() }
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
            auto const& asset_resources = _resource_database[asset_object->value.resource];
            auto* const resource_object = _resource_system.find(asset_resources.content_location);

            load_status = (*it++)->load_asset(asset_object->value.reference, resource_object->metadata(), resource_object->data(), result_data);

            // #todo Remove this const cast.
            const_cast<AssetObject&>(asset_object->value).status = load_status;
        }

        return load_status;
    }

} // namespace asset
