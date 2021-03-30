#pragma once
#include <ice/pod/array.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class IceGfxPass final : public GfxPass
    {
    public:
        IceGfxPass(
            ice::Allocator& alloc
        ) noexcept;
        ~IceGfxPass() noexcept;

        bool has_work() const noexcept;

        void add_stage(
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void record_commands(
            ice::render::CommandBuffer cmd_buffer,
            ice::render::RenderCommands& cmds
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Array<ice::gfx::GfxStage*> _stages;
    };

} // namespace ice::gfx
