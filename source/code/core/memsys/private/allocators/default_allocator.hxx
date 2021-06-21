#pragma once
#include "allocator_utils.hxx"

#include <ice/allocator.hxx>
#include <atomic>

namespace ice::memory
{

    class DefaultAllocator final : public ice::Allocator
    {
    public:
        DefaultAllocator() noexcept = default;
        ~DefaultAllocator() noexcept override;

        auto allocate(uint32_t size, uint32_t align) noexcept -> void* override;

        void deallocate(void* ptr) noexcept override;

        auto allocated_size(void* ptr) const noexcept -> uint32_t override;

        auto total_allocated() const noexcept -> uint32_t override;

    private:
        std::atomic_uint32_t _total_allocated{ 0 };
    };

} // namespace ice::memory
