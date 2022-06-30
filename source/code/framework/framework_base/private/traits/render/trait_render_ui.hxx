#pragma once
#include <ice/render/render_declarations.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/pod/hash.hxx>
#include <ice/ui_data.hxx>
#include <ice/shard.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_RenderUIData = "action/render/ui`ice::RenderUIRequest const*"_shard;

    struct RenderUIRequest
    {
        ice::u64 id;
        ice::vec2f position;
        ice::ui::UIData const* data;
        ice::ui::Element const* data_layouts;
    };

    struct RenderUIData
    {
        ice::u64 id;
        ice::ui::UIData const* uidata;

        struct Uniform
        {
            ice::vec2f position;
            ice::vec2f scale;
        };

        Uniform uniform;

        ice::render::Buffer buffer_uniform;
        ice::render::Buffer buffer_vertices;
        ice::render::Buffer buffer_colors;

        ice::render::ResourceSet resourceset_uniform;

        ice::Span<ice::vec2f> vertices;
        ice::Span<ice::vec4f> colors;
        ice::Span<ice::vec2f> uvs;

        bool is_ready;

        ice::ui::Element const* element_layouts;
    };

    struct RenderUICommand
    {
        ice::RenderUIData const* render_data;
        ice::u32 vertice_count;
    };

    class IceWorldTrait_RenderUI final : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderUI(
            ice::Allocator& alloc
        ) noexcept;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
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

    private:
        auto create_render_data(
            ice::Allocator& alloc,
            ice::EngineRunner& runner,
            ice::RenderUIRequest const& ui_request,
            ice::RenderUIData& render_data
        ) noexcept -> ice::Task<>;

    private:
        ice::Data _shader_data[2];
        ice::render::Shader _shaders[2];
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::ResourceSetLayout _resource_set_layout[1];
        //ice::render::ResourceSet _resource_set[1];

        ice::vec2f _display_size;
        ice::pod::Hash<RenderUIData*> _render_data;
    };

    void register_trait_render_ui(ice::WorldTraitArchive& archive) noexcept;

} // namespace ice

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::RenderUIRequest const*> = ice::payload_id("ice::RenderUIRequest const*");
