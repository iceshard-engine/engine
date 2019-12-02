#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

#include <cppcoro/sync_wait.hpp>
#include <thread>

namespace iceshard
{

    void Engine::add_long_task(cppcoro::task<> task) noexcept
    {
        add_task([](cppcoro::task<> task_object) noexcept -> cppcoro::task<> {

            std::thread th{ cppcoro::sync_wait<cppcoro::task<>>, std::move(task_object) };
            th.detach();
            co_return;

        }(std::move(task)));
    }

} // namespace iceshard
