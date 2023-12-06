/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/os/windows.hxx>
#include <ice/native_file.hxx>
#include <ice/param_list.hxx>
#include <ice/path_utils.hxx>
#include <ice/resource.hxx>
#include <ice/resource_hailstorm.hxx>
#include <ice/resource_hailstorm_operations.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/uri.hxx>

#include "hsc_packer_app.hxx"
#include "hsc_packer_aiostream.hxx"

auto select_chunk_loose_resource(
    ice::Metadata const& resource_meta,
    ice::Data resource_data,
    ice::Span<ice::hailstorm::v1::HailstormChunk const> chunks,
    void* userdata
) noexcept -> ice::hailstorm::v1::HailstormWriteChunkRef
{
    return ice::hailstorm::v1::HailstormWriteChunkRef{
        .data_chunk = 0,
        .meta_chunk = 1,
    };
}

auto read_resource_size(
    ice::ResourceHandle const* resource_handle,
    ice::Data& out_data,
    std::atomic_uint32_t& out_processed
) noexcept -> ice::Task<>
{
    ice::LooseResource const* const resource = ice::get_loose_resource(resource_handle);
    out_data.size = resource->size();
    out_data.alignment = ice::ualign::b_8;
    out_data.location = nullptr;
    out_processed.fetch_add(1, std::memory_order_relaxed);
    co_return;
}

int main2(int argc, char** argv)
{
    using ice::operator""_uri;
    using ice::operator""_sid;
    using ice::operator""_MiB;

    ice::HostAllocator alloc;
    ice::ParamList params{ alloc, { argv, ice::u32(argc) } };
    ice::Array<ice::String> input_dirs{ alloc };
    if (ice::params::find_all(params, Param_Input, input_dirs) == false)
    {
        return 1;
    }

    ice::String output_file;
    if (ice::params::find_first(params, Param_Output, output_file) == false)
    {
        return 3;
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
        return 2;
    }

    ice::Array<ice::String> resource_paths{ alloc };
    ice::Array<ice::Data> resource_data{ alloc };
    ice::Array<ice::Metadata> resource_metas{ alloc };
    ice::Array<ice::u32> resource_metamap{ alloc };
    ice::Array<ice::ResourceHandle*> resource_handles{ alloc };

    ice::array::resize(resource_data, ice::count(resources));
    ice::array::resize(resource_paths, ice::count(resources));
    ice::array::resize(resource_metamap, ice::count(resources));
    ice::array::resize(resource_handles, ice::count(resources));

    ice::MutableMetadata meta{ alloc };
    ice::array::push_back(resource_metas, meta);

    std::atomic_uint32_t res_count = 0;
    ice::u32 res_idx = 0;
    for (ice::Resource const* resource : resources)
    {
        if (ice::path::extension(resource->name()) == ".json")
        {
            resource_paths[res_idx] = resource->name();
            resource_metamap[res_idx] = 0;
            resource_handles[res_idx] = tracker->find_resource(resource->uri());

            ice::schedule_task_on(
                read_resource_size(resource_handles[res_idx], resource_data[res_idx], res_count),
                main_sched
            );
            res_idx += 1;
        }
    }

    // Once the task is scheduled we are now going to iterate over the queue untill
    //  the event is signalled which means all required resources where loaded.
    while (res_count.load(std::memory_order_relaxed) != res_idx)
    {
        SleepEx(1, 0);
    }

    ice::array::resize(resource_paths, res_idx);
    ice::array::resize(resource_data, res_idx);
    ice::array::resize(resource_metamap, res_idx);

    ice::hailstorm::v1::HailstormWriteData const hsdata{
        .paths = resource_paths,
        .data = resource_data,
        .metadata = resource_metas,
        .metadata_mapping = resource_metamap,
        .custom_values = { 0, 1 }
    };

    ice::native_file::HeapFilePath filepath{ alloc };
    ice::native_file::path_from_string(output_file, filepath);

    HSCPWriteParams const write_params{
        .filename = filepath,
        .task_scheduler = main_sched,
        .fn_chunk_selector = select_chunk_loose_resource,
    };
    bool const success = hscp_write_hailstorm_file(alloc, write_params, hsdata, *tracker, resource_handles);
    return success ? 0 : 1;
}
