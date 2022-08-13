#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    class NullAllocator final : public ice::Allocator
    {
    public:
        NullAllocator(std::source_location src_loc = std::source_location::current()) noexcept
            : ice::Allocator{ src_loc }
        { }

    protected:
        auto do_allocate(
            [[maybe_unused]] ice::AllocRequest request
        ) noexcept -> ice::AllocResult override
        {
            return { .result = nullptr, .size = 0, .alignment = ice::ualign::invalid };
        }

        void do_deallocate(
            [[maybe_unused]] ice::Memory result
        ) noexcept override
        {
        }
    };

} // namespace ice
