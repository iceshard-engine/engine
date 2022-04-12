#pragma once
#include <ice/base.hxx>

namespace ice
{

    struct Data final
    {
        void const* location = nullptr;
        ice::u32 size = 0;
        ice::u32 alignment = 0;
    };

    template<typename T>
    inline auto data_view(T const* location, ice::u32 size, ice::u32 alignment = alignof(T)) noexcept -> ice::Data
    {
        return Data{
            .location = location,
            .size = size,
            .alignment = alignment,
        };
    }

    template<typename T>
    inline auto data_view(T const& object, ice::u32 alignment = alignof(T)) noexcept -> ice::Data
    {
        return Data{
            .location = &object,
            .size = sizeof(T),
            .alignment = alignment,
        };
    }

    template<typename T, ice::u32 Size>
    constexpr auto data_view(T const (&object)[Size], ice::u32 alignment = alignof(T)) noexcept -> ice::Data
    {
        return Data{
            .location = &object,
            .size = sizeof(T) * Size,
            .alignment = alignment,
        };
    }

} // namespace ice
