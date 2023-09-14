#include "android_platform_core.hxx"

#include <ice/mem_allocator_host.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

namespace ice::platform
{

    using ice::platform::android::AndroidCore;

    auto available_features() noexcept -> ice::platform::FeatureFlags
    {
        return FeatureFlags::Core | FeatureFlags::Paths;
    }

    auto initialize(
        ice::platform::FeatureFlags flags
    ) noexcept -> ice::Result
    {
        static ice::HostAllocator host_alloc;
        return initialize_with_allocator(flags, host_alloc);
    }

    auto initialize_with_allocator(
        ice::platform::FeatureFlags flags,
        ice::Allocator& alloc
    ) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        // Check that we have a JNI NativeActivity instance already created.
        if (AndroidCore::global_instance == nullptr)
        {
            return E_PlatformAndroidNotANativeActivity;
        }

        if (flags == FeatureFlags::None || !ice::has_all(available_features(), flags))
        {
            return Res::E_InvalidArgument;
        }

        return Res::Success;
    }

    auto query_api(ice::platform::FeatureFlags flag, void*& out_api_ptr) noexcept -> ice::Result
    {
        AndroidCore* instance_ptr = AndroidCore::global_instance;
        ICE_ASSERT(instance_ptr != nullptr, "Platform not initialized!");

        if (ice::has_all(available_features(), flag) == false)
        {
            return E_PlatformFeatureNotAvailable;
        }

        switch (flag)
        {
        case FeatureFlags::Core:
            out_api_ptr = static_cast<ice::platform::Core*>(instance_ptr);
            break;
        case FeatureFlags::Paths:
            out_api_ptr = static_cast<ice::platform::Paths*>(instance_ptr);
            break;
        default:
            return Res::E_InvalidArgument;
        }

        return Res::Success;
    }

    auto query_apis(ice::platform::FeatureFlags flags, void** out_api_ptrs) noexcept -> ice::Result
    {
        ice::Result result = Res::Success;
        for (FeatureFlags flag : { FeatureFlags::Core, FeatureFlags::RenderSurface })
        {
            if (ice::has_all(flags, flag))
            {
                result = query_api(flag, *out_api_ptrs);
                if (result == false)
                {
                    break;
                }
                out_api_ptrs += 1;
            }
            else
            {
                result = Res::E_InvalidArgument;
                break;
            }
        }

        return result;
    }

    void shutdown() noexcept
    {
        IPT_ZONE_SCOPED;

        // Not Implemented.
        // TODO: Check if we should programatically close the app
    }

} // namespace ice::platform
