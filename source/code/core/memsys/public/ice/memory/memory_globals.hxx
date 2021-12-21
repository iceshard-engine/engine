#pragma once
#include <ice/allocator.hxx>

namespace ice::memory
{

    static constexpr ice::u32 KB = 1024u;

    static constexpr ice::u32 MB = KB * 1024u;


    void init(ice::u32 scratch_buffer_size = 4 * MB) noexcept;

    void shutdown() noexcept;

    auto default_allocator() noexcept -> ice::Allocator&;

    auto default_scratch_allocator() noexcept -> ice::Allocator&;

} // namespace ice::memory
