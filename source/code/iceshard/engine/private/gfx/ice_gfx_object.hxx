/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_object.hxx>

namespace ice::gfx::v2
{

    struct GfxObjectHandleBase
    {
        ice::render::Renderpass renderpass;
        ice::render::Framebuffer framebuffer;
        ice::render::Pipeline pipeline;
        ice::render::PipelineLayout pipeline_layout;
        ice::render::ResourceSet resourceset;
        ice::render::ResourceSetLayout resourceset_layout;
        ice::render::Shader shader;
        ice::render::Image image;
        ice::render::Buffer buffer;
    };

} // namespace ice::gfx::v2
