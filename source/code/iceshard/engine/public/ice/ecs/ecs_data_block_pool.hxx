/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/ecs/ecs_data_block.hxx>

namespace ice::ecs::detail
{

    //! \brief The default size for data-blocks used by the 'DataBlockPool' implementation.
    static constexpr ice::usize Constant_DefaultBlockSize = 32_KiB;

    //! \brief Pool of data blocks used by `EntityStorage` system to manage memory for entities.
    class DataBlockPool
    {
    public:
        virtual ~DataBlockPool() noexcept = default;

        //! \return Size in bytes for each allocated block.
        //!
        //! \note Currently the `EntityStorage` implementation expects all blocks to have the same size.
        virtual auto provided_block_size() const noexcept -> ice::usize = 0;

        //! \return A unused data block ready to store entity data.
        //!
        //! \note (For developers) After this function returns the pool should't access any of the fields in the block structure.
        virtual auto request_block() noexcept -> ice::ecs::detail::DataBlock* = 0;

        //! \brief Returns the block to the pool, ensuring that no data will read from or written to it.
        //! \pre The block parameter is not a `nullptr`.
        //!
        //! \param block The block to be released.
        //!
        //! \note (For developers) Once this function is called the pool can do anything with the memory it represents. No operations
        //!   are permitted anymore once a block was returned.
        virtual void release_block(ice::ecs::detail::DataBlock* block) noexcept = 0;
    };

    //! \brief A default, simplistic implementation of a block pool used as fallback in the `EntityStorage` implementation.
    auto create_default_block_pool(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::ecs::detail::DataBlockPool>;

} // namespace ice::ecs
