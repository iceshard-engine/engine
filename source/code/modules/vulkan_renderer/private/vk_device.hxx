#pragma once
#include <ice/render/render_device.hxx>
#include <ice/pod/array.hxx>
#include <ice/unique_ptr.hxx>

#include "vk_include.hxx"
#include "vk_memory_manager.hxx"
#include "vk_queue.hxx"

namespace ice::render::vk
{

    class VulkanRenderCommands final : public ice::render::RenderCommands
    {
    public:
        ~VulkanRenderCommands() noexcept override = default;

        void begin(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::RenderPass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::vec2u extent,
            ice::vec4f clear_color
        ) noexcept override;

        void next_subpass(
            ice::render::CommandBuffer cmds,
            ice::render::SubPassContents contents
        ) noexcept override;

        void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void end(
            ice::render::CommandBuffer cmds
        ) noexcept override;
    };

    class VulkanRenderDevice final : public ice::render::RenderDevice
    {
    public:
        VulkanRenderDevice(
            ice::Allocator& alloc,
            VkDevice vk_device,
            VkPhysicalDevice vk_physical_device,
            VkPhysicalDeviceMemoryProperties const& memory_properties
        ) noexcept;
        ~VulkanRenderDevice() noexcept;

        auto create_swapchain(
            ice::render::RenderSurface* surface
        ) noexcept -> ice::render::RenderSwapchain* override;

        void destroy_swapchain(
            ice::render::RenderSwapchain* swapchain
        ) noexcept override;

        auto create_renderpass(
            ice::render::RenderPassInfo const& info
        ) noexcept -> ice::render::RenderPass override;

        void destroy_renderpass(
            ice::render::RenderPass render_pass
        ) noexcept override;

        auto create_framebuffer(
            ice::vec2u extent,
            ice::render::RenderPass renderpass,
            ice::Span<ice::render::Image> images
        ) noexcept -> ice::render::Framebuffer override;

        void destroy_framebuffer(
            ice::render::Framebuffer framebuffer
        ) noexcept override;

        auto create_image(
            ice::render::ImageInfo image_info,
            ice::Data data
        ) noexcept -> ice::render::Image override;

        void destroy_image(
            ice::render::Image image
        ) noexcept override;

        auto create_queue(
            ice::render::QueueID queue_id,
            ice::u32 queue_index,
            ice::u32 command_pools
        ) const noexcept -> ice::render::RenderQueue* override;

        void destroy_queue(
            ice::render::RenderQueue* queue
        ) const noexcept override;

        auto get_commands() noexcept -> ice::render::RenderCommands& override;

    private:
        ice::Allocator& _allocator;
        VkDevice _vk_device;
        VkPhysicalDevice _vk_physical_device;

        ice::UniquePtr<VulkanMemoryManager> _vk_memory_manager;

        ice::pod::Array<VulkanQueue*> _vk_queues;
        VulkanRenderCommands _vk_render_commands;
    };

} // namespace ice::render::vk
