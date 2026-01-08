/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_host.hxx>
#include <ice/resource.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_thread_utils.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/tool_app.hxx>
#include <ice/config.hxx>
#include <ice/config/config_builder.hxx>
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
    uint32_t /*partial_chunk_start*/,
    uint32_t /*partial_chunk_count*/,
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
    ice::ResourceHandle const& resource_handle,
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

inline auto hsdata_view(ice::Data mem) noexcept -> hailstorm::Data
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
        , _aioport{ ice::native_aio::aio_open(_allocator, {.worker_limit=4}) }
        , _param_includes{ _allocator }
        , _param_configs{ _allocator }
        , _param_output{ _allocator }
        , _param_verbose{ false }
        , _inputs{ _allocator }
        , _filter_extensions_heap{ _allocator }
        , _filter_extensions{ _allocator }
    {
        _tpool = ice::create_thread_pool(_allocator, _tqueue, { .thread_count = 12, .aioport = _aioport });
    }

    ~HailStormPackerApp() noexcept
    {
        _tpool.reset(); // Need to close the threadpool before the aio
        ice::native_aio::aio_close(_aioport);
    }

    auto run() noexcept -> ice::i32 override
    {
        return run_explicit();
    }

    auto run_from_directories() noexcept -> ice::i32
    {
        for (ice::String configpath : _param_configs)
        {
            ice::HeapString<> const config_path_utf8 = hscp_process_directory(_allocator, configpath);
            ice::native_file::HeapFilePath config_path{ _allocator };
            ice::native_file::path_from_string(config_path, config_path_utf8);

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

                // When loading from json, we always get the finalized version of the config.
                // That memory needs to have the same lifetiem as the Config object.
                ice::Memory configmem;
                ice::Config const config = ice::config::from_json(
                    _allocator,
                    ice::String{ (char const*)filemem.location, (ice::u32)filemem.size.value },
                    configmem
                );

                ice::Array<ice::String> extensions{ _allocator };
                if (ice::config::get_array(config, "filter.extensions", extensions))
                {
                    for (ice::String ext : extensions)
                    {
                        ice::array::push_back(_filter_extensions_heap, { _allocator, ext });
                        ice::array::push_back(_filter_extensions, ice::array::back(_filter_extensions_heap));
                    }
                }
                HSCP_ERROR_IF(
                    ice::E_Fail != ice::S_Success,
                    "Failed to parse config file '{}'...",
                    ice::String{ config_path_utf8 }
                );

                _allocator.deallocate(filemem);
                _allocator.deallocate(configmem);
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
            _allocator, _param_includes, &_tsched, _aioport
        );

        // Spawning one dedicated IO thread for AIO support. Allows us to read resources MUCH faster.
        ice::ResourceTrackerCreateInfo rtinfo{
            .predicted_resource_count = 1'000'000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{},
            .flags_io_wait = ice::TaskFlags{},
        };
        ice::UniquePtr<ice::ResourceTracker> tracker = ice::create_resource_tracker(
            _allocator, rtinfo
        );

        ice::ResourceProvider* provider = tracker->attach_provider(ice::move(fsprov));
        tracker->sync_resources();

        ice::Array<ice::Resource*> resources{ _allocator };
        if (provider->collect(resources) == 0)
        {
            HSCP_ERROR("No files where found in the included directories.");
            return 1;
        }

        // TODO: Filter out any resources we don't want and set the pointer to 'nullptr'
#if 0
        auto* const end = _filter_extensions._data + _filter_extensions._count;
        bool const found = std::find(
            _filter_extensions._data,
            end,
            ice::path::extension(resource->name())
        ) != end;

        if (found)
#endif

        return create_package(*tracker, resources);
    }

    auto run_explicit() noexcept -> ice::i32
    {
        // Prepare the output file name.
        _param_output = hscp_process_directory(_allocator, _param_output);

        // The paths that will be searched for loose file resources.
        ice::Array<ice::ResourceFileEntry> files{ _allocator,  };
        ice::array::reserve(files, ice::count(_inputs));
        for (ice::String file : _inputs)
        {
            ice::array::push_back(files, { .path = file });
        }

        ice::UniquePtr<ice::ResourceProvider> fsprov = ice::create_resource_provider_files(
            _allocator, files
        );

        // Spawning one dedicated IO thread for AIO support. Allows us to read resources MUCH faster.
        ice::ResourceTrackerCreateInfo rtinfo{
            .predicted_resource_count = 1'000'000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{},
            .flags_io_wait = ice::TaskFlags{},
        };
        ice::UniquePtr<ice::ResourceTracker> tracker = ice::create_resource_tracker(
            _allocator, rtinfo
        );

        ice::ResourceProvider* provider = tracker->attach_provider(ice::move(fsprov));
        tracker->sync_resources();

        ice::Array<ice::Resource*> resources{ _allocator };
        if (provider->collect(resources) == 0)
        {
            HSCP_ERROR("No files where found in the included directories.");
            return 1;
        }

        return create_package(*tracker, resources);
    }

    auto create_package(
        ice::ResourceTracker& tracker,
        ice::Span<ice::Resource* const> resources
    ) noexcept -> ice::i32
    {
        ice::Array<hailstorm::Data> resource_data{ _allocator };
        ice::Array<hailstorm::Data> resource_metas{ _allocator };
        ice::Array<ice::u32> resource_metamap{ _allocator };
        ice::Array<ice::ResourceHandle> resource_handles{ _allocator };
        ice::Array<std::string_view> resource_paths{ _allocator };

        ice::array::resize(resource_data, resources.size().u32());
        ice::array::resize(resource_metamap, resources.size().u32());
        ice::array::resize(resource_handles, resources.size().u32());
        ice::array::resize(resource_paths, resources.size().u32());

        // We serialize an empty meta object
        ice::ConfigBuilder meta{ _allocator };
        ice::Memory metamem = meta.finalize(_allocator);

        //ice::array::push_back(resource_metas, hsdata_view(metamem));

        std::atomic_uint32_t res_count = 0;
        ice::u32 res_idx = 0;
        for (ice::Resource* resource : resources)
        {
            if (resource != nullptr)
            {
                //ice::Metadata m = ice::meta_load(md);

                resource_paths[res_idx] = resource->name();
                resource_metamap[res_idx] = res_idx;
                resource_handles[res_idx] = tracker.find_resource(resource->uri());

                ice::Data md;
                ice::wait_for_result(ice::resource_meta(resource_handles[res_idx], md));
                ice::array::push_back(resource_metas, hsdata_view(md));

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
            using ice::operator""_Tms;
            ice::current_thread::sleep(1_Tms);
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
        ice::native_file::path_from_string(output, _param_output);
        HSCPWriteParams const write_params{
            .filename = output,
            .aioport = _aioport,
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
        ice::params_define(params, Param_Verbose, _param_verbose);

        ice::params_define(params, {
                .name = "-c,--config",
                .description = "Configuration file(s) with more detailed generation requirements.",
            },
            _param_configs
        );
        ice::params_define(params, {
                .name = "input",
                .description = "Input files to be stored in the HSC pack.",
                .flags = ice::ParamFlags::TakeAll
            },
            _inputs
        );
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
    ice::native_aio::AIOPort _aioport;

    // Params
    ice::Array<ice::String> _param_includes;
    ice::Array<ice::String> _param_configs;
    ice::HeapString<> _param_output;
    bool _param_verbose;

    ice::Array<ice::String> _inputs;

    // Filters
    ice::Array<ice::HeapString<>> _filter_extensions_heap;
    ice::Array<ice::String> _filter_extensions;

    // Workaround for Clang
    static inline ice::tool::ToolAppInstancer const& _workaroundSymbol = HailStormPackerApp::AppInstancer;
};
