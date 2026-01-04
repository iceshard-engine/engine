/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string.hxx>

namespace ice::native_aio
{

    using AIOPort = struct AIOPortInternal*;

    struct AIOPortInfo
    {
        ice::u32 worker_limit = 1;
        ice::String debug_name;
    };

    struct AIOStatusInfo
    {
    };

    enum class AIORequestResult : ice::u8
    {
        Error,
        Success,
        Timeout,
    };

    using AIORequestCallback = void(*)(
        ice::native_aio::AIORequestResult result,
        ice::usize bytes_read,
        void* userdata
    ) noexcept;

    struct alignas(8) AIORequest
    {
        using enum AIORequestResult;

        //! Note the buffer is large enough to store any systems internally required values.
        //! \note (using 40 bytes to store one AIORequest in a single cacheline and not share them)
        char _internal[40];

        ice::native_aio::AIOPort _port;

        //! \brief Callback to be called when the request completes.
        ice::native_aio::AIORequestCallback _callback = nullptr;

        //! \brief Additional userdata to be passed to the callback.
        void* _userdata = nullptr;
    };

    auto aio_open(
        ice::Allocator& alloc,
        ice::native_aio::AIOPortInfo const& info
    ) noexcept -> ice::native_aio::AIOPort;

    void aio_close(ice::native_aio::AIOPort port) noexcept;
    auto aio_status(ice::native_aio::AIOPort port) noexcept -> ice::native_aio::AIOStatusInfo;
    auto aio_native_handle(ice::native_aio::AIOPort port) noexcept -> void*;
    auto aio_worker_limit(ice::native_aio::AIOPort port) noexcept -> ice::u32;

    struct AIOProcessLimits
    {
        ice::u32 timeout_ms = ice::u32_max;
        ice::u32 events_max = ice::u32_max;
    };

    auto aio_process_events(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits = {}
    ) noexcept -> ice::u32;

} // namespace ice::native_aio
