/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool_app.hxx>
#include <ice/module_register.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_format.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/resource_compiler.hxx>
#include <ice/resource_meta.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_utils.hxx>
#include <ice/sort.hxx>
#include <ice/log.hxx>

#include "asset_compiler_resource_provider.hxx"

ice::ParamInstance<bool> Param_Verbose{ "", "-v,--verbose", "Verbosity of the process." };

class AssetCompilerApp : public ice::tool::ToolApp<AssetCompilerApp>
{
public:
    AssetCompilerApp() noexcept
        : ToolApp<AssetCompilerApp>{}
        , _output{ }
        , _asset_resource{ }
        , _includes{ _allocator }
        , _params{ _allocator }
        , _inputs_meta{ _allocator }
        , _inputs{ _allocator }
        , _queue{ }
        , _scheduler{ _queue }
    {
        _thread = ice::create_thread(_allocator, _queue, { .exclusive_queue = 1, .wait_on_queue = 1, .debug_name = "compiler-thread",  });
    }

    bool parse_parameter(this AssetCompilerApp& self, ice::Span<ice::String const> results) noexcept
    {
        if (ice::count(results) == 2)
        {
            ice::array::push_back(
                self._params,
                ice::shard(results[0], ice::string::begin(results[1]))
            );
        }
        if (ice::count(results) == 1)
        {
            ice::array::push_back(
                self._params,
                ice::shard(results[0], true)
            );
        }
        return true;
    }

    bool setup(ice::Params& params) noexcept override
    {
        ice::params_define(params, {
                .name = "-o,--output",
                .description = "The asset file to be created.",
                // .type_name = "PATH",
                .flags = ice::ParamFlags::IsRequired
            },
            _output
        );
        ice::params_define(params, {
                .name = "-r,--resource,--name",
                .description = "The name of the resource the asset to be created from. By default it's the first 'input' path provided.",
                .type_name = "NAME",
            },
            _asset_resource
        );
        ice::params_define(params, {
                .name = "-c,--compiler",
                .description = "A module that implements the asset type and a compiler for the resource that should be compiled.",
                .type_name = "VALUE",
                .flags = ice::ParamFlags::ValidateFile,
            },
            _compiler
        );
        ice::params_define(params, {
                .name = "-i,--include",
                .description = "Additional include directories to search for dependent resources",
                // .type_name = "PATH",
                .flags = ice::ParamFlags::ValidateDirectory | ice::ParamFlags::TakeAll,
            },
            _includes
        );
        ice::params_define_custom(params, {
                .name = "-p,--param",
                .description = "Additional parameters passed to the selected resource compiler. (-p name value)",
                .typesize = { 1, 2 },
                .flags = ice::ParamFlags::TakeAll,
            }, this, (ice::ParamsCustomCallback) &AssetCompilerApp::parse_parameter
        );
        ice::params_define(params, {
                .name = "-m,--metadata",
                .description = "Metadata descriptions for the asset. Can provide multiple files that will be combined in order.",
                // .type_name = "PATH",
                .flags = ice::ParamFlags::ValidateFile,
            },
            _inputs_meta
        );
        ice::params_define(params, {
                .name = "input",
                .description = "Input files required to create an asset",
                // .type_name = "PATH",
                .min = 1,
                .flags = ice::ParamFlags::ValidateFile | ice::ParamFlags::IsRequired,
            },
            _inputs
        );
        ice::params_register_globals(params);
        return true;
    }

    auto run() noexcept -> ice::i32 override
    {
        if (_modules->load_module(_allocator, _compiler))
        {
            ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Failed to load compiler module '{}'", _compiler);
            return 1;
        }

        if (ice::string::empty(_asset_resource))
        {
            _asset_resource = ice::path::basename(_output);
        }

        ICE_LOG_IF(
            Param_Verbose,
            ice::LogSeverity::Retail, ice::LogTag::Tool,
            "Creating asset '{}' from {} sources and {} metadata files.",
            _asset_resource, ice::count(_inputs), ice::count(_inputs_meta)
        );

        // Setup the AssetCompiler resource provider.
        ice::UniquePtr<ice::ResourceTracker> resource_tracker = ice::create_resource_tracker(
            _allocator,
            _scheduler,
            { .predicted_resource_count = 10'000, .io_dedicated_threads = 0 }
        );

        resource_tracker->attach_provider(
            ice::make_unique<AssetCompilerResourceProvider>(
                _allocator, _allocator, _inputs
            )
        );
        resource_tracker->attach_provider(
            ice::create_resource_provider(_allocator, _includes, &_scheduler)
        );

        resource_tracker->sync_resources();

        ice::ResourceHandle* res = resource_tracker->find_resource(ice::URI{ ice::Scheme_URN, _asset_resource });
        if (res == nullptr)
        {
            ICE_LOG(
                ice::LogSeverity::Critical, ice::LogTag::Tool,
                "The selected resource '{}' was not found.",
                _asset_resource
            );
            return 1;
        }

        ice::String const res_ext = ice::path::extension(ice::resource_origin(res));

        ice::ResourceCompiler const* resource_compiler = nullptr;

        ice::Array<ice::api::resource_compiler::v1::ResourceCompilerAPI> resource_compilers{ _allocator };
        if (_modules->query_apis(resource_compilers))
        {
            for (ice::api::resource_compiler::v1::ResourceCompilerAPI& compiler : resource_compilers)
            {
                if (compiler.fn_supported_resources == nullptr)
                {
                    continue;
                }

                ice::ucount out_idx = 0;
                if (ice::search(compiler.fn_supported_resources(_params), res_ext, out_idx))
                {
                    resource_compiler = &compiler;
                    break;
                }
            }
        }

        // Create the metadata object
        ice::MutableMetadata meta{ _allocator };
        for (ice::String input_meta : _inputs_meta)
        {
            ice::native_file::HeapFilePath input_meta_path{ _allocator };
            ice::native_file::path_from_string(input_meta, input_meta_path);
            ice::native_file::File file = ice::native_file::open_file(input_meta_path, ice::native_file::FileOpenFlags::Read);

            static constexpr ice::ErrorCode ReadMetadataError{ "E.8000:AssetCompiler:Failed to read metadata file!" };
            ice::Result result = ReadMetadataError;
            if (file)
            {
                ice::Memory memory = _allocator.allocate(ice::native_file::sizeof_file(file));
                if (ice::native_file::read_file(file, memory.size, memory) == memory.size)
                {
                    result = ice::meta_deserialize_from(meta, ice::data_view(memory));
                }
                _allocator.deallocate(memory);
            }

            ICE_LOG_IF(result == ice::E_Fail, ice::LogSeverity::Warning, ice::LogTag::Tool, "{}", result.error());
        }

        // If asset is in 'raw' format execute the resource compiler.
        // if (state == ice::AssetState::Raw)
        {
            if (resource_compiler == nullptr || resource_compiler->fn_supported_resources == nullptr)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Resource compiler for resource {} is not available.", _asset_resource);
                return 1;
            }

            ice::ucount out_idx = 0;
            if (ice::search(resource_compiler->fn_supported_resources(_params), res_ext, out_idx) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Resource compiler for resource {} is not available.", _asset_resource);
                return 1;
            }

            ice::ResourceCompilerCtx ctx{ .userdata = nullptr };
            ice::api::resource_compiler::v1::ResourceCompilerCtxCleanup ctx_cleanup{ _allocator, ctx, resource_compiler->fn_cleanup_context };
            if (resource_compiler->fn_prepare_context && resource_compiler->fn_prepare_context(_allocator, ctx, _params) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied preparing compiler context for {}.", _asset_resource);
                return 1;
            }

            ice::Array<ice::ResourceHandle*> sources{ _allocator };
            if (resource_compiler->fn_collect_sources(ctx, res, *resource_tracker, sources) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied gathering sources for {}.", _asset_resource);
                return 1;
            }

            // If empty we add our own handle to the list
            if (ice::array::empty(sources))
            {
                ice::array::push_back(sources, res);
            }

            ice::Array<ice::URI> dependencies{ _allocator };
            if (resource_compiler->fn_collect_dependencies(ctx, res, *resource_tracker, dependencies) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied gathering dependencies for {}.", _asset_resource);
                return 1;
            }

            if (ice::wait_for_result(resource_compiler->fn_validate_source(ctx, res, *resource_tracker)) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied validation of sources for {}.", _asset_resource);
                return 1;
            }

            ice::Array<ice::ResourceCompilerResult> results{ _allocator };
            for (ice::ResourceHandle* source : sources)
            {
                auto fn = [](ice::ResourceCompilerResult& out_result, ice::Task<ice::ResourceCompilerResult> task) noexcept -> ice::Task<>
                {
                    out_result = co_await task;
                };

                ice::ManualResetEvent waitev{};
                ice::ResourceCompilerResult result;
                ice::manual_wait_for(
                    waitev,
                    fn(
                        result,
                        resource_compiler->fn_compile_source(
                            ctx, source, *resource_tracker, sources, dependencies, _allocator
                        )
                    )
                );

                while(waitev.is_set() == false)
                {
                    _queue.process_all();
                }

                ice::array::push_back(results, result);
            }

            if (ice::wait_for_result(resource_compiler->fn_build_metadata(ctx, res, *resource_tracker, results, dependencies, meta)) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied building metadata for {}.", _asset_resource);
                return 1;
            }

            // Build the final asset object
            ice::Memory const final_asset_data = resource_compiler->fn_finalize(ctx, res, results, dependencies, _allocator);
            if (final_asset_data.location != nullptr)
            {
                ice::Memory const final_meta_data = ice::meta_save(meta, _allocator);

                // Calc meta offset
                ice::u32 meta_offset = (ice::u32) sizeof(ice::ResourceFormatHeader) + ice::string::size(_asset_resource) + 1;
                ice::u32 const meta_diff = meta_offset & 0x7;
                if (meta_diff != 0)
                {
                    meta_offset += (8 - meta_diff);
                }

                // Prepare the resource header struct
                ice::ResourceFormatHeader const rfh{
                    .magic = ice::Constant_ResourceFormatMagic,
                    .version = ice::Constant_ResourceFormatVersion,
                    .name_size = ice::string::size(_asset_resource),
                    .meta_offset = meta_offset,
                    .meta_size = static_cast<ice::u32>(final_meta_data.size.value),
                    .offset = (ice::u32) (meta_offset + final_meta_data.size.value),
                    .size = static_cast<ice::u32>(final_asset_data.size.value),
                };

                ice::native_file::HeapFilePath output_path{ _allocator };
                ice::native_file::path_from_string(_output, output_path);
                ice::native_file::File output_file = ice::native_file::open_file(output_path, ice::native_file::FileOpenFlags::Write);

                char const filler[8]{0};
                ice::Data const file_parts[]{
                    ice::data_view(rfh), // Header
                    ice::string::data_view(_asset_resource), // Name
                    ice::Data{ &filler, meta_diff, ice::ualign::b_1 },
                    ice::data_view(final_meta_data), // Metadata
                    ice::data_view(final_asset_data) // Data
                };

                // Write all parts
                for (ice::Data file_part : file_parts)
                {
                    if (ice::native_file::append_file(output_file, file_part) != file_part.size)
                    {
                        ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied to write final output file {}.", _asset_resource);
                    }
                }

                _allocator.deallocate(final_meta_data);
                _allocator.deallocate(final_asset_data);
            }

            // Release partial results.
            for (ice::ResourceCompilerResult& result : results)
            {
                _allocator.deallocate(result.result);
            }
        }

        // Write the final asset file.
        return 0;
    }

public: // Tool information
    auto name() const noexcept -> ice::String override { return "asset_compiler"; }
    auto version() const noexcept -> ice::String override { return "0.1.0"; }
    auto description() const noexcept -> ice::String override
    {
        return "Compiles input files into a single asset resources. This resource is optimized for loading using IceShard.";
    }

private:
    ice::String _output;
    ice::String _asset_resource;
    ice::String _compiler;
    ice::Array<ice::String> _includes;
    ice::Array<ice::String> _inputs_meta;
    ice::Array<ice::String> _inputs;
    ice::Array<ice::Shard> _params;

    ice::TaskQueue _queue;
    ice::TaskScheduler _scheduler;
    ice::UniquePtr<ice::TaskThread> _thread;
};
