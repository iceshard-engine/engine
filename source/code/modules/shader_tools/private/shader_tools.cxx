#include <ice/shader_tools.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/log_module.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_expected.hxx>
#include <ice/module.hxx>
#include <ice/profiler.hxx>

#include "shader_tools_asl.hxx"
#include "shader_tools_glsl.hxx"
#include "shader_tools_wgsl.hxx"

#if ISP_WINDOWS
#include <shaderc/shaderc.hpp>
#endif

namespace ice
{

#if ISP_WINDOWS
    static constexpr ice::ErrorCode E_FailedToTranspileASLShaderToGLSL{ "E.1101:ResourceCompiler:Shader compiler failed to create GLSL shader from ASL." };

    auto shader_resources() noexcept -> ice::Span<ice::String>
    {
        static ice::String supported_extensions[]{ ".asl", ".glsl" };
        return supported_extensions;
    }

    bool collect_shader_sources(
        ice::ResourceHandle* resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Array<ice::ResourceHandle*>& out_sources
    ) noexcept
    {
        return false;
    }

    auto load_shader_source(
        ice::Allocator& alloc,
        ice::ResourceTracker& tracker,
        ice::ResourceHandle* source,
        ice::render::ShaderStageFlags shader_stage,
        ice::HeapString<>& out_result
    ) noexcept -> ice::TaskExpected<ice::String, ice::ErrorCode>
    {
        ice::ResourceResult const result = co_await tracker.load_resource(source);
        ice::String const path = ice::resource_origin(source);

        if (ice::path::extension(path) == ".asl")
        {
            auto import_loader = ice::create_script_loader(alloc, tracker);

            out_result = ice::transpile_shader_asl_to_glsl(
                alloc,
                *import_loader,
                result.data,
                { shader_stage }
            );

            // Failed to transpile
            if (ice::string::empty(out_result))
            {
                co_return E_FailedToTranspileASLShaderToGLSL;
            }
            co_return out_result;
        }
        else
        {
            co_return ice::String{
                (char const*) result.data.location,
                (ice::ucount) result.data.size.value
            };
        }
    }

    bool context_prepare(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        return true;
    }

    bool context_cleanup(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        return true;
    }

    auto compile_shader_source(
        ice::ResourceCompilerCtx&,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        shaderc::CompileOptions compile_options{};

        // We don't use optimization in runtime-baked shaders
        compile_options.SetOptimizationLevel(shaderc_optimization_level_zero);
        compile_options.SetTargetSpirv(shaderc_spirv_version_1_6); // TODO: take from the metadata / platform settings?
        shaderc::Compiler compiler{};

        ice::String const path = ice::resource_origin(source);
        ice::String const ext = ice::path::extension(path);
        bool const is_vertex_shader = ice::string::substr(path, ice::string::size(path) - (4 + ice::size(ext)), ice::size(ext)) == "vert";

        ice::render::ShaderStageFlags const shader_stage = is_vertex_shader
            ? ice::render::ShaderStageFlags::VertexStage
            : ice::render::ShaderStageFlags::FragmentStage;

        ice::HeapString<> transpiled_result{ alloc };
        ice::Expected<ice::String, ice::ErrorCode> result = co_await load_shader_source(
            alloc, tracker, source, shader_stage, transpiled_result
        );

        if (result.valid() == false)
        {
            ICE_LOG(LogSeverity::Error, LogTag::System, "Failed to load shader '{}' with error '{}'", path, result.error());
            co_return { };
        }

        ice::String const glsl_source = result.value();

        shaderc::SpvCompilationResult const spv_result = compiler.CompileGlslToSpv(
            ice::string::begin(glsl_source),
            ice::string::size(glsl_source),
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
            ICE_LOG(LogSeverity::Error, LogTag::System, "Failed to load shader '{}' with error '{}'", path, spv_result.GetErrorMessage());
            co_return { };
        }

        // Spv result is a 4byte BC table
        ice::usize const result_size = ice::size_of<ice::u32> * (spv_result.end() - spv_result.begin());
        ice::Memory const result_mem = alloc.allocate(result_size);
        ice::memcpy(result_mem.location, spv_result.begin(), result_size);
        co_return ResourceCompilerResult{ .result = result_mem };
    }
#endif

    struct ShaderTools_ResourceCompilerModule : ice::Module<ShaderTools_ResourceCompilerModule>
    {
        static void v1_compiler_api(ice::api::resource_compiler::v1::ResourceCompilerAPI& api) noexcept
        {
            // api.fn_collect_sources =
            api.fn_prepare_context = context_prepare;
            api.fn_cleanup_context = context_cleanup;
            api.fn_supported_resources = shader_resources;
            api.fn_compile_source = compile_shader_source;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto& negotiator) noexcept
        {
            LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_compiler_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(ShaderTools_ResourceCompilerModule);
    };

} // namespace ice
