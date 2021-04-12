#pragma once
#include <ice/engine_frame.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    class IceshardMemoryFrame final : public ice::EngineFrame
    {
    public:
        IceshardMemoryFrame(
            ice::memory::ScratchAllocator& alloc
        ) noexcept;
        ~IceshardMemoryFrame() noexcept override;

        auto memory_consumption() noexcept -> ice::u32 override;

        void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept override;

        auto requests() const noexcept -> ice::Span<EngineRequest const>;

    private:
        ice::memory::ScratchAllocator& _allocator;
        ice::memory::ScratchAllocator _inputs_allocator;
        ice::memory::ScratchAllocator _request_allocator;
        ice::memory::ScratchAllocator _message_allocator;
        ice::memory::ScratchAllocator _storage_allocator;
        ice::memory::ScratchAllocator _data_allocator;

        ice::pod::Array<ice::EngineRequest> _requests;
    };

} // namespace ice
