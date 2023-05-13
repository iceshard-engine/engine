/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

//#include "trait_render_resource.hxx"
//
//#include <ice/engine_runner.hxx>
//#include <ice/world/world_portal.hxx>
//
//#include <ice/gfx/gfx_device.hxx>
//#include <ice/gfx/gfx_resource_tracker.hxx>
//#include <ice/gfx/gfx_frame.hxx>
//
//#include <ice/render/render_command_buffer.hxx>
//#include <ice/render/render_resource.hxx>
//#include <ice/render/render_swapchain.hxx>
//
//namespace ice
//{
//
//    IceWorldTrait_RenderResource::IceWorldTrait_RenderResource(ice::Allocator& alloc) noexcept
//        : _allocator{ alloc }
//        , _resources{ _allocator }
//    {
//    }
//
//    auto IceWorldTrait_RenderResource::gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const>
//    {
//        static ice::StringID const dependencies[]{
//            "frame.clear"_sid,
//        };
//        static ice::gfx::GfxStageInfo const infos[]{
//            ice::gfx::GfxStageInfo
//            {
//                .name = "frame.render-sprites"_sid,
//                .dependencies = dependencies,
//                .type = ice::gfx::GfxStageType::DrawStage
//            }
//        };
//        return infos;
//    }
//
//    auto IceWorldTrait_RenderResource::gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const>
//    {
//        static ice::gfx::GfxStageSlot const slots[]{
//            ice::gfx::GfxStageSlot
//            {
//                .name = "frame.render-sprites"_sid,
//                .stage = this
//            }
//        };
//        return slots;
//    }
//
//    void IceWorldTrait_RenderResource::define_resource(
//        ice::TraitRenderResource const& resource_definition
//    ) noexcept
//    {
//        ice::pod::Array<ice::render::ResourceSetLayoutBinding> resource_layout_bindings{ _allocator };
//
//        for (ice::StringID_Arg uniform_buffer_name : resource_definition.uniform_buffers)
//        {
//
//        }
//
//        ice::render::ResourceSetLayoutBinding{
//            .binding_index = 0,
//
//        };
//
//        IceTraitRenderResource render_resource{
//            .name = resource_definition.name,
//        };
//    }
//
//    void IceWorldTrait_RenderResource::on_update(
//        ice::EngineFrame& frame,
//        ice::EngineRunner& runner,
//        ice::WorldPortal& portal
//    ) noexcept
//    {
//        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
//        gfx_frame.set_stage_slots(gfx_stage_slots());
//    }
//
//    void IceWorldTrait_RenderResource::record_commands(
//        ice::EngineFrame const& frame,
//        ice::render::CommandBuffer cmds,
//        ice::render::RenderCommands& api
//    ) const noexcept
//    {
//        api.end_renderpass(cmds);
//    }
//
//} // namespace ice
