/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_command_buffer.hxx>
#include <ice/span.hxx>

namespace ice::render
{

    enum class QueueID : ice::u32
    {
        Invalid = 0xffff'ffff
    };

    enum class QueueFlags : ice::u32
    {
        None = 0x0,
        Graphics = 0x1,
        Compute = 0x2,
        Transfer = 0x4,
        //! This flags is only present after a render surface was created.
        Present = 0x8,
    };

    struct QueueFamilyInfo
    {
        ice::render::QueueID id;
        ice::render::QueueFlags flags;
        ice::u32 count;
    };

    static_assert(ice::TrivialContainerLogicAllowed<QueueFamilyInfo>);

    struct QueueInfo
    {
        ice::render::QueueID id;
        ice::u32 count;
    };

    class RenderQueue
    {
    public:
        virtual void allocate_buffers(
            ice::u32 pool_index,
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept = 0;

        virtual void release_buffers(
            ice::u32 pool_index,
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept = 0;

        virtual void reset_pool(
            ice::u32 pool_index
        ) noexcept = 0;

        virtual void submit(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence* fence
        ) noexcept = 0;

        virtual void present(
            ice::render::RenderSwapchain* swapchain
        ) noexcept = 0;

    protected:
        virtual ~RenderQueue() noexcept = default;
    };

} // namespace ice
