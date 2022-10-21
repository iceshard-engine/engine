#pragma once
#include <ice/base.hxx>
#include <atomic>

namespace ice
{

    class ManualResetEvent
    {
    public:
        ManualResetEvent(bool set_on_create = false) noexcept;
        ~ManualResetEvent() noexcept = default;

        void set() noexcept;
        void reset() noexcept;
        void wait() noexcept;

        bool is_set() const noexcept;

    private:
        std::atomic<ice::u8> _internal_value;
    };

} // namespace ice
