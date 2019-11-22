#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

#include <cppcoro/sync_wait.hpp>
#include <thread>

namespace iceshard
{

    void Engine::create_task(std::function<cppcoro::task<>(core::allocator&)>&& task) noexcept
    {
        add_task(task(current_frame().frame_allocator()));
    }

    void Engine::create_task(std::function<cppcoro::task<>(iceshard::Frame&)>&& task) noexcept
    {
        add_task(task(current_frame()));
    }

    void Engine::create_task(std::function<cppcoro::task<>(iceshard::Engine&)>&& task) noexcept
    {
        add_task(task(*this));
    }

    void Engine::create_long_task(std::function<cppcoro::task<>(iceshard::Engine&)>&& task) noexcept
    {
        add_task([](cppcoro::task<> task_object) noexcept -> cppcoro::task<> {

            auto thread_fn = [](cppcoro::task<> t) noexcept  {
                auto sync_task_beg = std::chrono::high_resolution_clock::now();
                //fmt::print("{}\n", c);
                //co_await t;
                cppcoro::sync_wait(t);
                auto sunc_task_end = std::chrono::high_resolution_clock::now();
                fmt::print("Long task took: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(sunc_task_end - sync_task_beg).count());
            };

            std::thread th{ std::move(thread_fn), std::move(task_object) };
            th.detach();
            //fmt::print("Awaiting task to finish!\n");
            //co_await task_object;
            co_return;

        }(task(*this)));
    }

} // namespace iceshard
