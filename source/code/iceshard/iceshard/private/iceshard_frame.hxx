#pragma once
#include <ice/engine_frame.hxx>
#include <ice/memory/scratch_allocator.hxx>

namespace ice
{

    class IceshardMemoryFrame final : public ice::EngineFrame
    {
    public:
        IceshardMemoryFrame(
            ice::memory::ScratchAllocator& alloc
        ) noexcept;
        ~IceshardMemoryFrame() noexcept override = default;

        auto memory_consumption() noexcept -> ice::u32 override;

    private:
        ice::memory::ScratchAllocator& _allocator;
        ice::memory::ScratchAllocator _inputs_allocator;
        ice::memory::ScratchAllocator _message_allocator;
        ice::memory::ScratchAllocator _storage_allocator;
        ice::memory::ScratchAllocator _data_allocator;
    };

} // namespace ice
