#pragma once
#include <ice/data.hxx>

namespace ice
{

    struct Memory final
    {
        void* location;
        u32 size;
        u32 alignment;

        inline operator ice::Data() noexcept;
    };


    inline Memory::operator ice::Data() noexcept
    {
        return Data{
            .location = this->location,
            .size = this->size,
            .alignment = this->alignment
        };
    }

    template<typename T>
    inline auto memory_block(T* location, u32 size, u32 alignment = alignof(T)) noexcept -> Memory
    {
        return Memory{
            .location = location,
            .size = size,
            .alignment = alignment,
        };
    }

    template<typename T>
    inline auto memory_block(T& object, u32 alignment = alignof(T)) noexcept -> Memory
    {
        return Memory{
            .location = &object,
            .size = sizeof(T),
            .alignment = alignment,
        };
    }

} // namespace ice
