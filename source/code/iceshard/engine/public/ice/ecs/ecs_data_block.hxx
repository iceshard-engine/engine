#pragma once
#include <ice/base.hxx>

namespace ice::ecs
{

    struct DataBlock
    {
        ice::u32 block_size = 0;
        ice::u32 block_entity_count_max = 0;
        ice::u32 block_entity_count = 0;
        void* block_data;

        ice::ecs::DataBlock* next;
    };

} // namespace ice::ecs
