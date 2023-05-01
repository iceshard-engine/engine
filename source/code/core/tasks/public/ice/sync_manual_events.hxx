/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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

    class ManualResetSemaphore
    {
    public:
        ManualResetSemaphore(ice::u8 num_awaited = 0) noexcept;
        ~ManualResetSemaphore() noexcept = default;

        void set() noexcept;
        void reset(ice::u8 num_awaited) noexcept;
        void wait() noexcept;

        bool is_set() const noexcept;

    private:
        std::atomic<ice::u8> _internal_value;
    };

} // namespace ice
