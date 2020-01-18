#pragma
#include <cppcoro/task.hpp>
#include <vector>

namespace vs_hacks
{

    void cppcoro_sync_all_workaround(std::vector<cppcoro::task<>> task_list) noexcept;

} // namespace vs_hacks
