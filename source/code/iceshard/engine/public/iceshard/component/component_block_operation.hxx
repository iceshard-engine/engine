#pragma once
#include <core/base.hxx>
#include <core/pod/array.hxx>
#include <iceshard/component/component_block.hxx>

namespace iceshard
{

    namespace detail
    {

        void component_block_prepare(
            ComponentBlock* block,
            uint32_t const* sizes,
            uint32_t const* alignments,
            uint32_t array_count,
            uint32_t* offsets
        ) noexcept;

        void component_block_prepare(
            ComponentBlock* block,
            uint32_t const* sizes,
            uint32_t const* alignments,
            uint32_t array_count,
            void** pointers
        ) noexcept;

    } // namespace detail

    template<typename... Args>
    void component_block_prepare(ComponentBlock* block, Args&... pointers) noexcept
    {
        uint32_t const alignments[] = { alignof(std::remove_pointer_t<Args>) ... };
        uint32_t const sizes[] = { sizeof(std::remove_pointer_t<Args>) ... };
        void** pointers_array[] = { reinterpret_cast<void**>(&pointers) ... };

        return detail::component_block_prepare(block, alignments, sizes, sizeof...(Args), *pointers_array);
    }

    auto component_block_insert(ComponentBlock* block, uint32_t entity_count, uint32_t& first_index) noexcept -> uint32_t;

} // namespace iceshard
