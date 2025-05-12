/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>

namespace ice::ecs
{

    struct DataBlock
    {
        ice::ucount block_entity_count_max = 0;
        ice::ucount block_entity_count = 0;

        ice::usize block_data_size = 0_B;
        void* block_data;

        ice::ecs::DataBlock* next;
    };

} // namespace ice::ecs
