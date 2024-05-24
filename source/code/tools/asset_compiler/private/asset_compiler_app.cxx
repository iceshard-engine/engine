/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool_app.hxx>
#include <ice/module_register.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_format.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset_module.hxx>
#include <ice/asset_storage.hxx>
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
        , _inputs_meta{ _allocator }
        , _inputs{ _allocator }
        , _queue{ }
        , _scheduler{ _queue }
    {
        _thread = ice::create_thread(_allocator, _queue, { .exclusive_queue = 1, .wait_on_queue = 1, .debug_name = "compiler-thread",  });
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
            // ice::String const basename = ice::path::basename(_output);
            _asset_resource = ice::path::basename(_output);// ice::String{ ice::string::begin(_output), ice::string::end(basename) };
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
            { .predicted_resource_count = ice::count(_inputs), .io_dedicated_threads = 0 }
        );

        [[maybe_unused]]
        ice::ResourceProvider& resource_provider = *resource_tracker->attach_provider(
            ice::make_unique<AssetCompilerResourceProvider>(
                _allocator,
                _allocator,
                _inputs
            )
        );

        resource_tracker->sync_resources();

        // Setup the AssetCompiler asset type archive.
        ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(_allocator);
        ice::load_asset_type_definitions(_allocator, *_modules, *asset_types);

        ice::ResourceHandle* res = resource_tracker->find_resource(ice::URI{ ice::Scheme_URN, _asset_resource });
        ice::String const res_ext = ice::path::extension(ice::resource_origin(res));

        ice::ResourceCompiler const* resource_compiler = nullptr;
        ice::AssetTypeDefinition const* asset_definition = nullptr;
        for (ice::AssetType_Arg asset_type : asset_types->asset_types())
        {
            ice::AssetTypeDefinition const& def = asset_types->find_definition(asset_type);
            ice::ucount out_idx = 0;
            if (ice::search(def.resource_extensions, res_ext, out_idx))
            {
                resource_compiler = asset_types->find_compiler(asset_type);
                asset_definition = &def;
                break;
            }
        }

        if (asset_definition == nullptr)
        {
            ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Asset definition for resource '{}' is not available.", _asset_resource);
            return 1;
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

        // Check for expected asset states.
        ice::AssetState state = asset_definition->fn_asset_state(asset_definition->ud_asset_state, *asset_definition, meta, ice::resource_uri(res));
        if (state != ice::AssetState::Raw && state != ice::AssetState::Baked)
        {
            ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Unexpected asset state for resource {}. Expected 'Raw' or 'Baked'.", _asset_resource);
            return 2;
        }

        // If asset is in 'raw' format execute the resource compiler.
        if (state == ice::AssetState::Raw)
        {
            if (resource_compiler == nullptr)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Resource compiler for resource {} is not available.", _asset_resource);
                return 1;
            }

            ice::ucount out_idx = 0;
            if (ice::search(resource_compiler->fn_supported_resources(), res_ext, out_idx) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Resource compiler for resource {} is not available.", _asset_resource);
                return 1;
            }

            ice::Array<ice::ResourceHandle*> sources{ _allocator };
            if (resource_compiler->fn_collect_sources(res, *resource_tracker, sources) == false)
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
            if (resource_compiler->fn_collect_dependencies(res, *resource_tracker, dependencies) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied gathering dependencies for {}.", _asset_resource);
                return 1;
            }

            if (ice::wait_for(resource_compiler->fn_validate_source(res, *resource_tracker)) == false)
            {
                ICE_LOG(ice::LogSeverity::Critical, ice::LogTag::Tool, "Falied validation of sources for {}.", _asset_resource);
                return 1;
            }

            ice::Array<ice::ResourceCompilerResult> results{ _allocator };
            for (ice::ResourceHandle* source : sources)
            {
                ice::ResourceCompilerResult const result = ice::wait_for(
                    resource_compiler->fn_compile_source(
                        source, *resource_tracker, sources, dependencies, _allocator
                    )
                );
                ice::array::push_back(results, result);
            }

            // Build the final asset object
            ice::Memory const final_asset_data = resource_compiler->fn_finalize(res, results, dependencies, _allocator);
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
    ice::Array<ice::String> _inputs_meta;
    ice::Array<ice::String> _inputs;

    ice::TaskQueue _queue;
    ice::TaskScheduler _scheduler;
    ice::UniquePtr<ice::TaskThread> _thread;
};
