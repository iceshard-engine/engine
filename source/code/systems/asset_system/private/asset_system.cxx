#include <asset_system/asset_system.hxx>
#include <asset_system/asset_resolver.hxx>
#include <asset_system/asset_compiler.hxx>
#include <resource/resource_messages.hxx>
#include <resource/resource_meta.hxx>

namespace asset
{

    namespace detail
    {

        class ResourceAssetResolved final : public AssetResolver
        {
        public:
            bool resolve_asset_info(
                core::StringView extension,
                resource::ResourceMetaView const& meta,
                asset::AssetStatus& status_out,
                asset::AssetType& type_out
            ) noexcept override
            {
                if (core::string::equals(extension, ".isr"))
                {
                    int32_t asset_type_value = 0;
                    if (resource::get_meta_int32(meta, "asset.type"_sid, asset_type_value))
                    {
                        type_out = AssetType{ asset_type_value };
                        status_out = AssetStatus::Available;
                    }
                }
                return type_out != AssetType::Unresolved;
            }
        };

        class ConfigAssetResolver final : public AssetResolver
        {
        public:
            bool resolve_asset_info(
                core::StringView extension,
                resource::ResourceMetaView const& /*meta*/,
                asset::AssetStatus& status_out,
                asset::AssetType& type_out
            ) noexcept override
            {
                if (core::string::equals(extension, ".json"))
                {
                    type_out = AssetType::Config;
                    status_out = AssetStatus::Available;
                    return true;
                }
                return false;
            }
        };

    } // namespace detail

    AssetSystem::AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept
        : _allocator{ alloc }
        , _resource_system{ resource_system }
        , _resource_database{ _allocator }
        , _asset_objects{ _allocator }
        , _asset_resolvers{ _allocator }
        , _asset_compilers{ _allocator }
        , _asset_loaders{ _allocator }
        , _asset_compiler_map{ _allocator }
        , _asset_loader_map{ _allocator }
    {
        add_resolver(core::memory::make_unique<asset::AssetResolver, asset::detail::ResourceAssetResolved>(_allocator));
        add_resolver(core::memory::make_unique<asset::AssetResolver, asset::detail::ConfigAssetResolver>(_allocator));
    }

    AssetSystem::~AssetSystem() noexcept
    {
        for (AssetReference const& ref : _resource_database)
        {
            _allocator.destroy(ref.compiled_asset);
        }
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

    auto AssetSystem::add_compiler(
        core::memory::unique_pointer<asset::AssetCompiler> compiler
    ) noexcept -> asset::AssetCompilerHandle
    {
        _next_compiler_handle += 1;
        AssetCompilerHandle compiler_handle{ _next_compiler_handle };

        for (auto asset_type : compiler->supported_asset_types())
        {
            _asset_compiler_map[asset_type].push_back(compiler.get());
        }

        _asset_compilers.emplace(compiler_handle, std::move(compiler));
        return compiler_handle;
    }

    void AssetSystem::remove_compiler(asset::AssetCompilerHandle compiler_handle) noexcept
    {
        auto const it = _asset_compilers.find(compiler_handle);
        if (it != _asset_compilers.end())
        {
            auto const& element_to_remove = it->second;

            for (auto asset_type : element_to_remove->supported_asset_types())
            {
                auto& compiler_vector = _asset_compiler_map[asset_type];

                auto compiler_to_remove = std::find(compiler_vector.begin(), compiler_vector.end(), element_to_remove.get());
                compiler_vector.erase(compiler_to_remove);
            }

            _asset_compilers.erase(it);
        }
    }

    auto AssetSystem::add_loader(
        core::memory::unique_pointer<asset::AssetLoader> loader
    ) noexcept -> AssetLoaderHandle
    {
        _next_loader_handle += 1;
        AssetLoaderHandle loader_handle{ _next_loader_handle };

        for (auto asset_type : loader->supported_asset_types())
        {
            _asset_loader_map[asset_type].push_back(loader.get());
        }

        _asset_loaders.emplace(loader_handle, std::move(loader));
        return loader_handle;
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
        core::message::filter<resource::message::ResourceAdded>(_resource_system.messages(), [&](resource::message::ResourceAdded const& msg) noexcept
            {
                auto native_name = msg.resource->name();
                auto extension_pos = core::string::find_first_of(native_name, '.');

                auto basename = core::string::substr(native_name, 0, extension_pos);
                auto extension = core::string::substr(native_name, extension_pos);

                auto it = _asset_resolvers.begin();
                auto const end = _asset_resolvers.end();

                AssetType resolved_type = AssetType::Unresolved;
                AssetStatus resolved_status = AssetStatus::Invalid;

                bool asset_info_resolved = false;
                while (asset_info_resolved == false && it != end)
                {
                    asset_info_resolved = it->second->resolve_asset_info(
                        extension,
                        msg.resource->metadata(),
                        resolved_status,
                        resolved_type
                    );

                    it = std::next(it);
                }

                // Resource is not a known asset!
                if (asset_info_resolved == false)
                {
                    // #todo verbose message?
                    return;
                }

                IS_ASSERT(resolved_type != AssetType::Unresolved, "Asset type resolving failed");
                IS_ASSERT(resolved_status != AssetStatus::Invalid, "Asset type resolving failed");

                Asset reference{ basename, resolved_type };

                auto asset_it = core::pod::multi_hash::find_first(_asset_objects, core::hash(reference.name));
                while (asset_it != nullptr)
                {
                    // We found an asset with the same basename and type
                    if (asset_it->value.reference.type == reference.type)
                    {
                        break;
                    }
                    asset_it = core::pod::multi_hash::find_next(_asset_objects, asset_it);
                }

                if (asset_it == nullptr)
                {
                    core::pod::multi_hash::insert(
                        _asset_objects,
                        static_cast<uint64_t>(reference.name.hash_value),
                        AssetObject{ core::pod::array::size(_resource_database), std::move(reference) }
                    );
                    core::pod::array::push_back(
                        _resource_database,
                        AssetReference{
                            .content_location = msg.resource->location(),
                            .resource_object = msg.resource,
                            .status = resolved_status,
                            .compiled_asset = nullptr
                        }
                    );
                }
                else
                {
                    AssetReference& ref = _resource_database[asset_it->value.resource_index];
                    if (ref.status == AssetStatus::Available_Raw && resolved_status == AssetStatus::Available)
                    {
                        ref.content_location = msg.resource->location();
                        ref.resource_object = msg.resource;
                        ref.status = resolved_status;

                        fmt::print(
                            "Replacing asset {} resource [ {} => {} ]\n",
                            reference.name,
                            msg.resource->location(),
                            ref.resource_object->location()
                        );
                    }
                    else
                    {
                        fmt::print(
                            "Ignoring probaby duplicate asset resource: {}\n",
                            msg.resource->location()
                        );
                    }
                }
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
                    AssetObject{ static_cast<uint64_t>(removed_resource_index), std::move(reference) }
                );
                _resource_database[removed_resource_index] = AssetReference{
                    .content_location = resource->location(),
                    .resource_object = resource,
                    .status = AssetStatus::Unloading,
                    .compiled_asset = nullptr
                };
            }
            else
            {
                core::pod::multi_hash::insert(
                    _asset_objects,
                    name_hash,
                    AssetObject{ core::pod::array::size(_resource_database), std::move(reference) }
                );
                core::pod::array::push_back(
                    _resource_database,
                    AssetReference{
                        .content_location = resource->location(),
                        .resource_object = resource,
                        .status = AssetStatus::Available_Raw,
                        .compiled_asset = nullptr
                    }
                );
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

        auto& asset_resources = _resource_database[asset_object->value.resource_index];

        if (asset_resources.resource_object == nullptr)
        {
            asset_resources.resource_object = _resource_system.find(asset_resources.content_location);

            // #todo at some point we will assume this is not 'raw' but 'baked' data.
            asset_resources.status = asset::AssetStatus::Available_Raw;
        }

        // Try compiling if needed
        if (asset_resources.status == AssetStatus::Available_Raw)
        {
            auto it = _asset_compiler_map[ref.type].rbegin();
            auto const it_end = _asset_compiler_map[ref.type].rend();

            auto* compilation_result = _allocator.make<AssetCompilationResult>(_allocator);

            AssetCompilationStatus compilation_status = AssetCompilationStatus::Failed;
            while (it != it_end && compilation_status != AssetCompilationStatus::Success)
            {
                compilation_status = (*it)->compile_asset(
                    _allocator, _resource_system, ref,
                    asset_resources.resource_object->data(),
                    asset_resources.resource_object->metadata(),
                    *compilation_result
                );
            }

            if (compilation_status == AssetCompilationStatus::Success)
            {
                asset_resources.compiled_asset = compilation_result;
                asset_resources.status = AssetStatus::Available;
            }
            else
            {
                _allocator.destroy(compilation_result);
                asset_resources.status = AssetStatus::Invalid;
            }
        }

        auto it_beg = _asset_loader_map[ref.type].rbegin();
        auto const it_end = _asset_loader_map[ref.type].rend();
        AssetStatus load_status = AssetStatus::Invalid;

        // If the current asset is already unloading, release it
        // This should only happen in a single case, when an asset is reloading
        //  after an update for such an asset was requested.
        if (asset_resources.status == asset::AssetStatus::Unloading)
        {
            auto it = it_beg;

            bool unloaded = false;
            while (it != it_end && unloaded == false)
            {
                unloaded = (*it)->release_asset(asset_object->value.reference);
                it += 1;
            }

            IS_ASSERT(
                unloaded == true,
                "Pending unload failed for asset {}", ref.name
            );

            asset_resources.status = asset::AssetStatus::Available;
        }

        // Try to load the new asset
        {
            auto it = it_beg;
            while (it != it_end && load_status == AssetStatus::Invalid) // We can assume its either Invalid or Loaded
            {
                if (asset_resources.resource_object != nullptr)
                {
                    resource::ResourceMetaView resource_meta = asset_resources.resource_object->metadata();
                    core::data_view resource_data = asset_resources.resource_object->data();

                    if (asset_resources.compiled_asset != nullptr)
                    {
                        resource_data = asset_resources.compiled_asset->data;
                        resource_meta = resource::create_meta_view(asset_resources.compiled_asset->metadata);
                    }

                    load_status = (*it)->load_asset(
                        asset_object->value.reference,
                        resource_meta,
                        resource_data,
                        result_data
                    );

                    asset_resources.status = load_status;
                }
                else
                {
                    asset_resources.status = AssetStatus::Invalid;
                }

                it += 1;
            }
        }

        return load_status;
    }

    auto AssetSystem::read(Asset ref, AssetData& data) noexcept -> AssetStatus
    {
        asset::AssetStatus result_status = AssetStatus::Invalid;

        // Get the requested asset object
        auto asset_object = core::pod::multi_hash::find_first(_asset_objects, static_cast<uint64_t>(ref.name.hash_value));
        while (asset_object != nullptr && asset_object->value.reference.type != ref.type)
        {
            asset_object = core::pod::multi_hash::find_next(_asset_objects, asset_object);
        }

        if (asset_object == nullptr)
        {
            return result_status;
        }

        auto& asset_resources = _resource_database[asset_object->value.resource_index];

        if (asset_resources.resource_object == nullptr)
        {
            asset_resources.resource_object = _resource_system.find(asset_resources.content_location);

            // #todo at some point we will assume this is not 'raw' but 'baked' data.
            asset_resources.status = asset::AssetStatus::Available_Raw;
        }

        // Try compiling if needed
        if (asset_resources.status == AssetStatus::Available_Raw)
        {
            auto it = _asset_compiler_map[ref.type].rbegin();
            auto const it_end = _asset_compiler_map[ref.type].rend();

            auto* compilation_result = _allocator.make<AssetCompilationResult>(_allocator);

            AssetCompilationStatus compilation_status = AssetCompilationStatus::Failed;
            while (it != it_end && compilation_status != AssetCompilationStatus::Success)
            {
                compilation_status = (*it)->compile_asset(
                    _allocator, _resource_system, ref,
                    asset_resources.resource_object->data(),
                    asset_resources.resource_object->metadata(),
                    *compilation_result
                );
            }

            if (compilation_status == AssetCompilationStatus::Success)
            {
                asset_resources.compiled_asset = compilation_result;
                asset_resources.status = AssetStatus::Available;
                result_status = AssetStatus::Compiled;

                data.content = asset_resources.compiled_asset->data;
                data.metadata = resource::create_meta_view(asset_resources.compiled_asset->metadata);
            }
            else
            {
                _allocator.destroy(compilation_result);
                asset_resources.status = AssetStatus::Invalid;
                result_status = AssetStatus::Invalid;
            }
        }
        else if (asset_resources.status == AssetStatus::Available)
        {
            data.content = asset_resources.resource_object->data();
            data.metadata = asset_resources.resource_object->metadata();
            result_status = AssetStatus::Available;
        }

        return result_status;
    }

} // namespace asset
