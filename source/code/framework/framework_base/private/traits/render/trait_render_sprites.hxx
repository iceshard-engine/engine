#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    namespace detail
    {

        struct SpriteInstanceInfo;
        struct SpriteInstance;

        struct RenderData_Sprite
        {
            ice::u32 shape_offset;
            ice::u32 shape_vertices;
            ice::vec2f material_scale;
            ice::render::Image material[1];
            ice::render::Buffer material_tileinfo[1];
            ice::render::ResourceSet sprite_resource[1];
        };

    } // namespace detail


    class AssetSystem;

    class IceWorldTrait_RenderSprites : public ice::GameWorldTrait_RenderDraw, public ice::gfx::GfxStage
    {
    public:
        IceWorldTrait_RenderSprites(
            ice::Allocator& alloc
        ) noexcept;

        auto gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const> override;
        auto gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const> override;

        void set_camera(
            ice::StringID_Arg camera_name
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
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) const noexcept override;

    protected:
        auto task_create_render_objects(
            ice::EngineRunner& runner,
            ice::AssetSystem& asset_system,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_destroy_render_objects(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_resource_camera(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_resource_data(
            ice::gfx::GfxDevice& gfx_device,
            ice::Span<detail::SpriteInstance> instances
        ) noexcept -> ice::Task<>;

        auto task_load_resource_material(
            ice::StringID_Arg material_name,
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_resource_material(
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device,
            ice::StringID material_name,
            detail::RenderData_Sprite sprite_data
        ) noexcept -> ice::Task<>;

        auto task_destroy_resource_material(
            ice::gfx::GfxDevice& gfx_device,
            ice::detail::RenderData_Sprite sprite_data
        ) noexcept -> ice::Task<>;

    private:
        ice::AssetSystem* _asset_system = nullptr;
        ice::pod::Hash<ice::detail::RenderData_Sprite> _sprite_materials;

        ice::render::ResourceSetLayout _resource_set_layouts[2]{ };
        ice::render::ResourceSet _resource_sets[1];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
        ice::render::Sampler _sampler;
        ice::render::Image _textures[2];

        ice::render::Buffer _vertex_buffer;
        ice::pod::Hash<ice::u32> _vertex_offsets;

        ice::render::Buffer _instance_buffer;

        ice::StringID _render_camera;
        ice::render::Buffer _render_camera_buffer{ };
    };

} // namespace ice
