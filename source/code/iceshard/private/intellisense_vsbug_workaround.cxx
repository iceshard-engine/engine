#include "intellisense_vsbug_workaround.hxx"
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/when_all_ready.hpp>

void vs_hacks::cppcoro_sync_all_workaround(std::vector<cppcoro::task<>> task_list) noexcept
{
    cppcoro::sync_wait(cppcoro::when_all_ready(std::move(task_list)));
}
