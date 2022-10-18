#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/mem_data.hxx>

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

    class AssetStorage;

    class IceWorldTrait_RenderSprites : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderSprites(
            ice::Allocator& alloc
        ) noexcept;

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

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    protected:
        void update_resource_camera(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept;

        void update_resource_data(
            ice::gfx::GfxDevice& gfx_device,
            ice::Span<detail::SpriteInstance> instances
        ) noexcept;

        void destroy_resource_material(
            ice::gfx::GfxDevice& gfx_device,
            ice::detail::RenderData_Sprite const& sprite_data
        ) noexcept;

        auto task_load_resource_material(
            ice::String material_name,
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_resource_material(
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device,
            ice::StringID material_name,
            detail::RenderData_Sprite sprite_data
        ) noexcept -> ice::Task<>;

    private:
        ice::AssetStorage* _asset_system = nullptr;
        ice::HashMap<ice::detail::RenderData_Sprite> _sprite_materials;

        ice::render::ResourceSetLayout _resource_set_layouts[2]{ };
        ice::render::ResourceSet _resource_sets[1];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
        ice::Data _shader_data[2];

        ice::render::Sampler _sampler;
        ice::render::Image _textures[2];

        ice::render::Buffer _vertex_buffer;
        ice::HashMap<ice::u32> _vertex_offsets;

        ice::render::Buffer _instance_buffer;

        ice::StringID _render_camera;
        ice::render::Buffer _render_camera_buffer{ };
    };


    class WorldTraitArchive;

    void register_trait_render_sprites(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
