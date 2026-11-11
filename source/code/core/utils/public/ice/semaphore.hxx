/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/os.hxx>

namespace ice
{

#if ISP_UNIX
    namespace detail
    {
        struct UnixSemaphore final
        {
            ~UnixSemaphore() noexcept;

            bool post() noexcept;
            bool wait(ice::Tms duration) noexcept;

            sem_t _native;
        };
    } // namespace detail
#else
#error "Not implemented for this platform"
#endif

    using Semaphore = ice::UniquePtr<ice::detail::UnixSemaphore>;

    auto create_semaphore(ice::Allocator& alloc, ice::i32 initial_value) noexcept -> ice::Semaphore;

} // namespace ice