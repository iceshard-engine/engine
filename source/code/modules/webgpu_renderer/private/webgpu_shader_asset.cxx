/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_shader_asset.hxx"
#include "webgpu_utils.hxx"

#include <ice/render/render_shader.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset.hxx>
#include <ice/path_utils.hxx>

namespace ice::render::vk
{

    auto shader_resources() noexcept -> ice::Span<ice::String>
    {
        static ice::String supported_extensions[]{ ".wgsl" };
        return supported_extensions;
    }

    auto compile_shader_source(
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        ICE_LOG_WGPU(LogSeverity::Info, "Load WebGPU shader source: {}", ice::resource_path(source));
        ice::ResourceResult loaded = co_await tracker.load_resource(source);
        ICE_ASSERT_CORE(loaded.resource_status == ResourceStatus::Loaded);
        ICE_LOG_WGPU(LogSeverity::Info, "Compiled WebGPU shader resource: {}", ice::resource_path(source));

        ice::Memory result = alloc.allocate(loaded.data.size + 1_B);
        ice::memcpy(result, loaded.data);
        reinterpret_cast<char*>(result.location)[loaded.data.size.value] = '\0';
        co_return { result };
    }

    void WebGPUShaderCompilerModule::v1_resource_compiler_api(ice::api::resource_compiler::v1::ResourceCompilerAPI& api) noexcept
    {
        api.fn_supported_resources = shader_resources;
        api.fn_compile_source = compile_shader_source;
    }

    auto asset_shader_state(
        void*,
        ice::AssetTypeDefinition const&,
        ice::Metadata const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        if (ice::path::extension(uri.path) == ".wgsl")
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
        static ice::String extensions[]{ ".wgsl" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

        // Create the api object and pass it when registering the type
        ice::api::resource_compiler::v1::ResourceCompilerAPI api{};
        WebGPUShaderCompilerModule::v1_resource_compiler_api(api);

        ice::ResourceCompiler const compiler{ api };
        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition, &compiler);
    }

    void WebGPUShaderAssetModule::v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept
    {
        api.register_types_fn = asset_type_shader_definition;
    }

} // namespace ice::render::vk
