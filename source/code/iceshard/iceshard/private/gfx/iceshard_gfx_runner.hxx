#pragma once
#include <ice/gfx/gfx_runner.hxx>
#include <ice/task_thread.hxx>

namespace ice::gfx
{

    class IceGfxRunner : public ice::gfx::GfxRunner
    {
    public:
        IceGfxRunner(ice::Allocator& alloc) noexcept;
        ~IceGfxRunner() noexcept override;

        void push_task(ice::Task<void> task) noexcept override;

    private:
        ice::UniquePtr<ice::TaskThread> _thread;
    };

} // namespace ice::gfx
