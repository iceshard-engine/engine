#include "vk_shader_asset.hxx"

#if ISP_WINDOWS
#include <ice/render/render_shader.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset.hxx>
#include <ice/path_utils.hxx>

#include <shaderc/shaderc.hpp>

namespace ice::render::vk
{

    auto compile_shader_source(
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::AssetCompilerResult>
    {
        shaderc::CompileOptions compile_options{};

        // We don't use optimization in runtime-baked shaders
        compile_options.SetOptimizationLevel(shaderc_optimization_level_zero);
        compile_options.SetTargetSpirv(shaderc_spirv_version_1_6); // TODO: take from the metadata / platform settings?
        shaderc::Compiler compiler{};

        ice::String path = ice::resource_path(source);
        bool const is_vertex_shader = ice::string::substr(path, ice::string::size(path) - 9, 4) == "vert";

        ice::ResourceResult const result = co_await tracker.load_resource(source);
        shaderc::SpvCompilationResult const spv_result = compiler.CompileGlslToSpv(
            reinterpret_cast<char const*>(result.data.location),
            result.data.size.value,
            is_vertex_shader ? shaderc_shader_kind::shaderc_vertex_shader : shaderc_shader_kind::shaderc_fragment_shader,
            ice::string::begin(path),
            "main",
            compile_options
        );

        // Unload resource before continuing
        co_await tracker.unload_resource(source);

        // TODO List warnings

        // Check if we were successful
        if (spv_result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            // TODO List errors
            co_return { };
        }

        // Spv result is a 4byte BC table
        ice::usize const result_size = ice::size_of<ice::u32> * (spv_result.end() - spv_result.begin());
        ice::Memory const result_mem = alloc.allocate(result_size);
        ice::memcpy(result_mem.location, spv_result.begin(), result_size);
        co_return AssetCompilerResult{ .result = result_mem };
    }

    void VkShaderCompilerModule::v1_asset_compiler_api(ice::api::asset_compiler::v1::AssetCompilerAPI& api) noexcept
    {
        api.fn_compile_source = compile_shader_source;
    }

    auto asset_shader_state(
        void*,
        ice::AssetTypeDefinition const&,
        ice::Metadata const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        if (ice::path::extension(uri.path) == ".glsl")
        {
            return AssetState::Raw;
        }
        return AssetState::Baked;
    }

    auto asset_shader_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>
    {
        out_data = alloc.allocate(ice::meminfo_of<Data>);

        Data* shader_data = reinterpret_cast<Data*>(out_data.location);
        shader_data->location = data.location;
        shader_data->size = data.size;
        shader_data->alignment = data.alignment;
        co_return true;
    }

    void asset_type_shader_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        static ice::String extensions[]{ ".glsl", ".spv" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

        // Create the api object and pass it when registering the type
        ice::api::asset_compiler::v1::AssetCompilerAPI api{};
        VkShaderCompilerModule::v1_asset_compiler_api(api);

        ice::AssetCompiler const compiler{ api };
        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition, &compiler);
    }

    void VkShaderAssetModule::v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept
    {
        api.register_types_fn = asset_type_shader_definition;
    }

} // namespace ice::render::vk

#endif
