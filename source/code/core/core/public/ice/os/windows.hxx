#pragma once
#include <ice/base.hxx>
//#include <ice/assert.hxx>

#if ISP_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace ice::win32
{

    template<typename HandleType>
    class SafeHandle
    {
        static constexpr bool Constant_IsHandleSupported = false
            || std::is_same_v<HandleType, HANDLE>
            || std::is_same_v<HandleType, HMODULE>;

        static_assert(Constant_IsHandleSupported, "The given handle type is not supported by ice::win32::SafeHandle!");

    public:
        ~SafeHandle() noexcept;
        SafeHandle(HandleType native_handle) noexcept;
        SafeHandle(SafeHandle&& other) noexcept;
        SafeHandle(SafeHandle const&) = delete;

        auto operator=(SafeHandle&& other) noexcept -> SafeHandle&;
        auto operator=(SafeHandle const& other) noexcept -> SafeHandle& = delete;
        operator bool() const noexcept;

        auto native() const noexcept -> HandleType;

    private:
        HandleType _handle;
    };

    template<typename HandleType>
    inline SafeHandle<HandleType>::~SafeHandle() noexcept
    {
        if constexpr (std::is_same_v<HandleType, HANDLE>)
        {
            if (_handle != nullptr && _handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(_handle);
            }
        }
        else if constexpr (std::is_same_v<HandleType, HMODULE>)
        {
            if (_handle != nullptr)
            {
                FreeLibrary(_handle);
            }
        }
    }

    template<typename HandleType>
    inline SafeHandle<HandleType>::SafeHandle(HandleType native_handle) noexcept
        : _handle{ native_handle }
    {
    }

    template<typename HandleType>
    inline SafeHandle<HandleType>::SafeHandle(SafeHandle&& other) noexcept
        : _handle{ ice::exchange(other._handle, nullptr) }
    {
    }

    template<typename HandleType>
    inline auto SafeHandle<HandleType>::operator=(SafeHandle&& other) noexcept -> SafeHandle&
    {
        if (this == &other)
        {
            _handle = ice::exchange(other._handle, nullptr);
        }
        return *this;
    }

    template<typename HandleType>
    inline SafeHandle<HandleType>::operator bool() const noexcept
    {
        if constexpr (std::is_same_v<HandleType, HANDLE>)
        {
            return _handle != nullptr && _handle != INVALID_HANDLE_VALUE;
        }
        else if constexpr (std::is_same_v<HandleType, HMODULE>)
        {
            return _handle != nullptr;
        }
    }

    template<typename HandleType>
    inline auto SafeHandle<HandleType>::native() const noexcept -> HandleType
    {
        return _handle;
    }

    using SHHandle = SafeHandle<HANDLE>;
    using SHHModule = SafeHandle<HMODULE>;

} // namespace ice::win32

#endif // ISP_WINDOWS
