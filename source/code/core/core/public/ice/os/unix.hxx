/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/handle.hxx>

#if ISP_UNIX

#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <dlfcn.h>
#pragma clang diagnostic ignored "-Wunknown-attributes"

template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::File>
{
    //! \brief Unix file descriptors are represented using int values.
    //! \todo: Link to ref.
    using PlatformHandleType = int;

    //! \brief Unix file descriptors have a defined invalid value of '-1'
    //! \todo: Link to ref.
    static constexpr PlatformHandleType InvalidHandle = -1;

    static bool is_valid(PlatformHandleType handle, void*) noexcept
    {
        return handle != InvalidHandle;
    }

    static bool close(PlatformHandleType handle, void*) noexcept
    {
        return ::close(handle) == 0;
    }
};

template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::DynLib>
{
    using PlatformHandleType = void*;

    static constexpr PlatformHandleType InvalidHandle = nullptr;

    static bool is_valid(PlatformHandleType handle, void*) noexcept
    {
        return handle != InvalidHandle;
    }

    static bool close(PlatformHandleType handle, void*) noexcept
    {
        return dlclose(handle) == 0;
    }
};

// Can't use 'unix' since it's a define
namespace ice::unix_
{

    using FileHandle = ice::os::Handle<ice::os::HandleType::File>;
    using DynLibHandle = ice::os::Handle<ice::os::HandleType::DynLib>;

} // namespace ice::unix

#endif
