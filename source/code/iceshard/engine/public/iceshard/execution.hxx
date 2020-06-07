#pragma once
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>

namespace iceshard
{

    class Frame;

    class ExecutionInstance
    {
    public:
        ExecutionInstance() noexcept = default;
        ExecutionInstance(ExecutionInstance&&) noexcept = delete;
        ExecutionInstance(ExecutionInstance const&) noexcept = delete;

        auto operator=(ExecutionInstance&&) noexcept -> ExecutionInstance& = delete;
        auto operator=(ExecutionInstance const&) noexcept -> ExecutionInstance& = delete;

    public:
        virtual ~ExecutionInstance() noexcept = default;

        virtual auto previous_frame() const noexcept -> Frame const& = 0;

        virtual auto current_frame() noexcept -> Frame& = 0;

        virtual void next_frame() noexcept = 0;
    };

} // namespace iceshard
