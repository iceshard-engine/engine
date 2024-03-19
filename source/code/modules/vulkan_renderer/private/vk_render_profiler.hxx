/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_include.hxx"
#include <ice/render/render_profiler.hxx>

#if IPT_ENABLED
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#undef assert
#endif

namespace ice::render::detail
{

#if IPT_ENABLED
    struct ProfilingZone::Internal : tracy::VkCtxScope
    {
        using VkCtxScope::VkCtxScope;

        static void on_delete_impl(Internal* obj) noexcept
        {
            delete obj;
        }
    };

    namespace vk
    {
        struct VulkanProfiledCommandBuffer
        {
            TracyVkCtx tracy_ctx;
        };
    } // namespace vk

#else

    namespace vk
    {
        struct VulkanProfiledCommandBuffer { };
    } // namespace vk

#endif


} // namespace ice::render::vk
