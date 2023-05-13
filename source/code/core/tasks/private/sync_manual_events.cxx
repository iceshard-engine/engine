/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/sync_manual_events.hxx>
#include <ice/os/windows.hxx>
#include <ice/assert.hxx>

namespace ice
{

    // #TODO: https://github.com/iceshard-engine/engine/issues/90

    ManualResetEvent::ManualResetEvent(bool set_on_create) noexcept
        : _internal_value{ static_cast<ice::u8>(set_on_create ? 1 : 0) }
    {
    }

    void ManualResetEvent::set() noexcept
    {
        _internal_value.store(1, std::memory_order_release);
#if ISP_WINDOWS
        ::WakeByAddressAll(ice::addressof(_internal_value));
#else
        ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
    }

    void ManualResetEvent::reset() noexcept
    {
        _internal_value.store(0, std::memory_order_relaxed);
    }

    void ManualResetEvent::wait() noexcept
    {
        ice::u8 value = _internal_value.load(std::memory_order_acquire);

        bool wait_successful = true;
        while (value == 0)
        {
            if (wait_successful == false)
            {
#if ISP_WINDOWS
                ::Sleep(1);
#else
                ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
            }

#if ISP_WINDOWS
            wait_successful = ::WaitOnAddress(
                ice::addressof(_internal_value),
                ice::addressof(value),
                sizeof(_internal_value),
                INFINITE
            );
#else
            ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
            value = _internal_value.load(std::memory_order_acquire);
        }
    }

    bool ManualResetEvent::is_set() const noexcept
    {
        return _internal_value.load(std::memory_order_relaxed) == 1;
    }

    ManualResetBarrier::ManualResetBarrier(ice::u8 num_awaited) noexcept
        : _internal_value{ num_awaited }
    {
    }

    void ManualResetBarrier::set() noexcept
    {
        ice::u8 remaining = _internal_value.load(std::memory_order_relaxed);
        if (remaining != 0)
        {
            bool success = false;
            while (success == false && remaining > 0)
            {
                success = _internal_value.compare_exchange_weak(remaining, remaining - 1, std::memory_order_release, std::memory_order_relaxed);
            }

            ICE_ASSERT(success, "We got more 'set' calls than expected!");

#if ISP_WINDOWS
            ::WakeByAddressAll(ice::addressof(_internal_value));
#else
            ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
        }
    }

    void ManualResetBarrier::reset(ice::u8 num_awaited) noexcept
    {
        _internal_value.store(num_awaited, std::memory_order_relaxed);
    }

    void ManualResetBarrier::wait() noexcept
    {
        ice::u8 value = _internal_value.load(std::memory_order_acquire);

        bool wait_successful = true;
        while (value != 0)
        {
            if (wait_successful == false)
            {
#if ISP_WINDOWS
                ::Sleep(1);
#else
                ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
            }

#if ISP_WINDOWS
            wait_successful = ::WaitOnAddress(
                ice::addressof(_internal_value),
                ice::addressof(value),
                sizeof(_internal_value),
                INFINITE
            );
#else
            ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
            value = _internal_value.load(std::memory_order_acquire);
        }
    }

    bool ManualResetBarrier::is_set() const noexcept
    {
        return _internal_value.load(std::memory_order_relaxed) == 0;
    }

} // namespace ice
