/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/handle.hxx>

#if ISP_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::File>
{
    //! \brief Unix file descriptors are represented using int values.
    //! \todo: Link to ref.
    using PlatformHandleType = HANDLE;

    //! \brief Unix file descriptors have a defined invalid value of '-1'
    //! \todo: Link to ref.
    static constexpr PlatformHandleType InvalidHandle = INVALID_HANDLE_VALUE;

    static bool is_valid(PlatformHandleType handle, void*) noexcept
    {
        return /*handle != nullptr &&*/ handle != InvalidHandle;
    }

    static bool close(PlatformHandleType handle, void*) noexcept
    {
        return CloseHandle(handle) != 0;
    }
};

template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::DynLib>
{
    using PlatformHandleType = HMODULE;

    static constexpr PlatformHandleType InvalidHandle = NULL;

    static bool is_valid(PlatformHandleType handle, void*) noexcept
    {
        return handle != InvalidHandle;
    }

    static bool close(PlatformHandleType handle, void*) noexcept
    {
        return FreeLibrary(handle) != 0;
    }
};

namespace ice::win32
{

    using FileHandle = ice::os::Handle<ice::os::HandleType::File>;
    using DynLibHandle = ice::os::Handle<ice::os::HandleType::DynLib>;

} // namespace ice::win32

#endif // ISP_WINDOWS
