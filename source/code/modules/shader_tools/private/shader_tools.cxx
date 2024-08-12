#include <ice/shader_tools.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/log_module.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_expected.hxx>
#include <ice/module.hxx>
#include <ice/profiler.hxx>
#include <ice/sort.hxx>

#include "shader_tools_asl.hxx"
#include "shader_tools_glsl.hxx"
#include "shader_tools_wgsl.hxx"

#if ISP_WEBAPP
#define strcmpi strcasecmp
#endif

namespace ice
{

    auto param_shader_target(ice::Span<ice::Shard const> params) noexcept -> ice::ShaderTargetPlatform;
    auto param_shader_stage(ice::Span<ice::Shard const> params) noexcept -> ice::ShaderStage;

    bool compiler_context_prepare(
        ice::Allocator& alloc,
        ice::ResourceCompilerCtx& ctx,
        ice::Span<ice::Shard const> params
    ) noexcept
    {
        ice::ShaderTargetPlatform const target = param_shader_target(params);
        ice::ShaderStage const stage = param_shader_stage(params);
        ctx.userdata = alloc.create<ShaderCompilerContext>(alloc, target, stage);
        return true;
    }

    bool compiler_context_cleanup(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        alloc.destroy(shader_context(ctx));
        return true;
    }

    auto compiler_supported_shader_resources(
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Span<ice::String>
    {
        if constexpr (ice::build::is_windows)
        {
            ice::ShaderTargetPlatform const target = param_shader_target(params);
            static ice::String supported_extensions[]{ ".asl", ".glsl", ".asl", ".hlsl" };
            return ice::span::subspan(
                ice::Span{ supported_extensions },
                // Select GLSL [0, 1] or HLSL span [2, 3]
                target == ShaderTargetPlatform::GLSL ? 0 : 2,
                // If we target 'WGSL' we only want to accept 'asl' files [0]
                2u - ice::u32(target == ShaderTargetPlatform::WGSL)
            );
        }
        else if constexpr (ice::build::is_webapp)
        {
            static ice::String supported_extensions[]{ ".asl", ".wgsl" };
            return supported_extensions;
        }
        else
        {
            return {};
        }
    }

    auto compiler_compile_shader_source(
        ice::ResourceCompilerCtx& resctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const> resources,
        ice::Span<ice::URI const> uris,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
#if ISP_WINDOWS
        ShaderCompilerContext& ctx = *shader_context(resctx);
        if (ctx.target == ShaderTargetPlatform::GLSL)
        {
            return glsl::compiler_compile_shader_source(resctx, source, tracker, resources, uris, alloc);
        }
        else
#endif
        {
            return wgsl::compiler_compile_shader_source(resctx, source, tracker, resources, uris, alloc);
        }
    }

    auto compiler_build_shader_meta(
        ice::ResourceCompilerCtx& resctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceCompilerResult const> resources,
        ice::Span<ice::URI const> uris,
        ice::MutableMetadata& out_meta
    ) noexcept -> ice::Task<bool>
    {
        ShaderCompilerContext sctx = *shader_context(resctx);
        ice::meta_set_int32(out_meta, "ice.shader.stage"_sid, sctx.shader_type);
        ice::meta_set_string(out_meta, "ice.shader.entry_point"_sid, sctx.shader_main);
        co_return true;
    }

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


    auto param_shader_target(ice::Span<ice::Shard const> params) noexcept -> ice::ShaderTargetPlatform
    {
        ice::ShaderTargetPlatform target = ice::build::is_webapp
            ? ShaderTargetPlatform::WGSL
            : ShaderTargetPlatform::GLSL;

        if constexpr (ice::build::is_windows)
        {
            ice::u32 idx;
            if (ice::search(params, ice::ShardID_ShaderTargetPlatform, idx) == false)
            {
                return target;
            }

            // If it's not a valid enum we also check for strings matching the names.
            if (ice::shard_inspect(params[idx], target) == false)
            {
                const char* target_name;
                if (ice::shard_inspect(params[idx], target_name) == false)
                {
                    ICE_LOG(
                        LogSeverity::Error, LogTag::Tool,
                        "Invalid value type provided for param 'shader:target'. 'ice::ShaderPlatformTarget' enum or 'char const*' expected.",
                    );
                }
                else
                {
                    if (strcmpi(target_name, "GLSL") == 0)
                    {
                        target = ShaderTargetPlatform::GLSL;
                    }
                    else if (strcmpi(target_name, "HLSL") == 0)
                    {
                        target = ShaderTargetPlatform::HLSL;
                    }
                    else if (strcmpi(target_name, "WGSL") == 0)
                    {
                        target = ShaderTargetPlatform::WGSL;
                    }
                    else
                    {
                        ICE_LOG(
                            LogSeverity::Error, LogTag::Tool,
                            "Invalid value '{}' provided for param 'shader:target'",
                            target_name
                        );
                    }
                }
            }
        }

        ICE_ASSERT(target != ShaderTargetPlatform::HLSL, "Shader platform 'HLSL' is not yet supported by IceShard!");
        return target;
    }

    auto param_shader_stage(ice::Span<ice::Shard const> params) noexcept -> ice::ShaderStage
    {
        ice::ShaderStage target = ShaderStage::Compiled;

        if constexpr (ice::build::is_windows)
        {
            ice::u32 idx;
            if (ice::search(params, ice::ShardID_ShaderStage, idx) == false)
            {
                return target;
            }

            // If it's not a valid enum we also check for strings matching the names.
            if (ice::shard_inspect(params[idx], target) == false)
            {
                const char* target_name;
                if (ice::shard_inspect(params[idx], target_name) == false)
                {
                    ICE_LOG(
                        LogSeverity::Error, LogTag::Tool,
                        "Invalid value type provided for param 'shader:stage'. 'ice::ShaderStage' enum or 'char const*' expected.",
                    );
                }
                else
                {
                    if (strcmpi(target_name, "Compiled") == 0)
                    {
                        target = ShaderStage::Compiled;
                    }
                    else if (strcmpi(target_name, "Transpiled") == 0)
                    {
                        target = ShaderStage::Transpiled;
                    }
                    else
                    {
                        ICE_LOG(
                            LogSeverity::Error, LogTag::Tool,
                            "Invalid value '{}' provided for param 'shader:stage'",
                            target_name
                        );
                    }
                }
            }
        }

        return target;
    }

} // namespace ice

#if ISP_WEBAPP
#undef strcmpi
#endif
