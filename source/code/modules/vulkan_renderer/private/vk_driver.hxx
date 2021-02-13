#pragma once
#include <ice/render/render_driver.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>
#include "vk_allocator.hxx"
#include "vk_memory_manager.hxx"

namespace ice::render::vk
{

    class VulkanRenderDriver final : public ice::render::RenderDriver
    {
    public:
        VulkanRenderDriver(
            ice::UniquePtr<VulkanAllocator> vk_alloc,
            VkInstance vk_instance
        ) noexcept;
        ~VulkanRenderDriver() noexcept override;

        auto render_api() const noexcept -> ice::render::RenderDriverAPI override;

        auto create_surface(
            ice::render::SurfaceInfo const& surface_info
        ) noexcept -> ice::render::RenderSurface* override;
        void destroy_surface(
            ice::render::RenderSurface* surface
        ) noexcept override;

        void query_queue_infos(
            ice::pod::Array<ice::render::QueueFamilyInfo>& queue_info
        ) noexcept override;

        auto create_device(
            ice::Span<ice::render::QueueInfo const> queue_info
        ) noexcept -> ice::render::RenderDevice* override;

        void destroy_device(
            ice::render::RenderDevice* device
        ) noexcept override;

        //void _queue_infos(
        //    ice::pod::Array<ice::render::QueueInfo>& queue_info
        //) noexcept override;

        //bool create_queues(
        //    ice::render::QueueCreateInfo const& queue_info
        //) noexcept override;
        //auto get_queue(
        //    ice::render::QueueType type,
        //    ice::u32 queue_index
        //) noexcept -> ice::render::RenderQueue* override;

        void create_buffer() noexcept override;
        void release_buffer() noexcept override;

        void create_resource_set() noexcept override;
        void release_resource_set() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<VulkanAllocator> _vk_alloc;

        // Vulkan mative handles
        VkInstance _vk_instance;
        VkPhysicalDevice _vk_physical_device; 
        VkPhysicalDeviceMemoryProperties _vk_physical_device_memory_properties;
        ice::pod::Array<VkQueueFamilyProperties> _vk_queue_family_properties;

        ice::i32 _vk_presentation_queue_family_index = -1;
    };

} // namespace ice::render::vk
