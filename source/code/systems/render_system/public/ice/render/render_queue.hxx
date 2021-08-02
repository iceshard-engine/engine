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
        Invalid = 0x0,
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

        virtual void reset_pool(
            ice::u32 pool_index
        ) noexcept = 0;

        virtual void submit(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence const* fence
        ) noexcept = 0;

        virtual void present(
            ice::render::RenderSwapchain* swapchain
        ) noexcept = 0;

    protected:
        virtual ~RenderQueue() noexcept = default;
    };

    constexpr auto operator|(QueueFlags left, QueueFlags right) noexcept -> QueueFlags
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<QueueFlags>(left_val | right_val);
    }

    constexpr auto operator&(QueueFlags left, QueueFlags right) noexcept -> QueueFlags
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<QueueFlags>(left_val & right_val);
    }

} // namespace ice
