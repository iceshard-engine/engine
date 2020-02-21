#pragma once
#include <core/base.hxx>

namespace iceshard
{

    struct ComponentBlock
    {
        uint32_t _block_size = 0;
        uint32_t _entity_count_max = 0;
        uint32_t _entity_count = 0;

        ComponentBlock* _next = nullptr;
    };

} // namespace iceshard
