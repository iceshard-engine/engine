/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_types.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/task.hxx>
#include <ice/task_expected.hxx>

namespace ice::gfx
{

    auto load_shader_program(
        ice::String name,
        ice::AssetStorage& assets
    ) noexcept -> ice::TaskExpected<ice::render::PipelineProgramInfo>;

    //auto await_shader_program_on(
    //    ice::String name,
    //    ice::AssetStorage& assets,
    //    ice::TaskScheduler& scheduler
    //) noexcept -> ice::TaskExpected<ice::render::PipelineProgramInfo>;

} // namespace ice::gfx
