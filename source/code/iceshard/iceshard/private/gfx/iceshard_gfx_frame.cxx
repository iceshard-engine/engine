#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/pod/hash.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/assert.hxx>

#include "../iceshard_task_executor.hxx"

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc, "gfx-frame" }
        , _enqueued_passes{ _allocator }
        , _frame_tasks{ _allocator }
        , _queue_group{ nullptr }
        , _task_executor{ _allocator, ice::Vector<ice::Task<void>> { _allocator } }
    {
        _frame_tasks.reserve(1024);
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
        {
            GfxFrame::GfxAwaitCommandsOperation* expected_initial_task = _head.load(std::memory_order_acquire);
            if (expected_initial_task != nullptr)
            {
                while (
                    _head.compare_exchange_weak(
                        expected_initial_task,
                        nullptr,
                        std::memory_order::acquire,
                        std::memory_order::relaxed
                    ) == false
                    )
                {
                    continue;
                }

                while (expected_initial_task != nullptr)
                {
                    GfxFrame::GfxAwaitCommandsOperation* next_op = expected_initial_task->_next;
                    expected_initial_task->_coro.destroy();
                    expected_initial_task = next_op;
                }
            }
        }
        {
            GfxFrame::GfxAwaitFrameEnd* expected_initial_task = _frame_end_head.load(std::memory_order_acquire);
            if (expected_initial_task != nullptr)
            {
                while (
                    _frame_end_head.compare_exchange_weak(
                        expected_initial_task,
                        nullptr,
                        std::memory_order::acquire,
                        std::memory_order::relaxed
                    ) == false
                    )
                {
                    continue;
                }

                while (expected_initial_task != nullptr)
                {
                    GfxFrame::GfxAwaitFrameEnd* next_op = expected_initial_task->_next;
                    expected_initial_task->_coro.destroy();
                    expected_initial_task = next_op;
                }
            }
        }
    }

    auto IceGfxFrame::aquire_task_commands(ice::StringID_Arg& queue_name) noexcept -> GfxFrame::GfxAwaitCommandsOperation
    {
        return GfxAwaitCommandsOperation{ *this, queue_name };
    }

    auto IceGfxFrame::frame_end() noexcept -> GfxAwaitFrameEnd
    {
        return GfxAwaitFrameEnd{ *this };
    }

    void IceGfxFrame::execute_task(ice::Task<void> task) noexcept
    {
        _frame_tasks.push_back(ice::move(task));
    }

    void IceGfxFrame::start_all() noexcept
    {
        _task_executor = IceshardTaskExecutor{ _allocator, ice::move(_frame_tasks) };
        _task_executor.start_all();
    }

    void IceGfxFrame::wait_ready() noexcept
    {
        GfxFrame::GfxAwaitFrameEnd* expected_initial_task = _frame_end_head.load(std::memory_order_acquire);
        if (expected_initial_task != nullptr)
        {
            while (
                _frame_end_head.compare_exchange_weak(
                    expected_initial_task,
                    nullptr,
                    std::memory_order::acquire,
                    std::memory_order::relaxed
                ) == false
            )
            {
                continue;
            }

            while (expected_initial_task != nullptr)
            {
                auto* next_op = expected_initial_task->_next;
                expected_initial_task->_coro.resume();
                expected_initial_task = next_op;
            }
        }

        _task_executor.wait_ready();
    }

    void IceGfxFrame::enqueue_pass(
        ice::StringID_Arg queue_name,
        ice::gfx::GfxPass* pass
    ) noexcept
    {
        ice::pod::multi_hash::insert(
            _enqueued_passes,
            ice::hash(queue_name),
            pass
        );
    }

    void IceGfxFrame::execute_passes(
        ice::EngineFrame const& frame,
        ice::gfx::IceGfxQueueGroup& queue_group
    ) noexcept
    {
        _queue_group = &queue_group;

        GfxFrame::GfxAwaitCommandsOperation* expected_initial_task = _head.load(std::memory_order_acquire);
        if (expected_initial_task != nullptr)
        {
            while (
                _head.compare_exchange_weak(
                    expected_initial_task,
                    nullptr,
                    std::memory_order::acquire,
                    std::memory_order::relaxed
                ) == false
                )
            {
                continue;
            }

            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue("default"_sid);
            if (queue != nullptr)
            {
                queue->test_begin();
                while (expected_initial_task != nullptr)
                {
                    auto* next_op = expected_initial_task->_next;
                    expected_initial_task->_coro.resume();
                    //ICE_ASSERT(
                    //    expected_initial_task->_coro.done(),
                    //    "Coroutine not finished!"
                    //);

                    //expected_initial_task->_coro.destroy();
                    expected_initial_task = next_op;
                }
                queue->test_end();
            }
        }

        for (auto const& entry : _enqueued_passes)
        {
            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue(ice::StringID{ StringID_Hash{ entry.key } });
            if (queue != nullptr)
            {
                queue->execute_pass(frame, entry.value);
            }
        }

        ice::pod::hash::clear(_enqueued_passes);
    }

    auto IceGfxFrame::task_commands(
        ice::StringID_Arg queue_name
    ) noexcept -> ice::gfx::GfxTaskCommands&
    {
        return *_queue_group->get_queue(queue_name);
    }

} // namespace ice::gfx
