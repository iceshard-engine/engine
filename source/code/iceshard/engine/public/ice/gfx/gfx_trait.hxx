#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxDevice;
    class GfxContext;
    class GfxFrame;

    class GfxRecordStage
    {
    public:
        virtual ~GfxRecordStage() noexcept = default;

        virtual void gfx_record(
            ice::EngineFrame const& engine_frame,
            ice::render::RenderCommands& render_commands,
            ice::render::CommandBuffer& command_buffer
        ) noexcept = 0;
    };

    class GfxTrait : public ice::WorldTrait
    {
    public:
        virtual auto gfx_render_stages() noexcept -> ice::Span<ice::StringID const> { return {}; }

        virtual void gfx_context_setup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept { }

        virtual void gfx_context_cleanup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept { }

        virtual void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context,
            ice::gfx::GfxFrame& frame
        ) noexcept { }
    };

} // namespace ice::gfx
