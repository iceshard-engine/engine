#pragma once
#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_font.hxx>

namespace ice
{

    struct TextRenderInfo
    {
        ice::u32 vertice_count;
        ice::render::ResourceSet resource_set;
    };

    class IceWorldTrait_RenderGlyphs : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderGlyphs(ice::Allocator& alloc) noexcept;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    private:
        void build_glyph_vertices(
            ice::gfx::GfxFont const* font,
            ice::DrawTextCommand const& draw_info,
            ice::vec4f* posuv_vertices,
            ice::u32& posuv_offset
        ) noexcept;

        auto load_font(
            ice::EngineRunner& runner,
            ice::Utf8String font_name
        ) noexcept -> ice::Task<>;

        auto load_font_atlas(
            ice::gfx::GfxFontAtlas const& atlas,
            ice::Data image_data,
            ice::EngineRunner& runner,
            ice::render::Image& out_image,
            ice::render::ResourceSet& out_set
        ) noexcept -> ice::Task<>;

    private:
        ice::vec2f _framebuffer_size;

        struct FontEntry
        {
            ice::AssetHandle* asset;
            ice::render::Image image;
            ice::render::ResourceSet resource_set;
            ice::gfx::GfxFont const* font;
        };

        ice::pod::Hash<FontEntry> _fonts;

        ice::render::Buffer _vertex_buffer;

        ice::Data _shader_data[4];
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[4];
        ice::render::Sampler _sampler;
        ice::render::ResourceSetLayout _resource_set_layouts[2];
        ice::render::ResourceSet _resource_sets[1];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;
        ice::render::Pipeline _debug_pipeline;
    };


    class WorldTraitArchive;

    void register_trait_render_glyphs(
        ice::WorldTraitArchive& archive
    ) noexcept;


} // namespace ice
