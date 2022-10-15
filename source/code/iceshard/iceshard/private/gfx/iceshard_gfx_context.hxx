#pragma once
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/container_types.hxx>

namespace ice::gfx
{

    class GfxPass;
    class GfxTrait;
    class GfxDevice;

    struct IceGfxContextStage
    {
        ice::gfx::GfxContextStage const* stage;
    };

    class IceGfxContext : public ice::gfx::GfxContext
    {
    public:
        IceGfxContext(
            ice::Allocator& alloc,
            ice::gfx::GfxPass const& gfx_pass
        ) noexcept;

        ~IceGfxContext() noexcept override = default;

        void prepare_context(
            ice::Span<ice::gfx::GfxContextStage const*> stages,
            ice::gfx::GfxDevice& device
        ) noexcept;

        void clear_context(
            ice::gfx::GfxDevice& device
        ) noexcept;

        void record_commands(
            ice::EngineFrame const& engine_frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& command_api
        ) noexcept;

    private:
        //ice::gfx::GfxPass const& _gfx_pass;
        ice::Array<ice::gfx::IceGfxContextStage> _cached_stages;
    };

} // namespace ice::gfx
