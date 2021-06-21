#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/data_storage.hxx>
#include <ice/input/input_types.hxx>

namespace ice
{

    template<typename T>
    class Task;

    struct EngineRequest;

    class EngineFrame : public ice::DataStorage
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto allocator() noexcept -> ice::Allocator& = 0;
        virtual auto memory_consumption() noexcept -> ice::u32 = 0;

        virtual auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> = 0;

        virtual void execute_task(ice::Task<void> task) noexcept = 0;

        virtual void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept = 0;
    };

} // namespace ice
