/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/os/windows.hxx>
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
#include <unordered_map>

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

struct HailstormWriteUD
{
    ice::Span<ice::Data> datas;
    std::atomic_uint32_t& count;
};

auto write_loose_resource(
    ice::u32 res_idx,
    ice::hailstorm::v1::HailstormWriteData const& write_data,
    ice::Memory memory,
    void* userdata
) noexcept -> bool
{
    HailstormWriteUD const& ud = *reinterpret_cast<HailstormWriteUD*>(userdata);
    ice::memcpy(memory, write_data.data[res_idx]);
    ud.count.fetch_sub(1, std::memory_order_relaxed);
    return true;
}

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

auto stream_open(ice::usize total_size, void* userdata) noexcept -> bool
{
    HANDLE* h = (HANDLE*)userdata;
    *h = CreateFile(L"test.hscs", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return true;
}

auto stream_close(void* userdata) noexcept -> bool
{
    CloseHandle(*(HANDLE*)userdata);
    return true;
}

auto stream_write_header(
    ice::Data header_data,
    ice::usize write_offset,
    void* userdata
) noexcept -> bool
{
    LARGE_INTEGER li;
    li.QuadPart = write_offset.value;
    OVERLAPPED ov{ };
    ov.Offset = li.LowPart;
    ov.OffsetHigh = li.HighPart;
    DWORD written = 0;
    WriteFile(*(HANDLE*)userdata, header_data.location, (DWORD) header_data.size.value, &written, &ov);
    return true;
}

auto stream_write_metadata(
    ice::hailstorm::v1::HailstormWriteData const& write_data,
    ice::u32 meta_idx,
    ice::usize write_offset,
    void* userdata
) noexcept -> bool
{
    static ice::Memory temp_mem = []() noexcept -> ice::Memory
    {
        static char buffer[4096];
        return { buffer, 4096, ice::ualign::b_8 };
    }();

    ice::meta_store(write_data.metadata[meta_idx], temp_mem);
    LARGE_INTEGER li;
    li.QuadPart = write_offset.value;
    OVERLAPPED ov{ };
    ov.Offset = li.LowPart;
    ov.OffsetHigh = li.HighPart;
    DWORD written = 0;
    WriteFile(*(HANDLE*)userdata, temp_mem.location, (DWORD)temp_mem.size.value, &written, &ov);
    return true;
}

auto stream_write_resource(
    ice::hailstorm::v1::HailstormWriteData const& write_data,
    ice::u32 res_idx,
    ice::usize write_offset,
    void* userdata
) noexcept -> bool
{
    LARGE_INTEGER li;
    li.QuadPart = write_offset.value;
    OVERLAPPED ov{ };
    ov.Offset = li.LowPart;
    ov.OffsetHigh = li.HighPart;
    DWORD written = 0;
    WriteFile(*(HANDLE*)userdata, write_data.data[res_idx].location, (DWORD)write_data.data[res_idx].size.value, &written, &ov);
    return true;
}


int main(int argc, char** argv)
{
    using ice::operator""_uri;

    ice::HostAllocator alloc;
    ice::ParamList params{ alloc, { argv, ice::u32(argc) } };
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
    using ice::operator""_sid;

    // The paths that will be searched for loose file resources.
    // TODO: Create a "ExplicitFileProvider" which will provide the file resources that it was explicitly created with.
    ice::UniquePtr<ice::ResourceProvider> fsprov = ice::create_resource_provider(
        alloc, input_dirs
    );
    ice::String hsc[]{ "test.hscs" };
    ice::UniquePtr<ice::ResourceProvider> hsprov = ice::create_resource_provider_hailstorm(
        alloc, hsc
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
    //tracker->attach_provider(ice::move(hsprov));
    tracker->sync_resources();

    ice::Array<ice::Resource const*> resources{ alloc };
    hsprov->refresh(resources);
    if (provider->collect(resources) == 0)
    {
        return 2;
    }

    // Find the resource that we want to have baked.
    using namespace ice;

    ice::Array<ice::String> resource_paths{ alloc };
    ice::Array<ice::Data> resource_data{ alloc };
    ice::Array<ice::Metadata> resource_metas{ alloc };
    ice::Array<ice::u32> resource_metamap{ alloc };

    ice::array::resize(resource_data, ice::count(resources));
    ice::array::resize(resource_paths, ice::count(resources));
    ice::array::resize(resource_metamap, ice::count(resources));

    ice::MutableMetadata meta{ alloc };
    ice::array::push_back(resource_metas, meta);

    ice::hailstorm::v1::HailstormChunk const chunks[]{
        hailstorm::v1::HailstormChunk{
            .size = 128_MiB,
            .align = ice::ualign::b_8,
            .type = 2,
            .persistance = 3,
            .is_encrypted = false,
            .is_compressed = false,
        },
        hailstorm::v1::HailstormChunk{
            .size = 1_MiB,
            .align = ice::ualign::b_8,
            .type = 1,
            .persistance = 3,
            .is_encrypted = false,
            .is_compressed = false,
        }
    };

    std::atomic_uint32_t res_count = 0;
    ice::u32 res_idx = 0;
    for (ice::Resource const* resource : resources)
    {
        if (ice::path::extension(resource->name()) == ".json")
        {
            res_count.fetch_add(1, std::memory_order_relaxed);
            resource_paths[res_idx] = resource->name();
            resource_metamap[res_idx] = 0;

            ice::schedule_task_on(
                [](
                    ice::Resource const* res,
                    ice::ResourceTracker& track,
                    ice::Span<ice::Data> out_data,
                    std::atomic_uint32_t& count,
                    ice::u32 idx
                ) noexcept -> ice::Task<>
                {
                    ice::HostAllocator alloc;
                    ice::ResourceHandle* resh;
                    using namespace ice;
                    {
                        IPT_ZONE_SCOPED;
                        resh = track.find_resource(res->uri());
                    }
                    ice::ResourceResult data = co_await track.load_resource(resh);
                    //ice::Metadata metadata;
                    //co_await ice::resource_meta(resh, metadata);
                    if (data.resource_status == ice::ResourceStatus::Loaded)
                    {
                        IPT_ZONE_SCOPED;
                        IPT_ZONE_NAME_STR(res->name());
                        ice::Metadata empty_meta{};
                        out_data[idx] = data.data;
                        out_data[idx].alignment = ice::ualign::b_8;
                        count.fetch_sub(1, std::memory_order_relaxed);
                    }
                    co_return;
                }(resource, *tracker, resource_data, res_count, res_idx),
                main_sched
            );
            res_idx += 1;
        }
    }

    //TOOD
    //ICE_ASSERT_CORE(false);
    //res_count.fetch_sub(1, std::memory_order_relaxed);

    // Once the task is scheduled we are now going to iterate over the queue untill
    //  the event is signalled which means all required resources where loaded.
    while (res_count.load(std::memory_order_relaxed) > 0)
    {
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

    HailstormWriteUD ud{
        .datas = resource_data,
        .count = res_count,
    };

    HANDLE handle;
    ice::hailstorm::v1::HailstormAsyncWriteParams hsparams{
        .base_params = ice::hailstorm::v1::HailstormWriteParams{
            .temp_alloc = alloc,
            .cluster_alloc = alloc,
            .initial_chunks = chunks,
            .estimated_chunk_count = 5,
            .fn_select_chunk = select_chunk_loose_resource,
            //.fn_resource_write = write_loose_resource,
            //.userdata = &ud
        },
        .fn_async_open = stream_open,
        .fn_async_write_header = stream_write_header,
        .fn_async_write_metadata = stream_write_metadata,
        .fn_async_write_resource = stream_write_resource,
        .fn_async_close = stream_close,
        .async_userdata = &handle
    };

    [[maybe_unused]]
    bool write_success = ice::hailstorm::v1::write_cluster_async(
        hsparams,
        hsdata
    );
    write_success = false;

    for (ice::Resource const* resource : resources)
    {
        if (ice::path::extension(resource->name()) == ".json")
        {
            res_count.fetch_add(1, std::memory_order_relaxed);
            ice::schedule_task_on(
                [](
                    ice::Resource const* res,
                    ice::ResourceTracker& track,
                    std::atomic_uint32_t& count
                ) noexcept -> ice::Task<>
                {
                    ice::HostAllocator alloc;
                    ice::ResourceHandle* resh;
                    using namespace ice;
                    {
                        IPT_ZONE_SCOPED;
                        resh = track.find_resource(res->uri());
                    }
                    co_await track.unload_resource(resh);
                    count.fetch_sub(1, std::memory_order_relaxed);
                    co_return;
                }(resource, *tracker, res_count),
                main_sched
            );
        }
    }

    while (res_count.load(std::memory_order_relaxed) > 0)
    {
    }

    return 0;
}
