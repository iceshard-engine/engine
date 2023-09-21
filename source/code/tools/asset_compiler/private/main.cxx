/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/os/windows.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/uri.hxx>

#include <ice/sync_manual_events.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>

static constexpr auto AssetType_Config = ice::make_asset_type("ice/core/config");

auto assettype_config_state(
    void*,
    ice::AssetTypeDefinition const&,
    ice::Metadata const&,
    ice::URI const&
) noexcept -> ice::AssetState
{
    return ice::AssetState::Baked;
}

int main(int argc, char** argv)
{
    if (argc < 2) return 1;

    ice::HostAllocator alloc;

    // Since this process is going to be designed for 'single input -> single output'
    //   scenarios, we won't create a thread-pool for the Task queue and instead
    //   we will consume all tasks in the main thread.
    ice::TaskQueue main_queue;
    ice::TaskScheduler main_sched{ main_queue };

    // Becase we don't want to spawn any additional threads we setting
    //  dedicated threads to '0'. Aiming to load resources on the main thread.
    ice::ResourceTrackerCreateInfo rtinfo{
        .predicted_resource_count = 1000,
        .io_dedicated_threads = 0,
        .flags_io_complete = ice::TaskFlags{},
        .flags_io_wait = ice::TaskFlags{},
    };
    ice::UniquePtr<ice::ResourceTracker> tracker = ice::create_resource_tracker(
        alloc, main_sched, rtinfo
    );

    // The paths that will be searched for loose file resources.
    // TODO: Create a "ExplicitFileProvider" which will provide the file resources that it was explicitly created with.
    ice::String const storages[]{ ice::String{ argv[1] } };
    ice::UniquePtr<ice::ResourceProvider> fsprov = ice::create_resource_provider(
        alloc, storages
    );
    tracker->attach_provider(fsprov.get());
    tracker->sync_resources();

    // Find the resource that we want to have baked.
    using namespace ice;

    // TOOD: Inset Asset system here. And ask for the asset
    //  instead of the resource, this will bake it and return the baked results we want.
    ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(
        alloc
    );
    ice::String const exts[]{ ".json" };
    asset_types->register_type(
        AssetType_Config,
        AssetTypeDefinition{
            .resource_extensions = exts,
            .fn_asset_state = assettype_config_state,
        }
    );

    ice::UniquePtr<ice::AssetStorage> assets = ice::create_asset_storage(
        alloc,
        ice::move(asset_types),
        ice::AssetStorageCreateInfo{
            .resource_tracker = *tracker,
            .task_scheduler = main_sched,
            .task_flags = ice::TaskFlags{},
        }
    );

    ice::Asset asset = assets->bind(AssetType_Config, "config");

    // Since we don't have any threading setup for the resource tracker, we need to
    //  manually await the load task and keep the result.
    ice::Data result;
    ice::ManualResetEvent ev{ };
    ice::manual_wait_for(asset.data(ice::AssetState::Baked), ev, result);

    // Once the task is scheduled we are now going to iterate over the queue untill
    //  the event is signalled which means all required resources where loaded.
    while (ev.is_set() == false)
    {
        for (auto* task : ice::linked_queue::consume(main_queue._awaitables))
        {
            task->_coro.resume();
        }
    }

    ICE_LOG(
        ice::LogSeverity::Info,
        ice::LogTag::Asset,
        "Asset baked with final size: {}",
        result.size
    );

    assets->release(asset);
    return 0;
}
