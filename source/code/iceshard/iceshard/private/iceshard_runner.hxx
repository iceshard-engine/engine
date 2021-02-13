#pragma once
#include <ice/engine_runner.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class IceshardMemoryFrame;

    class IceshardEngineRunner final : public ice::EngineRunner
    {
    public:
        IceshardEngineRunner(ice::Allocator& alloc) noexcept;
        ~IceshardEngineRunner() noexcept override;

        auto clock() const noexcept -> ice::Clock const& override;

        auto previous_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() noexcept -> EngineFrame& override;
        void next_frame() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::SystemClock _clock;

        ice::memory::ProxyAllocator _frame_allocator;
        ice::memory::ScratchAllocator _frame_data_allocator[2];
        ice::u32 _next_free_allocator = 0;

        ice::UniquePtr<ice::IceshardMemoryFrame> _previous_frame;
        ice::UniquePtr<ice::IceshardMemoryFrame> _current_frame;
    };

} // namespace ice
