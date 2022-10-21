#pragma once
#include <ice/mem_types.hxx>

namespace ice
{

    struct Memory
    {
        void* location;
        ice::usize size;
        ice::ualign alignment;
    };

} // namespace ice
