#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>

namespace iceshard
{

    void Engine::create_task(std::function<cppcoro::task<>(core::allocator&)> task) noexcept
    {
        add_task(task(current_frame().frame_allocator()));
    }

} // namespace iceshard