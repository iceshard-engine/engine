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
        auto do_allocate(ice::AllocRequest) noexcept -> ice::AllocResult override
        {
            return { nullptr, 0_B, ice::ualign::invalid };
        }

        void do_deallocate(void*) noexcept override { }
    };

} // namespace ice
