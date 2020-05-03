#pragma once
#include <core/allocator.hxx>
#include <core/pod/hash.hxx>
#include <core/pointer.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_swapchain.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_framebuffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>
#include <iceshard/renderer/vulkan/vulkan_resources.hxx>
#include <iceshard/renderer/vulkan/vulkan_pipeline.hxx>

#include <atomic>

namespace iceshard::renderer::vulkan
{

    class VulkanBuffer;
    class VulkanDeviceMemoryManager;

    class VulkanRenderSystem final : iceshard::renderer::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept;
        ~VulkanRenderSystem() noexcept override;

        void begin_frame() noexcept override;

        void end_frame() noexcept override;

        auto acquire_command_buffer(RenderPassStage stage) noexcept->CommandBuffer override;

        void submit_command_buffer(CommandBuffer cmd_buffer) noexcept override;


        auto get_resource_set(
            core::stringid_arg_type name
        ) noexcept -> ResourceSet override;

        auto create_resource_set(
            core::stringid_arg_type name,
            iceshard::renderer::RenderPipelineLayout layout,
            core::pod::Array<RenderResource> const& resources
        ) noexcept -> ResourceSet override;

        void update_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<RenderResource> const& resources
        ) noexcept override;

        void destroy_resource_set(
            core::stringid_arg_type name
        ) noexcept override;


        auto create_pipeline(
            core::stringid_arg_type name,
            RenderPipelineLayout layout,
            core::pod::Array<asset::AssetData> const& shader_assets
        ) noexcept -> Pipeline override;

        void destroy_pipeline(
            core::stringid_arg_type name
        ) noexcept override;

        auto create_data_buffer(
            iceshard::renderer::api::BufferType type,
            uint32_t size
        ) noexcept -> iceshard::renderer::api::Buffer override;

    public:
        // API v1.0 proxies
        auto devices() noexcept -> VulkanDevices const& { return _devices; }

        auto swapchain() noexcept -> VulkanSwapchain;

        auto render_area() noexcept -> VkExtent2D;

        auto resource_layouts() noexcept -> VulkanResourceLayouts;

    public:
        auto v1_surface() noexcept -> VkSurfaceKHR;
        auto v1_graphics_device() noexcept -> VkDevice;
        auto v1_graphics_queue() noexcept -> VkQueue;
        auto v1_renderpass() noexcept -> VkRenderPass;
        auto v1_current_framebuffer() noexcept -> VkFramebuffer;
        auto v1_framebuffer_semaphore() noexcept -> VkSemaphore const*;

        auto v1_primary_cmd_buffer() noexcept -> VkCommandBuffer;
        auto v1_secondary_cmd_buffer() noexcept -> VkCommandBuffer;
        auto v1_transfer_cmd_buffer() noexcept -> VkCommandBuffer;

    public:
        void v1_submit_command_buffers() noexcept;
        void v1_execute_subpass_commands(VkCommandBuffer cmds) noexcept;
        void v1_present() noexcept;

    protected:
        void acquire_next_image() noexcept;
        void present_image() noexcept;

    private:
        core::allocator& _allocator;
        bool _initialized = false;

        VkInstance const _vk_instance;

        VkExtent2D _render_area;

        VulkanSurface _surface;
        VulkanDevices _devices;
        VulkanSwapchain _swapchain;

        core::memory::unique_pointer<VulkanDeviceMemoryManager> _device_memory_manager;
        core::Vector<core::memory::unique_pointer<iceshard::renderer::vulkan::VulkanBuffer>> _vulkan_buffers;

        uint32_t _subpass_stage;
        VulkanRenderPass _renderpass;

        VulkanCommandBuffers _command_buffers;
        core::pod::Array<VkCommandBuffer> _command_buffers_secondary;
        std::atomic_uint32_t _next_command_buffer = 0;

        core::pod::Array<VkCommandBuffer> _command_buffers_submitted;
        std::atomic_uint32_t _submitted_command_buffer_count = 0;
        core::pod::Array<uint32_t> _command_buffers_subpass;

        uint32_t _current_framebuffer_index = 0;
        core::pod::Array<VulkanFramebuffer> _framebuffers;
        VkSemaphore _framebuffer_semaphore = vk_nullptr;

        VulkanResourcePool _resource_pool;
        VulkanResourceLayouts _resource_layouts;
        core::pod::Hash<VulkanResourceSet*> _resource_sets;

        VulkanPipelineLayouts _pipeline_layouts;
        core::pod::Hash<VulkanPipeline*> _pipelines;

        // Synchronization
        VkFence _draw_fence;
    };

    auto create_render_system(core::allocator& alloc, VkInstance device) noexcept -> VulkanRenderSystem*;

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept;

} // namespace iceshard::renderer::vulkan
