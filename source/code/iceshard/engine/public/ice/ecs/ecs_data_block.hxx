/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>

namespace ice::ecs::detail
{

    //! \brief Descriptor of a memory block containing data for entities of a specific Archetype.
    struct DataBlock
    {
        //! \brief Maximum number of entities this specific data-block can hold. This value can change depending on the block size.
        //!
        //! \note It is important to remember that a block is not able to hold more than `ice::ecs::Constant_MaxBlockEntityIndex` entities.
        ice::ucount block_entity_count_max = 0;

        //! \brief Current number of entities in the block.
        //!
        //! \note The `EntityStorage` ensures that all entities are always stored one after another inside blocks without empty data locations.
        ice::ucount block_entity_count = 0;

        //! \brief Size of the available data to be used for entity components.
        ice::usize block_data_size = 0_B;

        //! \brief Pointer to component data.
        void* block_data;

        ice::ecs::detail::DataBlock* next;
    };

} // namespace ice::ecs
