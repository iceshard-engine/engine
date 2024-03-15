/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_stage.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/stringid.hxx>
#include <ice/span.hxx>

namespace ice::gfx
{

    struct GfxStageRegistry
    {
        virtual ~GfxStageRegistry() noexcept = default;

        virtual void register_stage(
            ice::StringID_Arg key,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;

        virtual bool query_stages(
            ice::Span<ice::StringID> stage_keys,
            ice::Array<ice::gfx::GfxStage*>& out_stages
        ) const noexcept = 0;
    };

    auto create_stage_registry(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxStageRegistry>;

} // namespace ice::gfx
