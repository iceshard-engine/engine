/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_host.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/tool_app.hxx>
#include <ice/sort.hxx>
#include <ice/uri.hxx>

#include <hailstorm/hailstorm_operations.hxx>

#include "hsc_packer_app.hxx"
#include "hsc_packer_aiostream.hxx"

using ice::operator""_sid;
using hailstorm::v1::HailstormChunk;
using hailstorm::v1::HailstormWriteChunkRef;

auto create_chunk_loose_resource(
    hailstorm::Data resource_meta,
    hailstorm::Data resource_data,
    HailstormChunk base_chunk,
    void* userdata
) noexcept -> HailstormChunk
{
    if (resource_data.size > base_chunk.size)
    {
        base_chunk.size = resource_data.size + base_chunk.align;
    }
    return base_chunk;
}

auto select_chunk_loose_resource(
    hailstorm::Data resource_meta,
    hailstorm::Data resource_data,
    std::span<HailstormChunk const> chunks,
    void* userdata
) noexcept -> HailstormWriteChunkRef
{
    HailstormWriteChunkRef result{
        .data_chunk = ice::u16_max,
        .meta_chunk = ice::u16_max,
    };

    auto it = chunks.rbegin();
    auto const end = chunks.rend();
    while (it != end && (result.data_chunk == ice::u16_max || result.meta_chunk == ice::u16_max))
    {
        if (it->type == 1 && result.meta_chunk == ice::u16_max)
        {
            result.meta_chunk = ice::u16(std::distance(it, end) - 1);
        }
        else if (it->type == 2 && result.data_chunk == ice::u16_max)
        {
            result.data_chunk = ice::u16(std::distance(it, end) - 1);
        }

        it += 1;
    }

    ICE_ASSERT_CORE(result.data_chunk != ice::u16_max && result.meta_chunk != ice::u16_max);
    return result;
}

auto read_resource_size(
    ice::ResourceHandle const* resource_handle,
    hailstorm::Data& out_data,
    std::atomic_uint32_t& out_processed
) noexcept -> ice::Task<>
{
    ice::LooseResource const* const resource = ice::get_loose_resource(resource_handle);
    out_data.size = resource->size().value;
    out_data.align = 8;
    out_data.location = nullptr;
    out_processed.fetch_add(1, std::memory_order_relaxed);
    co_return;
}

inline auto hsdata_view(ice::Memory mem) noexcept -> hailstorm::Data
{
    return { mem.location, mem.size.value, (size_t)mem.alignment };
}

class HailStormPackerApp final : public ice::tool::ToolApp<HailStormPackerApp>
{
public:
    HailStormPackerApp() noexcept
        : _tqueue{ }
        , _tsched{ _tqueue }
        , _tpool{ }
        , _param_includes{ _allocator }
        , _param_configs{ _allocator }
        , _param_output{ _allocator }
        , _param_verbose{ false }
        , _filter_extensions_heap{ _allocator }
        , _filter_extensions{ _allocator }
    {
        _tpool = ice::create_thread_pool(_allocator, _tqueue, { .thread_count = 8 });
    }

    ~HailStormPackerApp() noexcept = default;

    auto run() noexcept -> ice::i32 override
    {
        for (ice::String config : _param_configs)
        {
            ice::HeapString<> const config_path_utf8 = hscp_process_directory(_allocator, config);
            ice::native_file::HeapFilePath config_path{ _allocator };
            ice::native_file::path_from_string(config_path_utf8, config_path);

            HSCP_ERROR_IF(
                ice::native_file::exists_file(config_path) == false,
                "Config file '{}' does not exist, skipping...",
                ice::String{ config_path_utf8 }
            );

            using enum ice::native_file::FileOpenFlags;
            ice::native_file::File file = ice::native_file::open_file(config_path, Read);
            if (file)
            {
                ice::usize const filesize = ice::native_file::sizeof_file(file);
                ice::Memory const filemem = _allocator.allocate(filesize);
                ice::native_file::read_file(file, filesize, filemem);

                ice::MutableMetadata config_meta{ _allocator };
                ice::Result const res = ice::meta_deserialize_from(config_meta, ice::data_view(filemem));

                ice::Array<ice::String> extensions{ _allocator };
                if (ice::meta_read_string_array(config_meta, "filter.extensions"_sid, extensions))
                {
                    for (ice::String ext : extensions)
                    {
                        ice::array::push_back(_filter_extensions_heap, { _allocator, ext });
                        ice::array::push_back(_filter_extensions, ice::array::back(_filter_extensions_heap));
                    }
                }
                HSCP_ERROR_IF(
                    res != ice::S_Success,
                    "Failed to parse config file '{}'...",
                    ice::String{ config_path_utf8 }
                );

                _allocator.deallocate(filemem);
            }
        }

        if (ice::array::empty(_filter_extensions))
        {
            HSCP_ERROR("No valid configuration files where provided.");
            return 1;
        }

        // for (ice::HeapString<>& include : _param_includes)
        // {
        //     include = hscp_process_directory(_allocator, include);
        // }

        // Prepare the output file name.
        _param_output = hscp_process_directory(_allocator, _param_output);

        // The paths that will be searched for loose file resources.
        ice::UniquePtr<ice::ResourceProvider> fsprov = ice::create_resource_provider(
            _allocator, _param_includes, &_tsched
        );

        // Spawning one dedicated IO thread for AIO support. Allows us to read resources MUCH faster.
        ice::ResourceTrackerCreateInfo rtinfo{
            .predicted_resource_count = 1'000'000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{},
            .flags_io_wait = ice::TaskFlags{},
        };
        ice::UniquePtr<ice::ResourceTracker> tracker = ice::create_resource_tracker(
            _allocator, _tsched, rtinfo
        );

        ice::ResourceProvider* provider = tracker->attach_provider(ice::move(fsprov));
        tracker->sync_resources();

        ice::Array<ice::Resource const*> resources{ _allocator };
        if (provider->collect(resources) == 0)
        {
            HSCP_ERROR("No files where found in the included directories.");
            return 1;
        }

        return create_package(*tracker, resources);
    }

    auto create_package(
        ice::ResourceTracker& tracker,
        ice::Span<ice::Resource const* const> resources
    ) noexcept -> ice::i32
    {
        ice::Array<hailstorm::Data> resource_data{ _allocator };
        ice::Array<hailstorm::Data> resource_metas{ _allocator };
        ice::Array<ice::u32> resource_metamap{ _allocator };
        ice::Array<ice::ResourceHandle*> resource_handles{ _allocator };
        ice::Array<std::string_view> resource_paths{ _allocator };

        ice::array::resize(resource_data, ice::count(resources));
        ice::array::resize(resource_metamap, ice::count(resources));
        ice::array::resize(resource_handles, ice::count(resources));
        ice::array::resize(resource_paths, ice::count(resources));

        ice::MutableMetadata meta{ _allocator };
        ice::Memory metamem = ice::meta_save(meta, _allocator);

        ice::array::push_back(resource_metas, hsdata_view(metamem));

        std::atomic_uint32_t res_count = 0;
        ice::u32 res_idx = 0;
        auto* const end = _filter_extensions._data + _filter_extensions._count;
        for (ice::Resource const* resource : resources)
        {
            bool const found = std::find(
                _filter_extensions._data,
                end,
                ice::path::extension(resource->name())
            ) != end;

            if (found)
            {
                resource_paths[res_idx] = resource->name();
                resource_metamap[res_idx] = 0;
                resource_handles[res_idx] = tracker.find_resource(resource->uri());

                ice::schedule_task(
                    read_resource_size(resource_handles[res_idx], resource_data[res_idx], res_count),
                    _tsched
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

        hailstorm::v1::HailstormWriteData const hsdata{
            .paths = resource_paths,
            .data = resource_data,
            .metadata = resource_metas,
            .metadata_mapping = resource_metamap,
            .custom_values = { 0, 1 }
        };

        ice::native_file::HeapFilePath output{ _allocator };
        ice::native_file::path_from_string(_param_output, output);
        HSCPWriteParams const write_params{
            .filename = output,
            .task_scheduler = _tsched,
            .fn_chunk_selector = select_chunk_loose_resource,
            .fn_chunk_create = create_chunk_loose_resource,
        };
        bool const success = hscp_write_hailstorm_file(_allocator, write_params, hsdata, tracker, resource_handles);

        _allocator.deallocate(metamem);
        return success ? 0 : 1;
    }

    bool setup(ice::Params& params) noexcept override
    {
        ice::log_tag_register(LogTag_Main);
        ice::log_tag_register(LogTag_Details);
        ice::params_define(params, Param_Include, _param_includes);
        ice::params_define(params, Param_Output, _param_output);
        ice::params_define(params, Param_Config, _param_configs);
        ice::params_define(params, Param_Verbose, _param_verbose);
        return true;
    }

public: // Tool information
    auto name() const noexcept -> ice::String override { return "hsc_packer"; }
    auto version() const noexcept -> ice::String override { return "0.1.0"; }
    auto description() const noexcept -> ice::String override
    {
        return "Create a hailstorm pack files from the given input directories and configuration file.";
    }

private:
    ice::TaskQueue _tqueue;
    ice::TaskScheduler _tsched;
    ice::UniquePtr<ice::TaskThreadPool> _tpool;

    // Params
    ice::Array<ice::String> _param_includes;
    ice::Array<ice::String> _param_configs;
    ice::HeapString<> _param_output;
    bool _param_verbose;

    // Filters
    ice::Array<ice::HeapString<>> _filter_extensions_heap;
    ice::Array<ice::String> _filter_extensions;
};
