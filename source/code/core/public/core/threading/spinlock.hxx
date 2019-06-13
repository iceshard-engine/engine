#pragma once
#include <atomic>

namespace core::threading
{

//! \brief A very simple spinlock implementation.
class spinlock final
{
public:
    //! \brief Creates a un-set spinlock.
    spinlock() noexcept = default;

    //! \brief Destroys the spinlock object.
    //! \pre The spinlock object is not locked.
    ~spinlock() noexcept;

    //! \brief Waits for the spinlock to be unlocked, and locks it.
    void lock() noexcept;

    //! \brief Unlocks the spinlock object.
    void unlock() noexcept;

private:
    std::atomic_flag _flag = ATOMIC_FLAG_INIT;
};

} // namespace core::threading
