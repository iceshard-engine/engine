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

    struct ShaderCompilerContext
    {
        ice::i32 shader_type;
        ice::String shader_main;
    };

    auto shader_context(ice::ResourceCompilerCtx& ctx) noexcept -> ShaderCompilerContext*
    {
        return reinterpret_cast<ShaderCompilerContext*>(ctx.userdata);
    }

    bool compiler_context_prepare(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        ctx.userdata = alloc.create<ShaderCompilerContext>();
        return true;
    }

    bool compiler_context_cleanup(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        alloc.destroy(shader_context(ctx));
        return true;
    }

    auto compiler_compile_shader_source(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        ShaderCompilerContext& sctx = *shader_context(ctx);
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

        // TODO:
        sctx.shader_main = "main";
        sctx.shader_type = static_cast<ice::i32>(shader_stage);

        // Spv result is a 4byte BC table
        ice::usize const result_size = ice::size_of<ice::u32> * (spv_result.end() - spv_result.begin());
        ice::Memory const result_mem = alloc.allocate(result_size);
        ice::memcpy(result_mem.location, spv_result.begin(), result_size);
        co_return ResourceCompilerResult{ .result = result_mem };
    }

    auto compiler_build_shader_meta(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceCompilerResult const>,
        ice::Span<ice::URI const>,
        ice::MutableMetadata& out_meta
    ) noexcept -> ice::Task<bool>
    {
        ShaderCompilerContext sctx = *shader_context(ctx);
        ice::meta_set_int32(out_meta, "ice.shader.stage"_sid, sctx.shader_type);
        ice::meta_set_string(out_meta, "ice.shader.entry_point"_sid, sctx.shader_main);
        co_return true;
    }
#endif

    struct ShaderTools_ResourceCompilerModule : ice::Module<ShaderTools_ResourceCompilerModule>
    {
        static void v1_compiler_api(ice::api::resource_compiler::v1::ResourceCompilerAPI& api) noexcept
        {
#if ISP_WINDOWS || ISP_WEBAPP
            api.fn_prepare_context = compiler_context_prepare;
            api.fn_cleanup_context = compiler_context_cleanup;
            api.fn_supported_resources = compiler_supported_shader_resources;
            api.fn_compile_source = compiler_compile_shader_source;
            api.fn_build_metadata = compiler_build_shader_meta;
#endif // #if ISP_WINDOWS || ISP_WEBAPP
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto& negotiator) noexcept
        {
            LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_compiler_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(ShaderTools_ResourceCompilerModule);
    };

} // namespace ice
