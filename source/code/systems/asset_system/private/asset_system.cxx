#include <ice/asset_system.hxx>
#include <ice/asset_pipeline.hxx>
#include <ice/asset_oven.hxx>
#include <ice/asset_loader.hxx>
#include <ice/resource.hxx>
#include <ice/resource_status.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/pod/array.hxx>
#include <ice/pod/hash.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/map.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/assert.hxx>

#include <ice/stack_string.hxx>
#include <ice/asset_type.hxx>

#include "asset_internal.hxx"

namespace ice
{

    namespace detail
    {

        struct AssetEntry
        {
            ice::u32 data_index;
            ice::AssetType type;
            ice::StringID name;
        };

        struct AssetInfo
        {
            ice::URI location;
            ice::ResourceHandle* handle;
            ice::AssetPipeline* pipeline;
            ice::Memory baked_data;
            ice::Memory loaded_data;
            AssetObject* object;
        };

        class ExplicitAssetSolver : public ice::AssetSolver
        {
        public:
            ExplicitAssetSolver(
                ice::Allocator& alloc,
                ice::AssetSystem& assets,
                ice::ResourceTracker& resources,
                ice::Metadata const& metadata
            ) noexcept
                : _allocator{ alloc }
                , _assets{ assets }
                , _resources{ resources }
                , _metadata{ metadata }
            {
            }

            auto find_asset(
                ice::AssetType type,
                ice::StringID_Hash asset_name_hash
            ) noexcept -> ice::Asset
            {
                ice::ResourceHandle* explicit_resource = nullptr;
                ice::pod::Array<ice::String> explicit_dependencies{ _allocator };
                if (ice::meta_read_string_array(_metadata, "asset.explicit_dependencies"_sid, explicit_dependencies))
                {
                    ice::u32 idx = 0;
                    for (ice::String dep : explicit_dependencies)
                    {
                        if (ice::stringid_hash(ice::stringid(dep)) == asset_name_hash)
                        {
                            break;
                        }
                        idx += 1;
                    }

                    ice::pod::Array<ice::Utf8String> explicit_resources{ _allocator };
                    if (ice::meta_read_utf8_array(_metadata, "asset.explicit_resources"_sid, explicit_resources))
                    {
                        if (ice::size(explicit_resources) > idx)
                        {
                            explicit_resource = _resources.find_resource(ice::URI{ explicit_resources[idx] });
                        }
                    }
                }

                if (explicit_resource == nullptr)
                {
                    return _assets.request(type, ice::StringID{ asset_name_hash });
                }
                else
                {
                    return _assets.load(type, explicit_resource);
                }
            }

        private:
            ice::Allocator& _allocator;
            ice::AssetSystem& _assets;
            ice::ResourceTracker& _resources;
            ice::Metadata const& _metadata;
        };

        class GenericAssetSolver : public ice::AssetSolver
        {
        public:
            GenericAssetSolver(
                ice::AssetSystem& assets
            ) noexcept
                : _assets{ assets }
            { }

            auto find_asset(
                ice::AssetType type,
                ice::StringID_Hash asset_name_hash
            ) noexcept -> ice::Asset
            {
                return _assets.request(type, ice::StringID{ asset_name_hash });
            }

        private:
            ice::AssetSystem& _assets;
        };

    } // namespace detail

    class SimpleAssetSystem final : public AssetSystem
    {
    public:
        SimpleAssetSystem(
            ice::Allocator& alloc,
            ice::ResourceTracker& resource_system
        ) noexcept;

        ~SimpleAssetSystem() noexcept override;

        bool add_pipeline(
            ice::StringID_Arg name,
            ice::UniquePtr<AssetPipeline> pipeline
        ) noexcept override;

        bool remove_pipeline(
            ice::StringID_Arg name
        ) noexcept override;

        void bind_resources(
            ice::Span<ice::ResourceHandle*> resources
        ) noexcept override;

        bool bind_resource(
            ice::AssetType type,
            ice::StringID_Arg name,
            ice::ResourceHandle* resource
        ) noexcept override;

        auto request(
            ice::AssetType type,
            ice::StringID_Arg name
        ) noexcept -> ice::Asset override;

        auto load(
            ice::AssetType type,
            ice::ResourceHandle* resource
        ) noexcept -> Asset override;

        void release(
            ice::Asset asset
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::ResourceTracker& _resource_system;

        ice::memory::ProxyAllocator _oven_alloc;
        ice::Map<ice::StringID_Hash, ice::UniquePtr<ice::AssetPipeline>> _pipelines;
        ice::pod::Hash<ice::AssetPipeline*> _asset_pipelines;

        ice::pod::Hash<detail::AssetEntry> _explicit_entiries;
        ice::pod::Hash<detail::AssetEntry> _asset_entries;
        ice::pod::Array<detail::AssetInfo> _assets;
    };

    SimpleAssetSystem::SimpleAssetSystem(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker
    ) noexcept
        : _allocator{ alloc }
        , _resource_system{ resource_tracker }
        , _oven_alloc{ _allocator, "asset-oven-allocator" }
        , _pipelines{ _allocator }
        , _asset_pipelines{ _allocator }
        , _explicit_entiries{ _allocator }
        , _asset_entries{ _allocator }
        , _assets{ _allocator }
    {
        ice::pod::array::reserve(_assets, 1000);
    }

    SimpleAssetSystem::~SimpleAssetSystem() noexcept
    {
        for (detail::AssetInfo& info : _assets)
        {
            _oven_alloc.deallocate(info.loaded_data.location);
            _oven_alloc.deallocate(info.baked_data.location);
            _allocator.destroy(info.object);
        }
    }

    bool SimpleAssetSystem::add_pipeline(
        ice::StringID_Arg name,
        ice::UniquePtr<AssetPipeline> pipeline
    ) noexcept
    {
        ICE_ASSERT(
            ice::map::has(_pipelines, ice::stringid_hash(name)) == false,
            "A pipeline with this name {} is already present!", 0
        );

        if (ice::map::has(_pipelines, ice::stringid_hash(name)) == false)
        {
            for (ice::AssetType const type : pipeline->supported_types())
            {
                ice::pod::multi_hash::insert(_asset_pipelines, ice::hash(type), pipeline.get());
            }

            ice::map::set(_pipelines, ice::stringid_hash(name), ice::move(pipeline));
            return true;
        }
        return false;
    }

    bool SimpleAssetSystem::remove_pipeline(ice::StringID_Arg name) noexcept
    {
        // #todo assert/warning has(_pipelines, name) == true

        if (ice::map::has(_pipelines, ice::stringid_hash(name)))
        {
            ice::AssetPipeline* const pipeline = ice::map::get(
                _pipelines,
                ice::stringid_hash(name),
                ice::make_unique_null<ice::AssetPipeline>()
            ).get();

            // #todo assert pipeline_ptr != nullptr
            for (ice::AssetType const type : pipeline->supported_types())
            {
                auto* entry = ice::pod::multi_hash::find_first(_asset_pipelines, ice::hash(type));
                while (entry != nullptr && entry->value != pipeline)
                {
                    entry = ice::pod::multi_hash::find_next(_asset_pipelines, entry);
                }

                if (entry != nullptr)
                {
                    ice::pod::multi_hash::remove(_asset_pipelines, entry);
                }
            }
            ice::map::remove(_pipelines, ice::stringid_hash(name));
            return true;
        }
        return false;
    }

    void SimpleAssetSystem::bind_resources(ice::Span<ice::ResourceHandle*> resources) noexcept
    {
        for (ice::ResourceHandle* handle : resources)
        {
            ice::Resource_v2 const* resource = ice::resource_object_DEPRECATED(handle);

            // TODO: Remove during asset system refactor.
            ice::String const res_name_DEPRECATED{ reinterpret_cast<char const*>(resource->name().data()), resource->name().size() };

            ice::u32 const extension_pos = ice::string::find_first_of(res_name_DEPRECATED, '.');
            ice::String const basename = ice::string::substr(res_name_DEPRECATED, 0, extension_pos);
            ice::String const extension = ice::string::substr(res_name_DEPRECATED, extension_pos);
            ice::Metadata const& meta = resource->metadata();

            ice::AssetType result_type = AssetType::Unresolved;
            ice::AssetStatus result_status = AssetStatus::Invalid;

            auto* pipeline_it = ice::pod::hash::begin(_asset_pipelines);
            auto* const piepeline_end = ice::pod::hash::end(_asset_pipelines);

            ice::AssetPipeline* pipeline = nullptr;
            while (pipeline_it != piepeline_end && result_type == AssetType::Unresolved)
            {
                pipeline = pipeline_it->value;
                pipeline->resolve(
                    extension,
                    meta,
                    result_type,
                    result_status
                );

                pipeline_it += 1;
            }

            if (result_type == AssetType::Unresolved)
            {
                // #todo info result_type == AssetType::Unresolved
                continue;
            }

            if (result_status == AssetStatus::Available_Raw)
            {
                // #todo warning/error/assert pipeline->supports_baking(result_type)
                if (pipeline->supports_baking(result_type) == false)
                {
                    continue;
                }
            }

            ice::StringID const asset_name = ice::stringid(basename);
            ice::u64 const asset_name_hash = ice::hash(asset_name);

            auto entry = ice::pod::multi_hash::find_first(_asset_entries, asset_name_hash);
            while (entry != nullptr)
            {
                // We found an asset with the same basename and type
                if (entry->value.type == result_type)
                {
                    break;
                }
                entry = ice::pod::multi_hash::find_next(_asset_entries, entry);
            }

            if (entry == nullptr)
            {
                ice::pod::multi_hash::insert(
                    _asset_entries,
                    asset_name_hash,
                    detail::AssetEntry{
                        .data_index = ice::pod::array::size(_assets),
                        .type = result_type,
                        .name = asset_name,
                    }
                );

                ice::pod::array::push_back(
                    _assets,
                    detail::AssetInfo{
                        .location = resource->uri(),
                        .handle = handle,
                        .pipeline = pipeline,
                        .baked_data = ice::Memory{ },
                        .loaded_data = ice::Memory{ },
                        .object = detail::make_empty_object(_allocator, asset_name, result_status, resource->metadata()),
                    }
                );
            }
            else
            {
                detail::AssetInfo& current_info = _assets[entry->value.data_index];
                if (current_info.object->status == AssetStatus::Available_Raw && result_status == AssetStatus::Available)
                {
                    current_info.location = resource->uri();
                    current_info.handle = handle;
                    current_info.pipeline = pipeline;

                    _allocator.destroy(current_info.object);
                    current_info.object = detail::make_empty_object(_allocator, asset_name, result_status, resource->metadata());

                    // #todo log asset replacement
                }
                else
                {
                    // #todo log asset ignored
                }
            }

            // #todo assert result_type != AssetType::Invalid
        }
    }

    bool SimpleAssetSystem::bind_resource(
        ice::AssetType type,
        ice::StringID_Arg name,
        ice::ResourceHandle* handle
    ) noexcept
    {
        ice::Resource_v2 const* resource = ice::resource_object_DEPRECATED(handle);

        // TODO: Remove during asset system refactor.
        ice::String const res_name_DEPRECATED{ reinterpret_cast<char const*>(resource->name().data()), resource->name().size() };

        ice::u32 const extension_pos = ice::string::find_first_of(res_name_DEPRECATED, '.');
        ice::String const extension = ice::string::substr(res_name_DEPRECATED, extension_pos);
        ice::Metadata const& meta = resource->metadata();

        ice::AssetType result_type = AssetType::Unresolved;
        ice::AssetStatus result_status = AssetStatus::Invalid;

        auto* pipeline_it = ice::pod::hash::begin(_asset_pipelines);
        auto* const piepeline_end = ice::pod::hash::end(_asset_pipelines);

        ice::AssetPipeline* pipeline = nullptr;
        while (pipeline_it != piepeline_end && result_type == AssetType::Unresolved)
        {
            pipeline = pipeline_it->value;
            pipeline->resolve(
                extension,
                meta,
                result_type,
                result_status
            );

            pipeline_it += 1;
        }

        if (result_type == AssetType::Unresolved)
        {
            // #todo info result_type == AssetType::Unresolved
            return false;
        }

        if (result_status == AssetStatus::Available_Raw)
        {
            // #todo warning/error/assert pipeline->supports_baking(result_type)
            if (pipeline->supports_baking(result_type) == false)
            {
                return false;
            }
        }

        ice::String base_name = ice::string::substr(res_name_DEPRECATED, 0, extension_pos);

        ice::u64 const name_hash = ice::hash(base_name);

        auto* entry = ice::pod::multi_hash::find_first(_asset_entries, name_hash);
        while (entry != nullptr)
        {
            if (entry->value.type == type)
            {
                break;
            }
            entry = ice::pod::multi_hash::find_next(_asset_entries, entry);
        }

        if (entry == nullptr)
        {
            ice::pod::multi_hash::insert(
                _asset_entries,
                name_hash,
                detail::AssetEntry{
                    .data_index = ice::pod::array::size(_assets),
                    .type = type,
                    .name = name,
                }
            );

            ice::pod::array::push_back(
                _assets,
                detail::AssetInfo{
                    .location = resource->uri(),
                    .handle = handle,
                    .pipeline = pipeline,
                    .baked_data = ice::Memory{ },
                    .loaded_data = ice::Memory{ },
                    .object = detail::make_empty_object(_allocator, name, result_status, resource->metadata()),
                }
            );
        }
        else
        {
            detail::AssetInfo& current_info = _assets[entry->value.data_index];
            if (current_info.object->status == AssetStatus::Loaded && result_status == AssetStatus::Available)
            {
                current_info.location = resource->uri();
                current_info.handle = handle;
                current_info.pipeline = pipeline;

                //_oven_alloc.deallocate(current_info.object->data.location);
                _allocator.destroy(current_info.object);
                current_info.object = detail::make_empty_object(_allocator, name, AssetStatus::Unloading, { });

                // #todo log asset replacement
            }
            else
            {
                // #todo log asset ignored
            }
        }
        return true;
    }

    auto SimpleAssetSystem::request(ice::AssetType type, ice::StringID_Arg name) noexcept -> ice::Asset
    {
        // Get the requested asset object
        auto entry = ice::pod::multi_hash::find_first(_asset_entries, ice::hash(name));
        while (entry != nullptr && entry->value.type != type)
        {
            entry = ice::pod::multi_hash::find_next(_asset_entries, entry);
        }

        if (entry == nullptr)
        {
            // Asset does not exist
            return Asset::Invalid;
        }

        detail::AssetInfo& asset_info = _assets[entry->value.data_index];
        detail::AssetObject& asset_object = *asset_info.object;

        if (asset_object.status == AssetStatus::Loaded)
        {
            return detail::make_asset(asset_info.object);
        }

        // If the current asset is already unloading, release it
        // This should only happen in a single case, when an asset is reloading
        //  after an update for such an asset was requested.
        if (asset_object.status == AssetStatus::Unloading)
        {
            // #todo log unloading status

            _oven_alloc.deallocate(asset_info.loaded_data.location);
            _oven_alloc.deallocate(asset_info.baked_data.location);
            asset_info.loaded_data = ice::Memory{ };
            asset_info.baked_data = ice::Memory{ };
            asset_object.status = AssetStatus::Available;
        }

        ice::Data asset_data;

        // Try compiling if needed
        if (asset_object.status == AssetStatus::Available_Raw)
        {
            ice::Resource_v2 const* resource = ice::resource_object_DEPRECATED(asset_info.handle);

            // TODO: Remove during asset system refactor.
            ice::String const res_name_DEPRECATED{ reinterpret_cast<char const*>(resource->name().data()), resource->name().size() };

            ice::u32 const extension_pos = ice::string::find_first_of(res_name_DEPRECATED, '.');
            ice::String const extension = ice::string::substr(res_name_DEPRECATED, extension_pos);

            ice::AssetOven const* oven = asset_info.pipeline->request_oven(
                type,
                extension,
                resource->metadata()
            );

            ice::BakeResult bake_result = oven->bake(
                *asset_info.handle,
                _resource_system,
                *this,
                _oven_alloc,
                asset_info.baked_data
            );

            if (bake_result != BakeResult::Success)
            {
                // #todo log baking failure
                return Asset::Invalid;
            }

            asset_data = asset_info.baked_data;
            asset_object.status = AssetStatus::Available;
        }
        else
        {
            ice::ResourceResult const load_result = ice::sync_wait(_resource_system.load_resource(asset_info.handle));
            if (load_result.resource_status == ice::ResourceStatus::Loaded)
            {
                asset_data = load_result.data;
            }
        }

        ice::AssetLoader const* const loader = asset_info.pipeline->request_loader(type);
        ice::AssetStatus load_status = AssetStatus::Invalid;

        // Try to load the new asset
        {
            ice::Metadata const& meta = ice::resource_object_DEPRECATED(asset_info.handle)->metadata();
            if (ice::meta_has_entry(meta, "asset.explicit_dependencies"_sid))
            {
                detail::ExplicitAssetSolver explicit_solver{ _allocator, *this, _resource_system, meta };

                load_status = loader->load(
                    explicit_solver,
                    type,
                    asset_data,
                    _oven_alloc,
                    asset_info.loaded_data
                );
            }
            else
            {
                detail::GenericAssetSolver generic_solver{ *this };

                load_status = loader->load(
                    generic_solver,
                    type,
                    asset_data,
                    _oven_alloc,
                    asset_info.loaded_data
                );
            }

            if (load_status == AssetStatus::Loaded)
            {
                asset_object.status = load_status;
                asset_object.data = asset_info.loaded_data;
                return detail::make_asset(asset_info.object);
            }
            else
            {
                // #todo log invalid load
                asset_object.status = AssetStatus::Invalid;
            }
        }

        return Asset::Invalid;
    }

    auto SimpleAssetSystem::load(ice::AssetType type, ice::ResourceHandle* handle) noexcept -> Asset
    {
        ice::Resource_v2 const* resource = ice::resource_object_DEPRECATED(handle);

        // TODO: Remove during asset system refactor.
        ice::String const res_name_DEPRECATED{ reinterpret_cast<char const*>(resource->name().data()), resource->name().size() };

        ice::u32 const extension_pos = ice::string::find_first_of(res_name_DEPRECATED, '.');
        ice::String const extension = ice::string::substr(res_name_DEPRECATED, extension_pos);
        ice::Metadata const& meta = resource->metadata();

        ice::AssetType result_type = AssetType::Unresolved;
        ice::AssetStatus result_status = AssetStatus::Invalid;

        auto* pipeline_it = ice::pod::hash::begin(_asset_pipelines);
        auto* const piepeline_end = ice::pod::hash::end(_asset_pipelines);

        ice::AssetPipeline* pipeline = nullptr;
        while (pipeline_it != piepeline_end && result_type == AssetType::Unresolved)
        {
            pipeline = pipeline_it->value;
            pipeline->resolve(
                extension,
                meta,
                result_type,
                result_status
            );

            pipeline_it += 1;
        }

        if (result_type == AssetType::Unresolved)
        {
            // #todo info result_type == AssetType::Unresolved
            return Asset::Invalid;
        }

        if (result_status == AssetStatus::Available_Raw)
        {
            // #todo warning/error/assert pipeline->supports_baking(result_type)
            if (pipeline->supports_baking(result_type) == false)
            {
                return Asset::Invalid;
            }
        }

        ice::String base_name = ice::string::substr(res_name_DEPRECATED, 0, extension_pos);

        ice::u64 const name_hash = ice::hash(base_name);

        auto* entry = ice::pod::multi_hash::find_first(_explicit_entiries, name_hash);
        while (entry != nullptr)
        {
            if (entry->value.type == type)
            {
                break;
            }
            entry = ice::pod::multi_hash::find_next(_explicit_entiries, entry);
        }

        if (entry == nullptr)
        {
            ice::u32 const asset_data_index = ice::pod::array::size(_assets);
            ice::StringID const asset_name = stringid(base_name);

            ice::pod::multi_hash::insert(
                _explicit_entiries,
                name_hash,
                detail::AssetEntry{
                    .data_index = asset_data_index,
                    .type = type,
                    .name = asset_name,
                }
            );

            ice::pod::array::push_back(
                _assets,
                detail::AssetInfo{
                    .location = resource->uri(),
                    .handle = handle,
                    .pipeline = pipeline,
                    .baked_data = ice::Memory{ },
                    .loaded_data = ice::Memory{ },
                    .object = detail::make_empty_object(_allocator, asset_name, result_status, resource->metadata()),
                }
            );

            detail::AssetInfo& asset_info = _assets[asset_data_index];
            detail::AssetObject& asset_object = *asset_info.object;

            ice::Data asset_data;

            // Try compiling if needed
            if (asset_object.status == AssetStatus::Available_Raw)
            {
                ice::AssetOven const* oven = asset_info.pipeline->request_oven(
                    type,
                    extension,
                    resource->metadata()
                );

                ice::BakeResult bake_result = oven->bake(
                    *asset_info.handle,
                    _resource_system,
                    *this,
                    _oven_alloc,
                    asset_info.baked_data
                );

                if (bake_result != BakeResult::Success)
                {
                    // #todo log baking failure
                    return Asset::Invalid;
                }

                asset_data = asset_info.baked_data;
                asset_object.status = AssetStatus::Available;
            }
            else
            {
                ice::ResourceResult const load_result = ice::sync_wait(_resource_system.load_resource(asset_info.handle));
                if (load_result.resource_status == ice::ResourceStatus::Loaded)
                {
                    asset_data = load_result.data;
                }
            }

            ice::AssetLoader const* const loader = asset_info.pipeline->request_loader(type);
            ice::AssetStatus load_status = AssetStatus::Invalid;

            // Try to load the new asset
            {
                ice::Metadata const& meta_2 = resource->metadata();
                if (ice::meta_has_entry(meta_2, "asset.explicit_dependencies"_sid))
                {
                    detail::ExplicitAssetSolver explicit_solver{ _allocator, *this, _resource_system, meta_2 };

                    load_status = loader->load(
                        explicit_solver,
                        type,
                        asset_data,
                        _oven_alloc,
                        asset_info.loaded_data
                    );
                }
                else
                {
                    detail::GenericAssetSolver generic_solver{ *this };

                    load_status = loader->load(
                        generic_solver,
                        type,
                        asset_data,
                        _oven_alloc,
                        asset_info.loaded_data
                    );
                }

                if (load_status == AssetStatus::Loaded)
                {
                    asset_object.status = load_status;
                    asset_object.data = asset_info.loaded_data;
                    return detail::make_asset(asset_info.object);
                }
                else
                {
                    // #todo log invalid load
                    asset_object.status = AssetStatus::Invalid;
                }
            }

            return Asset::Invalid;

        }
        else
        {
            return detail::make_asset(_assets[entry->value.data_index].object);
        }

        return Asset::Invalid;
    }

    void SimpleAssetSystem::release(ice::Asset asset) noexcept
    {
    }

    auto create_asset_system(ice::Allocator& alloc, ice::ResourceTracker& resource_system) noexcept -> ice::UniquePtr<ice::AssetSystem>
    {
        return ice::make_unique<ice::AssetSystem, ice::SimpleAssetSystem>(alloc, alloc, resource_system);
    }

} // namespace ice
