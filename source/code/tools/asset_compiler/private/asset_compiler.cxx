/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#if 0
#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/os/windows.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/uri.hxx>

#include <ice/sync_manual_events.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/param_list.hxx>
#include <ice/path_utils.hxx>

#include <unordered_map>

static constexpr auto AssetCategory_Config = ice::make_asset_type("ice/core/config");

auto assetcat_config_state(
    void*,
    ice::AssetTypeDefinition const&,
    ice::Metadata const&,
    ice::URI const&
) noexcept -> ice::AssetState
{
    return ice::AssetState::Baked;
}

static constexpr ice::ParamDefinition<ice::String> Param_Input{
    .name = "input",
    .name_short = "i",
    .description = "Defines the input for the asset compiler",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Output{
    .name = "out",
    .name_short = "o",
    .description = "Defines the output for the asset compiler",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Config{
    .name = "config",
    .name_short = "c",
    .description = "Defines the config for the asset compiler",
    .flags = ice::ParamFlags::None,
};

int main(int argc, char** argv)
{
    using ice::operator""_uri;
    using ice::operator""_sid;

    ice::HostAllocator alloc;
    ice::Params params{ alloc, { argv, ice::u32(argc) } };
    ice::Array<ice::String> input_dirs{ alloc };
    if (ice::params::find_all(params, Param_Input, input_dirs) == false)
    {
        return 1;
    }

    // Since this process is going to be designed for 'single input -> single output'
    //   scenarios, we won't create a thread-pool for the Task queue and instead
    //   we will consume all tasks in the main thread.
    ice::TaskQueue main_queue;
    ice::TaskScheduler main_sched{ main_queue };

    ice::TaskThreadPoolCreateInfo tpci{
        .thread_count = 8,
    };
    ice::UniquePtr<ice::TaskThreadPool> tp = ice::create_thread_pool(alloc, main_queue, tpci);

    // The paths that will be searched for loose file resources.
    // TODO: Create a "ExplicitFileProvider" which will provide the file resources that it was explicitly created with.
    ice::UniquePtr<ice::ResourceProvider> fsprov = ice::create_resource_provider(
        alloc, input_dirs
    );

    // Becase we don't want to spawn any additional threads we setting
    //  dedicated threads to '0'. Aiming to load resources on the main thread.
    ice::ResourceTrackerCreateInfo rtinfo{
        .predicted_resource_count = 1'000'000,
        .io_dedicated_threads = 1,
        .flags_io_complete = ice::TaskFlags{},
        .flags_io_wait = ice::TaskFlags{},
    };
    ice::UniquePtr<ice::ResourceTracker> tracker = ice::create_resource_tracker(
        alloc, main_sched, rtinfo
    );

    ice::ResourceProvider* provider = tracker->attach_provider(ice::move(fsprov));
    tracker->sync_resources();

    ice::Array<ice::Resource const*> resources{ alloc };
    if (provider->collect(resources) == 0)
    {
        return 0;
    }

    // Find the resource that we want to have baked.
    using namespace ice;

    // TOOD: Inset Asset system here. And ask for the asset
    //  instead of the resource, this will bake it and return the baked results we want.
    ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(
        alloc
    );
    ice::String const exts[]{ ".json" };
    asset_types->register_type(
        AssetCategory_Config,
        AssetTypeDefinition{
            .resource_extensions = exts,
            .fn_asset_state = assetcat_config_state,
        }
    );
    return 0;
}
#endif
