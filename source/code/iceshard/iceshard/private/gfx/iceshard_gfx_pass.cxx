#include "iceshard_gfx_pass.hxx"
#include <ice/gfx/gfx_stage.hxx>

namespace ice::gfx
{

    IceGfxPass::IceGfxPass(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _stages{ _allocator }
    {
        ice::pod::array::reserve(_stages, 10);
    }

    IceGfxPass::~IceGfxPass() noexcept
    {
    }

    bool IceGfxPass::has_work() const noexcept
    {
        return ice::pod::array::empty(_stages) == false;
    }

    void IceGfxPass::add_stage(
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        ice::pod::array::push_back(
            _stages,
            stage
        );
    }

    void IceGfxPass::record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept
    {
        for (ice::gfx::GfxStage* stage : _stages)
        {
            stage->record_commands(
                cmd_buffer,
                cmds
            );
        }

        ice::pod::array::clear(_stages);
    }

} // namespace ice::gfx
