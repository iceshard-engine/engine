#pragma once

namespace iceshard
{

    struct ComponentBlock
    {
        uint32_t const _entity_count_max;
        uint32_t _entity_count = 0;

        ComponentBlock* _next = nullptr;
    };

} // namespace iceshard
