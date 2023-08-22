/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/sync_manual_events.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>
#include <ice/assert.hxx>

#if ISP_UNIX
#include <sys/syscall.h>
#include <linux/futex.h>
#endif

namespace ice
{

#if ISP_UNIX
    namespace detail
    {

        constexpr ice::i32 Constant_WakeAllWaiters = INT_MAX;
        constexpr ice::i32 Constant_WakeOneWaiter = INT_MAX;

        auto futex_wait(ice::i32* user_address, ice::i32 value) noexcept -> ice::i32
        {
            return syscall(
                SYS_futex,
                user_address,
                FUTEX_WAIT_PRIVATE,
                value,
                nullptr,
                nullptr,
                0
            );
        }

        auto futex_wake(ice::i32* user_address, ice::i32 value) noexcept -> ice::i32
        {
            return syscall(
                SYS_futex,
                user_address,
                FUTEX_WAKE_PRIVATE,
                value,
                nullptr,
                nullptr,
                0
            );
        }
    }
#endif // #if ISP_UNIX

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
#elif ISP_UNIX
        ice::i32 const waiters_woken_up = ice::detail::futex_wake(
            reinterpret_cast<ice::i32*>(&_internal_value),
            detail::Constant_WakeAllWaiters
        );
        ICE_ASSERT(waiters_woken_up != -1, "Failed futex call with error code: {}!", waiters_woken_up);
#endif
    }

    void ManualResetEvent::reset() noexcept
    {
        _internal_value.store(0, std::memory_order_relaxed);
    }

    void ManualResetEvent::wait() noexcept
    {
        ice::u8 value = _internal_value.load(std::memory_order_acquire);
#if ISP_WINDOWS
        bool wait_successful = true;
        while (value == 0)
        {
            if (wait_successful == false)
            {
                ::Sleep(1);
            }

            wait_successful = ::WaitOnAddress(
                ice::addressof(_internal_value),
                ice::addressof(value),
                sizeof(_internal_value),
                INFINITE
            );
            value = _internal_value.load(std::memory_order_acquire);
        }
#elif ISP_UNIX
        // Wait in a loop as futex() can have spurious wake-ups.
        while (value == 0)
        {
            ice::i32 const result = detail::futex_wait(
                reinterpret_cast<ice::i32*>(&_internal_value),
                value
            );

            if (result == -1)
            {
                if (errno == EAGAIN)
                {
                    // The state was changed from zero before we could wait.
                    // Must have been changed to 1.
                    return;
                }

                // Other errors we'll treat as transient and just read the
                // value and go around the loop again.
            }

            value = _internal_value.load(std::memory_order_acquire);
        }
#else
        ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
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

            if (remaining == 0)
            {
#if ISP_WINDOWS
                ::WakeByAddressAll(ice::addressof(_internal_value));
#elif ISP_UNIX
                ice::i32 const waiters_woken_up = ice::detail::futex_wake(
                    reinterpret_cast<ice::i32*>(&_internal_value),
                    detail::Constant_WakeAllWaiters
                );
                ICE_ASSERT(waiters_woken_up != -1, "Failed futex call with error code: {}!", waiters_woken_up);
#else
                ICE_ASSERT(false, "Synchronization not implemented for this platform!");
#endif
            }
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
#elif ISP_UNIX
                usleep(1000);
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
#elif ISP_UNIX
            ice::i32 const result = detail::futex_wait(
                reinterpret_cast<ice::i32*>(&_internal_value),
                value
            );

            if (result == -1)
            {
                wait_successful = (errno == EAGAIN);
            }
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
