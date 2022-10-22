/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/gfx/gfx_frame.hxx>

namespace ice::gfx
{

    auto GfxFrame::frame_begin() noexcept -> ice::gfx::GfxAwaitBeginFrame
    {
        return ice::gfx::GfxAwaitBeginFrame{ *this };
    }

    auto GfxFrame::frame_commands(ice::gfx::GfxFrameStage const* stage) noexcept -> ice::gfx::GfxAwaitExecuteStage
    {
        return ice::gfx::GfxAwaitExecuteStage{ *this, {.stage = stage } };
    }

    auto GfxFrame::frame_end() noexcept -> ice::gfx::GfxAwaitEndFrame
    {
        return ice::gfx::GfxAwaitEndFrame{ *this };
    }

} // namespace ice::gfx
