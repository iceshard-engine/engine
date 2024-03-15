/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/unix.hxx>

#if ISP_ANDROID
#include <jni.h>

template<typename JNIType>
struct AndroidJNIHandleDescriptorBase
{
    //! \brief Unix file descriptors are represented using int values.
    //! \todo: Link to ref.
    using PlatformHandleType = JNIType;
    using HandleManagerType = JNIEnv*;

    //! \brief Unix file descriptors have a defined invalid value of '-1'
    //! \todo: Link to ref.
    static constexpr PlatformHandleType InvalidHandle = JNIType{};

    static bool is_valid(PlatformHandleType handle, HandleManagerType jni) noexcept
    {
        return handle != InvalidHandle && jni != nullptr;
    }

    static bool close(PlatformHandleType handle, HandleManagerType jni) noexcept
    {
        jni->DeleteLocalRef(handle);
        return true;
    }
};

template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::JClass> : AndroidJNIHandleDescriptorBase<jclass> { };
template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::JObject> : AndroidJNIHandleDescriptorBase<jobject> { };
template<>
struct ice::os::HandleDescriptor<ice::os::HandleType::JString> : AndroidJNIHandleDescriptorBase<jstring> { };

// Can't use 'unix' since it's a define
namespace ice::jni
{

    using JClass  = ice::os::Handle<ice::os::HandleType::JClass>;
    using JObject = ice::os::Handle<ice::os::HandleType::JObject>;
    using JString = ice::os::Handle<ice::os::HandleType::JString>;

} // namespace ice::unix

#endif // ISP_ANDROID
