/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_app.hxx"

#include <ice/platform_core.hxx>
#include <ice/platform_storage.hxx>
#include <ice/platform_render_surface.hxx>

#include <ice/mem_allocator_host.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

namespace ice::platform
{

    using ice::platform::webasm::WebAsmApp;

    auto available_features() noexcept -> ice::platform::FeatureFlags
    {
        return FeatureFlags::Core
            | FeatureFlags::StoragePaths
            | FeatureFlags::RenderSurface
            | FeatureFlags::Threads;
    }

    auto initialize(
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Result
    {
        static ice::HostAllocator host_alloc;
        return initialize_with_allocator(host_alloc, flags, params);
    }

    auto initialize_with_allocator(
        ice::Allocator& alloc,
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        // Check that we have a JNI NativeActivity instance already created.
        if (WebAsmApp::global_instance == nullptr)
        {
            return ice::E_NotImplemented;
        }

        if (flags == FeatureFlags::None || !ice::has_all(available_features(), flags))
        {
            return ice::E_InvalidArgument; // TODO
        }

        WebAsmApp::global_instance->initialize(params);
        return ice::S_Success;
    }

    auto query_api(ice::platform::FeatureFlags flag, void*& out_api_ptr) noexcept -> ice::Result
    {
        WebAsmApp* instance_ptr = WebAsmApp::global_instance;
        ICE_ASSERT(instance_ptr != nullptr, "Platform not initialized!");

        if (ice::has_all(available_features(), flag) == false)
        {
            return ice::E_InvalidArgument; // TODO
        }

        switch (flag)
        {
        case FeatureFlags::Core:
            out_api_ptr = static_cast<ice::platform::Core*>(instance_ptr);
            break;
        case FeatureFlags::StoragePaths:
            out_api_ptr = static_cast<ice::platform::StoragePaths*>(instance_ptr);
            break;
        case FeatureFlags::Threads:
            out_api_ptr = instance_ptr->threads();
            break;
        case FeatureFlags::RenderSurface:
            out_api_ptr = &instance_ptr->_render_surface;
            break;
        default:
            return ice::E_InvalidArgument;
        }

        return ice::S_Success;
    }

    auto query_apis(ice::platform::FeatureFlags flags, void** out_api_ptrs) noexcept -> ice::Result
    {
        ice::Result result = ice::S_Success;
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
                result = ice::E_InvalidArgument;
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
