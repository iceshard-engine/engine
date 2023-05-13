/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_context.hxx"

#include <ice/gfx/gfx_pass.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxContext::IceGfxContext(
        ice::Allocator& alloc,
        ice::gfx::GfxPass const& gfx_pass
    ) noexcept
        : _alloc{ alloc }
        , _cached_stages{ alloc }
    {
        ice::array::resize(_cached_stages, gfx_pass.stage_count());
        for (IceGfxContextStage& stage_entry : _cached_stages)
        {
            stage_entry = { };
        }
    }

    void IceGfxContext::prepare_context(
        ice::Span<ice::gfx::GfxContextStage const*> stages,
        ice::gfx::GfxDevice& device
    ) noexcept
    {
        ice::u32 const stage_count = ice::count(stages);
        ICE_ASSERT(
            stage_count == ice::array::count(_cached_stages),
            "Currently a GfxContext can only updated with the exact same number of stages it was created for."
        );

        for (ice::u32 idx = 0; idx < stage_count; ++idx)
        {
            IceGfxContextStage& stage_entry = _cached_stages[idx];

            if (stage_entry.stage == nullptr)
            {
                stages[idx]->prepare_context(*this, device);
                stage_entry.stage = stages[idx];
            }
            else if (_cached_stages[idx].stage != stages[idx])
            {
                stage_entry.stage->clear_context(*this, device);
                stages[idx]->prepare_context(*this, device);
                stage_entry.stage = stages[idx];
            }
        }
    }

    void IceGfxContext::clear_context(
        ice::gfx::GfxDevice& device
    ) noexcept
    {
        for (IceGfxContextStage& stage_entry : _cached_stages)
        {
            if (stage_entry.stage)
            {
                stage_entry.stage->clear_context(*this, device);
            }
        }
    }

    void IceGfxContext::record_commands(
        ice::EngineFrame const& engine_frame,
        ice::render::CommandBuffer command_buffer,
        ice::render::RenderCommands& command_api
    ) noexcept
    {
        command_api.begin(command_buffer);
        for (IceGfxContextStage const& cached_stage : _cached_stages)
        {
            cached_stage.stage->record_commands(*this, engine_frame, command_buffer, command_api);
        }
        command_api.end(command_buffer);
    }

} // namespace ice::gfx
