#pragma once
#include <ice/base.hxx>

namespace ice
{

    struct Data final
    {
        void const* location;
        u32 size;
        u32 alignment;
    };

    template<typename T>
    inline auto data_view(T const* location, u32 size, u32 alignment = alignof(T)) noexcept -> Data
    {
        return Data{
            .location = location,
            .size = size,
            .alignment = alignment,
        };
    }

    template<typename T>
    inline auto data_view(T const& object, u32 alignment = alignof(T)) noexcept -> Data
    {
        return Data{
            .location = &object,
            .size = sizeof(T),
            .alignment = alignment,
        };
    }

} // namespace ice
