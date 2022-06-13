#include "trait_render_glyphs.hxx"
#include <ice/asset_storage.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_sync_wait.hxx>

#include <ice/shard.hxx>
#include <ice/shard_container.hxx>
#include <ice/profiler.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_font.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/game_render_traits.hxx>

namespace ice
{

    namespace detail
    {

        auto load_font_shader(ice::AssetStorage& assets, ice::Utf8String name) noexcept -> ice::Task<ice::Data>
        {
            ice::Asset const asset = co_await assets.request(ice::render::AssetType_Shader, name, ice::AssetState::Baked);
            ICE_ASSERT(asset_check(asset, AssetState::Baked), "Shader not available!");
            co_return asset.data;
        }

    } // namespace detail


    IceWorldTrait_RenderGlyphs::IceWorldTrait_RenderGlyphs(ice::Allocator& alloc) noexcept
        : _fonts{ alloc }
    {
    }

    void IceWorldTrait_RenderGlyphs::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        ice::vec2u const extent = gfx_device.swapchain().extent();
        _framebuffer_size = ice::vec2f(extent.x, extent.y);

        using namespace ice::gfx;
        using namespace ice::render;

        Renderpass renderpass = ice::gfx::find_resource<Renderpass>(gfx_device.resource_tracker(), "ice.gfx.renderpass.default"_sid);
        RenderDevice& device = gfx_device.device();

        _shader_stages[0] = ShaderStageFlags::VertexStage;
        _shader_stages[1] = ShaderStageFlags::FragmentStage;
        _shaders[0] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[0] });
        _shaders[1] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[1] });
        _shaders[2] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[2] });
        _shaders[3] = device.create_shader(ShaderInfo{ .shader_data = _shader_data[3] });

        SamplerInfo sampler_info{
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode = {
                .u = SamplerAddressMode::RepeatMirrored,
                .v = SamplerAddressMode::ClampToEdge,
                .w = SamplerAddressMode::ClampToEdge,
            },
            .mip_map_mode = SamplerMipMapMode::Nearest,
        };

        _sampler = device.create_sampler(sampler_info);

        ResourceSetLayoutBinding const resource_bindings[]{
            ResourceSetLayoutBinding
            {
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::Sampler,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
            ResourceSetLayoutBinding
            {
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::SampledImage,
                .shader_stage_flags = ShaderStageFlags::FragmentStage
            },
        };

        _resource_set_layouts[0] = device.create_resourceset_layout({ resource_bindings + 0, 1 });
        _resource_set_layouts[1] = device.create_resourceset_layout({ resource_bindings + 1, 1 });
        device.create_resourcesets({ _resource_set_layouts + 0, 1 }, _resource_sets);

        ResourceUpdateInfo const update_resources[]
        {
            ResourceUpdateInfo
            {
                .sampler = _sampler,
            },
        };

        ResourceSetUpdateInfo const update_sets[]
        {
            ResourceSetUpdateInfo
            {
                .resource_set = _resource_sets[0],
                .resource_type = ResourceType::Sampler,
                .binding_index = 0,
                .array_element = 0,
                .resources = update_resources,
            }
        };

        device.update_resourceset(update_sets);


        PipelinePushConstant const push_constants[]
        {
            PipelinePushConstant
            {
                .shader_stage_flags = ShaderStageFlags::VertexStage,
                .offset = 0,
                .size = sizeof(ice::vec2f) * 2,
            }
        };

        PipelineLayoutInfo const layout_info
        {
            .push_constants = push_constants,
            .resource_layouts = { _resource_set_layouts + 0, 2 },
        };

        _pipeline_layout = device.create_pipeline_layout(layout_info);


        ShaderInputAttribute attribs[]{
            ShaderInputAttribute{
                .location = 0,
                .offset = 0,
                .type = ShaderAttribType::Vec2f
            },
            ShaderInputAttribute{
                .location = 1,
                .offset = 8,
                .type = ShaderAttribType::Vec2f
            },
        };

        ShaderInputBinding bindings[]{
            ShaderInputBinding{
                .binding = 0,
                .stride = 16,
                .instanced = false,
                .attributes = { attribs + 0, 2 }
            },
        };

        PipelineInfo pipeline_info{
            .layout = _pipeline_layout,
            .renderpass = renderpass,
            .shaders = { _shaders + 0, 2 },
            .shaders_stages = _shader_stages,
            .shader_bindings = bindings,
            .primitive_topology = PrimitiveTopology::TriangleList,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _pipeline = device.create_pipeline(pipeline_info);

        pipeline_info.shaders = { _shaders + 2, 2 };
        _debug_pipeline = device.create_pipeline(pipeline_info);

        ice::u32 constexpr glyph_vec2f_count = 6;
        ice::u32 constexpr glyphs_vertices_bytes = glyph_vec2f_count * 4096 * sizeof(ice::vec4f);

        _vertex_buffer = device.create_buffer(BufferType::Vertex, glyphs_vertices_bytes);
    }

    void IceWorldTrait_RenderGlyphs::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;

        ice::Span<ice::vec4f const> vertice_array = engine_frame.named_span<ice::vec4f>("ice.glyph-render.vertices"_sid);
        if (vertice_array.empty() == false)
        {
            BufferUpdateInfo const update_info[]
            {
                BufferUpdateInfo{
                    .buffer = _vertex_buffer,
                    .data = { vertice_array.data(), (ice::u32) vertice_array.size_bytes(), alignof(ice::vec4f) },
                    .offset = 0
                }
            };

            gfx_device.device().update_buffers(update_info);
        }

        gfx_frame.set_stage_slot(ice::Constant_GfxStage_DrawGlyphs, this);
    }

    void IceWorldTrait_RenderGlyphs::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        using namespace ice::render;
        RenderDevice& device = gfx_device.device();

        for (auto const& entry : _fonts)
        {
            device.destroy_resourcesets({ &entry.value.resource_set, 1 });
            device.destroy_image(entry.value.image);
        }

        device.destroy_buffer(_vertex_buffer);
        device.destroy_sampler(_sampler);
        device.destroy_shader(_shaders[0]);
        device.destroy_shader(_shaders[1]);
        device.destroy_shader(_shaders[2]);
        device.destroy_shader(_shaders[3]);
        device.destroy_pipeline(_pipeline);
        device.destroy_pipeline(_debug_pipeline);
        device.destroy_pipeline_layout(_pipeline_layout);
        device.destroy_resourcesets(_resource_sets);
        device.destroy_resourceset_layout(_resource_set_layouts[0]);
        device.destroy_resourceset_layout(_resource_set_layouts[1]);
    }

    void IceWorldTrait_RenderGlyphs::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::AssetStorage& storage = runner.asset_storage();

        _shader_data[0] = ice::sync_wait(ice::detail::load_font_shader(storage, u8"shaders/debug/font-vert"));
        _shader_data[1] = ice::sync_wait(ice::detail::load_font_shader(storage, u8"shaders/debug/font-frag"));
        _shader_data[2] = ice::sync_wait(ice::detail::load_font_shader(storage, u8"shaders/debug/font-debug-vert"));
        _shader_data[3] = ice::sync_wait(ice::detail::load_font_shader(storage, u8"shaders/debug/font-debug-frag"));
    }

    void IceWorldTrait_RenderGlyphs::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        for (auto const& entry : _fonts)
        {
            runner.asset_storage().release({ entry.value.asset });
        }
    }

    void IceWorldTrait_RenderGlyphs::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] RenderGlyphs :: update");

        ice::pod::Array<ice::Utf8String> load_fonts{ frame.allocator() };
        ice::pod::array::reserve(load_fonts, 10);

        ice::u32 const text_draws = ice::shards::count(frame.shards(), ice::Shard_DrawTextCommand);
        ice::Span<ice::TextRenderInfo> text_infos = frame.create_named_span<ice::TextRenderInfo>("ice.glyph-render.text-infos"_sid, text_draws);

        ice::u32 draw_vertices = 0;
        ice::shards::inspect_each<ice::DrawTextCommand const*>(
            frame.shards(),
            ice::Shard_DrawTextCommand,
            [&draw_vertices, &load_fonts, this](ice::DrawTextCommand const* payload) noexcept
            {
                ice::u64 const font_hash = ice::hash(payload->font);

                if (ice::pod::hash::has(_fonts, font_hash) == false)
                {
                    ice::pod::array::push_back(load_fonts, payload->font);
                }
                else
                {
                    // Is bigger than needed due to utf8 characters taking more than 1 byte.
                    draw_vertices += ice::string::size(payload->text) * 6;
                }
            }
        );

        ice::vec4f* vertice_array = frame.create_named_span<ice::vec4f>("ice.glyph-render.vertices"_sid, draw_vertices).data();

        ice::u32 cmd_idx = 0;
        ice::u32 vert_count = 0;

        ice::shards::inspect_each<ice::DrawTextCommand const*>(
            frame.shards(),
            ice::Shard_DrawTextCommand,
            [&, this](ice::DrawTextCommand const* payload) noexcept
            {
                ice::u64 const font_hash = ice::hash(payload->font);
                ice::TextRenderInfo& render_info = text_infos[cmd_idx++];
                render_info.vertice_count = 0;

                static FontEntry const dummy_entry{ .font = nullptr };
                FontEntry const& entry = ice::pod::hash::get(_fonts, font_hash, dummy_entry);

                if (entry.font != nullptr)
                {
                    render_info.resource_set = entry.resource_set;

                    build_glyph_vertices(
                        entry.font,
                        *payload,
                        vertice_array + vert_count,
                        render_info.vertice_count
                    );

                    vert_count += render_info.vertice_count;
                }
            }
        );

        for (ice::Utf8String const font_name : load_fonts)
        {
            portal.execute(load_font(runner, font_name));
        }
    }

    void IceWorldTrait_RenderGlyphs::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] RenderGlyphs :: record commands");

        using namespace ice::render;
        ice::Span<ice::TextRenderInfo const> commands = frame.named_span<ice::TextRenderInfo>("ice.glyph-render.text-infos"_sid);
        if (commands.empty())
        {
            return;
        }

        float scale[2];
        scale[0] = 2.0f / _framebuffer_size.x;
        scale[1] = 2.0f / _framebuffer_size.y;
        float translate[2];
        translate[0] = -1.0f;
        translate[1] = -1.0f;

        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { scale, sizeof(scale) }, 0);
        api.push_constant(cmds, _pipeline_layout, ShaderStageFlags::VertexStage, { translate, sizeof(translate) }, sizeof(scale));
        api.bind_resource_set(cmds, _pipeline_layout, _resource_sets[0], 0);
        api.bind_vertex_buffer(cmds, _vertex_buffer, 0);
        api.bind_pipeline(cmds, _pipeline);

        ice::render::ResourceSet current_set = ice::render::ResourceSet::Invalid;

        ice::u32 last_vert_count = 0;
        ice::u32 current_vert_count = 0;
        for (ice::TextRenderInfo const& text_info : commands)
        {
            if (text_info.vertice_count == 0)
            {
                continue;
            }

            if (current_set != text_info.resource_set)
            {
                if (current_vert_count > 0)
                {
                    api.draw(cmds, current_vert_count, 1, last_vert_count, 0);
                    last_vert_count += current_vert_count;
                }

                current_set = text_info.resource_set;
                current_vert_count = text_info.vertice_count;
                api.bind_resource_set(cmds, _pipeline_layout, current_set, 1);
            }
            else
            {
                current_vert_count += text_info.vertice_count;
            }
        }

        if (current_vert_count > 0)
        {
            api.draw(cmds, current_vert_count, 1, last_vert_count, 0);
        }
    }

    constexpr auto utf8_to_utf32(
        ice::c8utf const* it,
        ice::u32& count
    ) noexcept -> ice::u32
    {
        constexpr ice::u32 utf8_1byte_mask = 0b1000'0000;
        constexpr ice::u32 utf8_2byte_mask = 0b1110'0000;
        constexpr ice::u32 utf8_3byte_mask = 0b1111'0000;
        constexpr ice::u32 utf8_4byte_mask = 0b1111'1000;
        constexpr ice::u32 utf8_vbyte_mask = 0b0011'1111;
        constexpr ice::u32 utf8_1byte_count = 0;
        constexpr ice::u32 utf8_2byte_count = 0b1100'0000;
        constexpr ice::u32 utf8_3byte_count = 0b1110'0000;
        constexpr ice::u32 utf8_4byte_count = 0b1111'0000;

        if ((*it & utf8_1byte_mask) == utf8_1byte_count)
        {
            count = 1;
            return it[0];
        }
        if ((*it & utf8_2byte_mask) == utf8_2byte_count)
        {
            count = 2;
            ice::u32 codepoint = (it[0] & ~utf8_2byte_mask);
            codepoint <<= 6;
            codepoint |= (it[1] & utf8_vbyte_mask);
            return codepoint;
        }
        if ((*it & utf8_3byte_mask) == utf8_3byte_count)
        {
            count = 3;
            ice::u32 codepoint = (it[0] & ~utf8_3byte_mask);
            codepoint <<= 6;
            codepoint |= (it[1] & utf8_vbyte_mask);
            codepoint <<= 6;
            codepoint |= (it[2] & utf8_vbyte_mask);
            return codepoint;
        }
        if ((*it & utf8_4byte_mask) == utf8_4byte_count)
        {
            count = 4;
            ice::u32 codepoint = (it[0] & ~utf8_4byte_mask);
            codepoint <<= 6;
            codepoint |= (it[1] & utf8_vbyte_mask);
            codepoint <<= 6;
            codepoint |= (it[2] & utf8_vbyte_mask);
            codepoint <<= 6;
            codepoint |= (it[3] & utf8_vbyte_mask);
            return codepoint;
        }
        return 0;
    }

    void IceWorldTrait_RenderGlyphs::build_glyph_vertices(
        ice::gfx::GfxFont const* font,
        ice::DrawTextCommand const& draw_info,
        ice::vec4f* posuv_vertices,
        ice::u32& posuv_offset
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] RenderGlyphs :: build vertices");

        ice::c8utf const* it = draw_info.text.data();
        ice::c8utf const* const end = draw_info.text.data() + draw_info.text.size();

        ice::u32 font_size = draw_info.font_size;
        ice::vec2u position = draw_info.position;

        while (it < end)
        {
            ice::u32 advance = 0;
            ice::u32 codepoint = utf8_to_utf32(it, advance);

            it += advance;

            for (ice::gfx::GfxGlyph const& glyph : font->glyphs)
            {
                if (glyph.codepoint == codepoint)
                {
                    if (glyph.size.x > 0)
                    {
                        ice::vec2f const pos = {
                            position.x + (glyph.offset.x * font_size),
                            position.y - (glyph.offset.y * font_size)
                        };

                        ice::vec2f const size = {
                            glyph.size.x * font_size,
                            glyph.size.y * font_size
                        };

                        // TRI 1
                        posuv_vertices[posuv_offset + 0].x = pos.x;
                        posuv_vertices[posuv_offset + 0].y = pos.y;
                        posuv_vertices[posuv_offset + 0].z = glyph.atlas_x;
                        posuv_vertices[posuv_offset + 0].w = glyph.atlas_y;
                        posuv_vertices[posuv_offset + 1].x = pos.x;
                        posuv_vertices[posuv_offset + 1].y = pos.y + size.y;
                        posuv_vertices[posuv_offset + 1].z = glyph.atlas_x;
                        posuv_vertices[posuv_offset + 1].w = glyph.atlas_y + glyph.atlas_h;
                        posuv_vertices[posuv_offset + 2].x = pos.x + size.x;
                        posuv_vertices[posuv_offset + 2].y = pos.y + size.y;
                        posuv_vertices[posuv_offset + 2].z = glyph.atlas_x + glyph.atlas_w;
                        posuv_vertices[posuv_offset + 2].w = glyph.atlas_y + glyph.atlas_h;

                        // TRI 1
                        posuv_vertices[posuv_offset + 3].x = pos.x;
                        posuv_vertices[posuv_offset + 3].y = pos.y;
                        posuv_vertices[posuv_offset + 3].z = glyph.atlas_x;
                        posuv_vertices[posuv_offset + 3].w = glyph.atlas_y;
                        posuv_vertices[posuv_offset + 4].x = pos.x + size.x;
                        posuv_vertices[posuv_offset + 4].y = pos.y + size.y;
                        posuv_vertices[posuv_offset + 4].z = glyph.atlas_x + glyph.atlas_w;
                        posuv_vertices[posuv_offset + 4].w = glyph.atlas_y + glyph.atlas_h;
                        posuv_vertices[posuv_offset + 5].x = pos.x + size.x;
                        posuv_vertices[posuv_offset + 5].y = pos.y;
                        posuv_vertices[posuv_offset + 5].z = glyph.atlas_x + glyph.atlas_w;
                        posuv_vertices[posuv_offset + 5].w = glyph.atlas_y;

                        // Move by number of draw_vertices
                        posuv_offset += 6;
                    }

                    position.x += glyph.advance * draw_info.font_size;
                    break;
                }
            }
        }

        ICE_ASSERT(it == end, "Invalid utf8 string!");
    }

    auto IceWorldTrait_RenderGlyphs::load_font(
        ice::EngineRunner& runner,
        ice::Utf8String font_name
    ) noexcept -> ice::Task<>
    {
        ice::u64 const font_hash = ice::hash(font_name);

        if (ice::pod::hash::has(_fonts, font_hash))
        {
            co_return;
        }

        ice::pod::hash::set(
            _fonts,
            font_hash,
            FontEntry{ .font = nullptr }
        );

        co_await runner.thread_pool();

        ice::Asset const asset = co_await runner.asset_storage().request(ice::gfx::AssetType_Font, font_name, ice::AssetState::Loaded);

        // Early return if we failed.
        if (ice::asset_check(asset, AssetState::Loaded) == false)
        {
            co_return;
        }
        ice::gfx::GfxFont const* font = reinterpret_cast<ice::gfx::GfxFont const*>(asset.data.location);

        co_await runner.schedule_current_frame();

        ice::Data const font_atlas_data{
            .location = ice::memory::ptr_add(font->data_ptr, font->atlases[0].image_data_offset),
            .size = font->atlases[0].image_data_size,
            .alignment = 4
        };

        ice::render::Image image = ice::render::Image::Invalid;
        ice::render::ResourceSet resource_set = ice::render::ResourceSet::Invalid;
        co_await load_font_atlas(font->atlases[0], font_atlas_data, runner, image, resource_set);

        co_await runner.schedule_next_frame();

        ice::pod::hash::set(
            _fonts,
            font_hash,
            FontEntry{
                .asset = asset.handle,
                .image = image,
                .resource_set = resource_set,
                .font = font
            }
        );
        co_return;
    }

    auto IceWorldTrait_RenderGlyphs::load_font_atlas(
        ice::gfx::GfxFontAtlas const& atlas,
        ice::Data image_data,
        ice::EngineRunner& runner,
        ice::render::Image& out_image,
        ice::render::ResourceSet& out_set
    ) noexcept -> ice::Task<>
    {
        ice::render::ImageInfo const image_info{
            .type = ice::render::ImageType::Image2D,
            .format = ice::render::ImageFormat::UNORM_RGBA,
            .usage = ice::render::ImageUsageFlags::TransferDst | ice::render::ImageUsageFlags::Sampled,
            .width = atlas.image_size.x,
            .height = atlas.image_size.y,
            .data = nullptr,
        };

        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();

        ice::render::RenderDevice& device = gfx_device.device();

        co_await gfx_frame.frame_begin();

        ice::render::Buffer const data_buffer = device.create_buffer(
            ice::render::BufferType::Transfer,
            image_data.size
        );

        out_image = device.create_image(image_info, { });
        device.create_resourcesets({ _resource_set_layouts + 1, 1 }, { &out_set, 1 });

        ice::render::BufferUpdateInfo const updates[1]{
            ice::render::BufferUpdateInfo
            {
                .buffer = data_buffer,
                .data = image_data
            }
        };

        device.update_buffers({ updates, ice::size(updates) });

        struct : public ice::gfx::GfxFrameStage
        {
            void record_commands(
                ice::EngineFrame const& frame,
                ice::render::CommandBuffer cmds,
                ice::render::RenderCommands& api
            ) const noexcept override
            {
                using namespace ice::render;

                ImageBarrier barriers[4]{ };

                for (ice::u32 idx = 0; idx < 1; ++idx)
                {
                    barriers[idx].image = image;
                    barriers[idx].source_layout = ImageLayout::Undefined;
                    barriers[idx].destination_layout = ImageLayout::TransferDstOptimal;
                    barriers[idx].source_access = AccessFlags::None;
                    barriers[idx].destination_access = AccessFlags::TransferWrite;
                }

                api.pipeline_image_barrier(
                    cmds,
                    PipelineStage::TopOfPipe,
                    PipelineStage::Transfer,
                    { barriers, 1 }
                );

                for (ice::u32 idx = 0; idx < 1; ++idx)
                {
                    api.update_texture_v2(
                        cmds,
                        image,
                        image_data,
                        image_size
                    );
                }

                for (ice::u32 idx = 0; idx < 1; ++idx)
                {
                    barriers[idx].image = image;
                    barriers[idx].source_layout = ImageLayout::TransferDstOptimal;
                    barriers[idx].destination_layout = ImageLayout::ShaderReadOnly;
                    barriers[idx].source_access = AccessFlags::TransferWrite;
                    barriers[idx].destination_access = AccessFlags::ShaderRead;
                }

                api.pipeline_image_barrier(
                    cmds,
                    PipelineStage::Transfer,
                    PipelineStage::FramentShader,
                    { barriers, 1 }
                );
            }

            ice::render::Image image;
            ice::render::Buffer image_data;
            ice::vec2u image_size;
        } frame_stage;

        frame_stage.image = out_image;
        frame_stage.image_data = data_buffer;
        frame_stage.image_size = { image_info.width, image_info.height };

        // Await command recording stage
        //  Here we have access to a command buffer where we can record commands.
        //  These commands will be later executed on the graphics thread.
        co_await gfx_frame.frame_commands(&frame_stage);

        // Await end of graphics frame.
        //  Here we know that all commands have been executed
        //  and temporary objects can be destroyed.
        co_await gfx_frame.frame_end();

        using ice::render::ResourceType;
        using ice::render::ResourceUpdateInfo;
        using ice::render::ResourceSetUpdateInfo;

        ResourceUpdateInfo const update_resources[]
        {
            ResourceUpdateInfo
            {
                .image = out_image,
            },
        };

        ResourceSetUpdateInfo const update_sets[]
        {
            ResourceSetUpdateInfo
            {
                .resource_set = out_set,
                .resource_type = ResourceType::SampledImage,
                .binding_index = 0,
                .array_element = 0,
                .resources = update_resources,
            }
        };

        device.update_resourceset(update_sets);

        device.destroy_buffer(data_buffer);
        co_return;
    }

    void register_trait_render_glyphs(ice::WorldTraitArchive& archive) noexcept
    {
        archive.register_trait(
            ice::Constant_TraitName_RenderGlyphs,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderGlyphs>
            }
        );
    }

} // namespace ice
