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

} // namespace ice
