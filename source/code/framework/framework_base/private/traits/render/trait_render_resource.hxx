/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/pod/hash.hxx>

namespace ice
{

    struct IceTraitRenderResource
    {
        ice::StringID name;

        ice::render::PipelineLayout pipeline_Layout;
        ice::render::ResourceSetLayout resource_set;
        ice::render::ResourceSetLayout resource_set_layout;

        ice::Span<ice::render::Buffer> uniform_buffers;
        ice::Span<ice::render::Sampler> image_samplers;
        ice::Span<ice::render::Image> images;
    };

    //class IceWorldTrait_RenderResource : public ice::GameWorldTrait_RenderResource, public ice::gfx::GfxStage
    //{
    //public:
    //    IceWorldTrait_RenderResource(ice::Allocator& alloc) noexcept;

    //    auto gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const> override;
    //    auto gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const> override;

    //    void define_resource(
    //        ice::TraitRenderResource const& resource_definition
    //    ) noexcept override;

    //    void on_update(
    //        ice::EngineFrame& frame,
    //        ice::EngineRunner& runner,
    //        ice::WorldPortal& portal
    //    ) noexcept override;

    //    void record_commands(
    //        ice::EngineFrame const& frame,
    //        ice::render::CommandBuffer cmds,
    //        ice::render::RenderCommands& api
    //    ) const noexcept override;

    //private:
    //    ice::Allocator& _allocator;
    //    ice::pod::Hash<ice::IceTraitRenderResource> _resources;
    //};

} // namespace ice
