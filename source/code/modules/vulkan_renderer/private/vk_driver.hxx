/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_driver.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container/array.hxx>
#include "vk_allocator.hxx"
#include "vk_memory_allocator.hxx"
#include "vk_extensions.hxx"

namespace ice::render::vk
{

    class VulkanRenderDriver final : public ice::render::RenderDriver
    {
    public:
        VulkanRenderDriver(
            ice::Allocator& alloc,
            ice::UniquePtr<VulkanAllocator> vk_alloc,
            VkInstance vk_instance,
            ice::render::vk::Extension instance_extensions
        ) noexcept;
        ~VulkanRenderDriver() noexcept override;

        auto render_api() const noexcept -> ice::render::RenderDriverAPI override;

        [[deprecated]]
        auto create_surface(
            ice::render::SurfaceInfo const& surface_info
        ) noexcept -> ice::render::RenderSurface* override;
        void destroy_surface(
            ice::render::RenderSurface* surface
        ) noexcept override;

        [[deprecated]]
        void query_queue_infos(
            ice::Array<ice::render::QueueFamilyInfo>& queue_info
        ) noexcept override;

        auto create_device(
            ice::Span<ice::render::QueueInfo const> queue_info
        ) noexcept -> ice::render::RenderDevice* override;

        void destroy_device(
            ice::render::RenderDevice* device
        ) noexcept override;

        auto allocator() noexcept -> ice::Allocator& { return _allocator.backing_allocator(); }

    private:
        ice::ProxyAllocator _allocator;
        ice::UniquePtr<VulkanAllocator> _vk_alloc;

        // Vulkan mative handles
        VkInstance _vk_instance;
        VkPhysicalDevice _vk_physical_device;
        VkPhysicalDeviceMemoryProperties _vk_physical_device_memory_properties;
        ice::Array<VkQueueFamilyProperties> _vk_queue_family_properties;

        // TODO: This value should not be stored here, as it might change for each created surface!
        ice::i32 _vk_presentation_queue_family_index = -1;

        // Enabled extensions
        ice::render::vk::Extension const _vk_instance_extensions;
    };

} // namespace ice::render::vk
