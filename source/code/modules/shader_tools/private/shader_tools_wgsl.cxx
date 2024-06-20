#include "shader_tools_wgsl.hxx"

#if ISP_WEBAPP
#include <ice/render/render_shader.hxx>
#include <ice/string/string.hxx>
#include <ice/path_utils.hxx>
#include <ice/log.hxx>

namespace ice
{

    auto compiler_supported_shader_resources() noexcept -> ice::Span<ice::String>
    {
        static ice::String supported_extensions[]{ /*".asl",*/ ".wgsl" };
        return supported_extensions;
    }

    bool compiler_context_prepare(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
        return true;
    }

    bool compiler_context_cleanup(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept
    {
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
        ICE_LOG(LogSeverity::Info, LogTag::Tool, "Load WebGPU shader source: {}", ice::resource_path(source));
        ice::ResourceResult loaded = co_await tracker.load_resource(source);
        ICE_ASSERT_CORE(loaded.resource_status == ResourceStatus::Loaded);
        ICE_LOG(LogSeverity::Info, LogTag::Tool, "Compiled WebGPU shader resource: {}", ice::resource_path(source));

        ice::Memory result = alloc.allocate(loaded.data.size + 1_B);
        ice::memcpy(result, loaded.data);
        reinterpret_cast<char*>(result.location)[loaded.data.size.value] = '\0';
        co_return { result };
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
        ice::String const path = ice::resource_path(source);
        ice::String const type = ice::string::substr(path, ice::string::find_last_of(path, '.') - 4, 4);
        if (type == "vert")
        {
            ice::meta_set_int32(out_meta, "ice.shader.stage"_sid, static_cast<ice::i32>(ice::render::ShaderStageFlags::VertexStage));
        }
        else
        {
            ice::meta_set_int32(out_meta, "ice.shader.stage"_sid, static_cast<ice::i32>(ice::render::ShaderStageFlags::FragmentStage));
        }
        ice::meta_set_string(out_meta, "ice.shader.entry_point"_sid, "main");
        co_return true;
    }

    auto transpile_shader_asl_to_wgsl(
        ice::Allocator& allocator,
        ice::Data asl_source
    ) noexcept -> ice::HeapString<>
    {
        ice::HeapString<> result{ allocator };
        return result;
    }

} // namespace ice

#endif // #if ISP_WEBAPP
