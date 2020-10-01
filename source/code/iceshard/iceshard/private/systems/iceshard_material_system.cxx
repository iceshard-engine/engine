#include "iceshard_material_system.hxx"

#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_buffers.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_model.hxx>

#include <core/pod/array.hxx>
#include <core/memory.hxx>

namespace iceshard
{

    static constexpr auto MaxTextureData = 4096u * 4096u * 4u * 3;

    IceshardMaterialSystem::IceshardMaterialSystem(
        core::allocator& alloc,
        asset::AssetSystem& asset_system,
        iceshard::renderer::RenderSystem& render_system
    ) noexcept
        : MaterialSystem{ }
        , _allocator{ alloc }
        , _asset_system{ asset_system }
        , _render_system{ render_system }
        , _material_resources{ _allocator }
    {
        auto buffers_view = core::pod::array::create_view(&_command_buffer, 1);
        iceshard::renderer::create_command_buffers(
            iceshard::renderer::api::CommandBufferType::Primary,
            buffers_view
        );
        _transfer_buffer = iceshard::renderer::create_buffer(
            iceshard::renderer::api::BufferType::TransferBuffer,
            MaxTextureData // Max texture size
        );
    }

    IceshardMaterialSystem::~IceshardMaterialSystem() noexcept
    {
        for (auto const& entry : _material_resources)
        {
            _render_system.destroy_pipeline(entry.value.name);
            _render_system.destroy_resource_set(entry.value.name);
        }
    }

    bool IceshardMaterialSystem::create_material(
        core::stringid_arg_type name,
        Material const& definition,
        iceshard::renderer::RenderPipelineLayout layout
    ) noexcept
    {
        using iceshard::renderer::ResourceSet;
        using iceshard::renderer::RenderPipelineLayout;
        using asset::AssetStatus;
        using asset::AssetData;

        AssetStatus load_status;
        AssetData shader_asset_data[2];

        core::stringid_type const texture_names[]{
            definition.texture_normal,
            definition.texture_diffuse,
            definition.texture_specular,
        };
        AssetData texture_asset_data[core::size(texture_names)];

        {
            load_status = _asset_system.load(
                asset::Asset{ definition.shader_vertex, asset::AssetType::Shader },
                shader_asset_data[0]
            );
            IS_ASSERT(load_status == AssetStatus::Loaded, "Shader {} is not available!", definition.shader_vertex);

            load_status = _asset_system.load(
                asset::Asset{ definition.shader_fragment, asset::AssetType::Shader },
                shader_asset_data[1]
            );
            IS_ASSERT(load_status == AssetStatus::Loaded, "Shader {} is not available!", definition.shader_fragment);
        }

        for (uint32_t i = 0; i < core::size(texture_names); ++i)
        {
            if (texture_names[i] != core::stringid_invalid)
            {
                load_status = _asset_system.load(
                    asset::Asset{ texture_names[i], asset::AssetType::Texture },
                    texture_asset_data[i]
                );
                IS_ASSERT(load_status == AssetStatus::Loaded, "Texture {} is not available!", texture_names[i]);
            }
        }

        if (_render_system.get_resource_set(name) == ResourceSet::Invalid)
        {
            auto pipeline_handle = _render_system.create_pipeline(
                name, layout,
                core::pod::array::create_view(shader_asset_data)
            );

            using iceshard::renderer::RenderResource;
            using iceshard::renderer::RenderResourceType;
            using iceshard::renderer::RenderResourceSetInfo;
            using iceshard::renderer::RenderResourceSetUsage;
            using iceshard::renderer::api::DataView;
            using iceshard::renderer::api::CommandBufferUsage;
            using iceshard::renderer::api::UpdateTextureData;

            using iceshard::renderer::map_buffer;
            using iceshard::renderer::unmap_buffer;
            using iceshard::renderer::commands::end;
            using iceshard::renderer::commands::begin;
            using iceshard::renderer::commands::update_texture;

            core::pod::Array<RenderResource> resources{ _allocator };
            core::pod::array::reserve(resources, 3);

            begin(_command_buffer, CommandBufferUsage::RunOnce);

            uint32_t data_offset = 0;
            for (uint32_t i = 0; i < core::size(texture_names); ++i)
            {
                if (texture_names[i] != core::stringid_invalid)
                {
                    iceshard::renderer::data::Texture const* texture = reinterpret_cast<iceshard::renderer::data::Texture const*>(
                        texture_asset_data[i].content.data()
                    );

                    core::data_view image_data{
                        texture->data,
                        texture->width * texture->height * 4
                    };

                    auto const texture_handle = iceshard::renderer::create_texture(texture_names[i], texture_asset_data[i]);

                    DataView buffer_view;
                    map_buffer(_transfer_buffer, buffer_view);
                    memcpy(
                        core::memory::utils::pointer_add(buffer_view.data, data_offset),
                        image_data.data(),
                        image_data.size()
                    );
                    unmap_buffer(_transfer_buffer);

                    update_texture(_command_buffer, texture_handle, _transfer_buffer,
                        UpdateTextureData
                        {
                            .image_extent = core::math::vec2<core::math::u32>(texture->width, texture->height),
                            .buffer_offset = data_offset
                        }
                    );


                    // Update offset
                    data_offset += image_data.size();
                    data_offset = (data_offset % 4) ? 4 - (data_offset % 4) : data_offset;

                    // Update resource description
                    core::pod::array::push_back(resources,
                        RenderResource{
                            .binding = i + 2,
                            .type = RenderResourceType::ResTexture2D,
                            .handle = {.texture = texture_handle },
                        }
                    );
                }
            }

            end(_command_buffer);
            _sleep(10);
            _render_system.submit_command_buffer_v2(_command_buffer);

            auto resource_set_handle = _render_system.create_resource_set(
                name, layout,
                RenderResourceSetInfo{ .usage = RenderResourceSetUsage::MaterialData },
                resources
            );

            IS_ASSERT(
                core::pod::hash::has(_material_resources, core::hash(name)) == false,
                "Material already loaded!"
            );
            core::pod::hash::set(_material_resources, core::hash(name),
                MaterialInfo{
                    .name = name,
                    .resources = {
                        .pipeline = pipeline_handle,
                        .resource = resource_set_handle,
                    }
                }
            );
            return true;
        }
        return false;
    }

    bool IceshardMaterialSystem::get_material(core::stringid_arg_type name, MaterialResources& resources) noexcept
    {
        bool const result = core::pod::hash::has(_material_resources, core::hash(name));
        if (result == true)
        {
            static MaterialInfo invalid_info{ };
            resources = core::pod::hash::get(_material_resources, core::hash(name), invalid_info).resources;
        }
        else
        {
            resources.pipeline = iceshard::renderer::api::Pipeline::Invalid;
            resources.resource = iceshard::renderer::api::ResourceSet::Invalid;
        }
        return result;
    }

} // namespace iceshard
