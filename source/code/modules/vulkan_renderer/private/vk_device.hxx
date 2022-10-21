#pragma once
#include <ice/render/render_device.hxx>
#include <ice/container/array.hxx>
#include <ice/mem_allocator_ring.hxx>
#include <ice/mem_unique_ptr.hxx>

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
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::vec2u extent,
            ice::vec4f clear_color
        ) noexcept override;

        void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::Span<ice::vec4f> clear_values,
            ice::vec2u extent
        ) noexcept override;

        void next_subpass(
            ice::render::CommandBuffer cmds,
            ice::render::SubPassContents contents
        ) noexcept override;

        void set_viewport(
            ice::render::CommandBuffer cmds,
            ice::vec4u viewport_rect
        ) noexcept override;

        void set_scissor(
            ice::render::CommandBuffer cmds,
            ice::vec4u scissor_rect
        ) noexcept override;

        void bind_pipeline(
            ice::render::CommandBuffer cmds,
            ice::render::Pipeline pipeline
        ) noexcept override;

        void bind_resource_set(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline_layout,
            ice::render::ResourceSet resource_set,
            ice::u32 first_set
        ) noexcept override;

        void bind_index_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer
        ) noexcept override;

        void bind_vertex_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer,
            ice::u32 binding
        ) noexcept override;

        void draw(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept override;

        void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count
        ) noexcept override;

        void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 index_offset,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept override;

        void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void end(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void pipeline_image_barrier(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineStage source_stage,
            ice::render::PipelineStage destination_stage,
            ice::Span<ice::render::ImageBarrier const> image_barriers
        ) noexcept override;

        void update_texture(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept override;

        void update_texture_v2(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept override;

        void push_constant(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline,
            ice::render::ShaderStageFlags shader_stages,
            ice::Data data,
            ice::u32 offset
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
            ice::render::RenderpassInfo const& info
        ) noexcept -> ice::render::Renderpass override;

        void destroy_renderpass(
            ice::render::Renderpass render_pass
        ) noexcept override;

        auto create_resourceset_layout(
            ice::Span<ice::render::ResourceSetLayoutBinding const> bindings
        ) noexcept -> ice::render::ResourceSetLayout override;

        void destroy_resourceset_layout(
            ice::render::ResourceSetLayout resourceset_layout
        ) noexcept override;

        bool create_resourcesets(
            ice::Span<ice::render::ResourceSetLayout const> resource_set_layouts,
            ice::Span<ice::render::ResourceSet> resource_sets_out
        ) noexcept override;

        void update_resourceset(
            ice::Span<ice::render::ResourceSetUpdateInfo const> update_infos
        ) noexcept override;

        void destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const> resource_sets
        ) noexcept override;

        auto create_pipeline_layout(
            ice::render::PipelineLayoutInfo const& info
        ) noexcept -> ice::render::PipelineLayout override;

        void destroy_pipeline_layout(
            ice::render::PipelineLayout pipeline_layout
        ) noexcept override;

        auto create_shader(
            ice::render::ShaderInfo const& shader_info
        ) noexcept -> ice::render::Shader override;

        void destroy_shader(
            ice::render::Shader shader
        ) noexcept override;

        auto create_pipeline(
            ice::render::PipelineInfo const& info
        ) noexcept -> ice::render::Pipeline override;

        void destroy_pipeline(
            ice::render::Pipeline pipeline
        ) noexcept override;

        auto create_buffer(
            ice::render::BufferType buffer_type,
            ice::u32 buffer_size
        ) noexcept -> ice::render::Buffer override;

        void destroy_buffer(
            ice::render::Buffer buffer
        ) noexcept override;

        void update_buffers(
            ice::Span<ice::render::BufferUpdateInfo const> update_infos
        ) noexcept override;

        auto create_framebuffer(
            ice::vec2u extent,
            ice::render::Renderpass renderpass,
            ice::Span<ice::render::Image const> images
        ) noexcept -> ice::render::Framebuffer override;

        void destroy_framebuffer(
            ice::render::Framebuffer framebuffer
        ) noexcept override;

        auto create_image(
            ice::render::ImageInfo const& image_info,
            ice::Data data
        ) noexcept -> ice::render::Image override;

        void destroy_image(
            ice::render::Image image
        ) noexcept override;

        auto create_sampler(
            ice::render::SamplerInfo const& sampler_info
        ) noexcept -> ice::render::Sampler override;

        void destroy_sampler(
            ice::render::Sampler sampler
        ) noexcept override;

        auto create_queue(
            ice::render::QueueID queue_id,
            ice::u32 queue_index,
            ice::u32 command_pools
        ) const noexcept -> ice::render::RenderQueue* override;

        void destroy_queue(
            ice::render::RenderQueue* queue
        ) const noexcept override;

        auto create_fence() noexcept -> ice::render::RenderFence*;

        void destroy_fence(
            ice::render::RenderFence* fence
        ) noexcept;

        auto get_commands() noexcept -> ice::render::RenderCommands& override;

        void wait_idle() const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::RingAllocator _gfx_thread_alloc;

        VkDevice _vk_device;
        VkPhysicalDevice _vk_physical_device;
        VkDescriptorPool _vk_descriptor_pool;

        ice::UniquePtr<VulkanMemoryManager> _vk_memory_manager;

        ice::Array<VulkanQueue*> _vk_queues;
        VulkanRenderCommands _vk_render_commands;
    };

} // namespace ice::render::vk
