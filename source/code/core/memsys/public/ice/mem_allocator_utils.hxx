#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    inline auto data_copy(ice::Allocator& alloc, ice::Data data) noexcept -> ice::Memory
    {
        ice::Memory const result = alloc.allocate({ data.size, data.alignment });
        if (result.location != nullptr)
        {
            ice::memcpy(result, data);
        }
        return result;
    }

} // namespace ice
