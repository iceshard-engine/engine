#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    enum class PipelineStage : ice::u32
    {
        ColorAttachmentOutput,
        FramentShader,
    };

} // namespace ice::render
