#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/data_storage.hxx>
#include <ice/input/input_types.hxx>

#include <ice/engine_task_operations.hxx>

namespace ice
{

    template<typename T>
    class Task;

    class EngineFrame;
    struct EngineRequest;

    struct FrameEndOperationData : ice::EngineTaskOperationBaseData { };

    using FrameEndOperation = ice::EngineTaskOperation<ice::EngineFrame, ice::FrameEndOperationData>;

    class EngineFrame : public ice::DataStorage
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto index() const noexcept -> ice::u32 = 0;

        virtual auto allocator() noexcept -> ice::Allocator& = 0;
        virtual auto memory_consumption() noexcept -> ice::u32 = 0;

        virtual auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> = 0;;

        virtual void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept = 0;

        virtual auto schedule_frame_end() noexcept -> ice::FrameEndOperation = 0;

    protected:
        friend FrameEndOperation;

        virtual void schedule_internal(
            ice::FrameEndOperationData& operation
        ) noexcept = 0;
    };

} // namespace ice
