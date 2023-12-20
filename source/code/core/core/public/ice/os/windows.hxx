/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

#if ISP_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace ice::win32
{

    enum class HandleType : ice::u8
    {
        FileHandle,
        DllHandle,
    };

    template<HandleType HType>
    struct Handle;

    using FileHandle = Handle<HandleType::FileHandle>;
    using DllHandle = Handle<HandleType::DllHandle>;

    template<HandleType HType>
    struct Win32NativeTypeDesciptor{ };

    template<>
    struct Win32NativeTypeDesciptor<HandleType::FileHandle>
    {
        using NativeType = HANDLE;

        static constexpr NativeType NullValue = INVALID_HANDLE_VALUE;

        static bool is_valid(NativeType handle) noexcept
        {
            return handle != nullptr && handle != INVALID_HANDLE_VALUE;
        }

        static bool close(NativeType handle) noexcept
        {
            return CloseHandle(handle) != 0;
        }
    };

    template<>
    struct Win32NativeTypeDesciptor<HandleType::DllHandle>
    {
        using NativeType = HMODULE;

        static constexpr NativeType NullValue = nullptr;

        static bool is_valid(NativeType handle) noexcept
        {
            return handle != nullptr;
        }

        static bool close(NativeType handle) noexcept
        {
            return FreeLibrary(handle) != 0;
        }
    };

    template<HandleType HType>
    struct Handle
    {
        using HandleInfo = Win32NativeTypeDesciptor<HType>;
        using NativeType = typename HandleInfo::NativeType;

        Handle() noexcept;
        Handle(NativeType value) noexcept;
        Handle(Handle&& other) noexcept;
        Handle(Handle const& other) noexcept = delete;
        ~Handle() noexcept;

        auto operator=(Handle&& other) noexcept -> Handle&;
        auto operator=(Handle const& other) noexcept -> Handle& = delete;

        operator bool() const noexcept { return HandleInfo::is_valid(_handle); }

        auto native() const noexcept -> NativeType { return _handle; }
        bool close() noexcept;

    private:
        NativeType _handle;
    };


    template<HandleType HType>
    Handle<HType>::Handle() noexcept
        : _handle{ HandleInfo::NullValue }
    {
    }

    template<HandleType HType>
    Handle<HType>::Handle(NativeType value) noexcept
        : _handle{ value }
    {
    }

    template<HandleType HType>
    Handle<HType>::Handle(Handle&& other) noexcept
        : _handle{ ice::exchange(other._handle, HandleInfo::NullValue) }
    {
    }

    template<HandleType HType>
    Handle<HType>::~Handle() noexcept
    {
        close();
    }

    template<HandleType HType>
    auto Handle<HType>::operator=(Handle&& other) noexcept -> Handle&
    {
        if (this != &other)
        {
            _handle = ice::exchange(other._handle, HandleInfo::NullValue);
        }
        return *this;
    }

    template<HandleType HType>
    bool Handle<HType>::close() noexcept
    {
        if (*this)
        {
            return HandleInfo::close(ice::exchange(_handle, HandleInfo::NullValue));
        }
        return false;
    }

} // namespace ice::win32

#endif // ISP_WINDOWS
