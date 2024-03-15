/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxPass;

    class GfxQueue
    {
    public:
        virtual ~GfxQueue() noexcept = default;

        virtual void reset() noexcept = 0;

        virtual void request_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> out_buffers
        ) noexcept = 0;

        virtual void submit_command_buffers(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence const* fence
        ) noexcept = 0;

        virtual bool presenting() const noexcept = 0;

        virtual void present(ice::render::RenderSwapchain* swapchain) const noexcept = 0;
    };

    namespace v2
    {

        struct GfxQueueGroup_Temp
        {
            virtual ~GfxQueueGroup_Temp() noexcept = default;

            virtual bool get_queue(
                ice::render::QueueFlags flags,
                ice::gfx::GfxQueue*& out_queue
            ) noexcept = 0;
        };

    } // namespace v2

} // namespace ice::gfx
