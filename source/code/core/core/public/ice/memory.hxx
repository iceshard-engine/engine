#pragma once
#include <ice/data.hxx>

namespace ice
{

    struct Memory final
    {
        void* location;
        u32 size;
        u32 alignment;

        inline operator ice::Data() const noexcept;
    };


    inline Memory::operator ice::Data() const noexcept
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
