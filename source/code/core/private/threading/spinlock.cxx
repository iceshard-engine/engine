#include <core/threading/spinlock.hxx>
#include <core/debug/assert.hxx>

namespace core::threading
{

spinlock::~spinlock() noexcept
{
    IS_ASSERT(!_flag.test_and_set(std::memory_order_relaxed), "This spinlock was not unlocked before destruction!");
}

void spinlock::lock() noexcept
{
    while (_flag.test_and_set(std::memory_order_acquire))
    {
        _mm_pause();
    }
}

void spinlock::unlock() noexcept
{
    _flag.clear(std::memory_order_release);
}

} // namespace core::threading
