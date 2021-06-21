#pragma once
#include <ice/allocator.hxx>

namespace ice::memory
{

    static constexpr uint32_t KB = 1024u;

    static constexpr uint32_t MB = KB * 1024u;


    void init(uint32_t scratch_buffer_size = 4 * MB) noexcept;

    [[deprecated("This function will be removed at a later version. Please use the regular `init` function instead!")]]
    void init_with_stats(uint32_t scratch_buffer_size = 4 * MB) noexcept;

    void shutdown() noexcept;

    auto default_allocator() noexcept -> ice::Allocator&;

    auto default_scratch_allocator() noexcept -> ice::Allocator&;

} // namespace ice::memory
