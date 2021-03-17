#pragma once
#include <ice/render/render_queue.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_buffer.hxx>

namespace ice::render
{

    enum class Semaphore : ice::uptr
    {
        Invalid = 0x0
    };

    class RenderSurface;

    class RenderSwapchain;

    class RenderDevice
    {
    protected:
        virtual ~RenderDevice() noexcept = default;

    public:
        //virtual auto active_queue_count() const noexcept -> ice::u32 = 0;
        virtual auto create_swapchain(
            ice::render::RenderSurface* surface
        ) noexcept -> ice::render::RenderSwapchain* = 0;

        virtual void destroy_swapchain(
            ice::render::RenderSwapchain* swapchain
        ) noexcept = 0;

        virtual auto create_renderpass(
            ice::render::RenderPassInfo const& info
        ) noexcept -> ice::render::RenderPass = 0;

        virtual void destroy_renderpass(
            ice::render::RenderPass render_pass
        ) noexcept = 0;

        virtual auto create_resourceset_layout(
            ice::Span<ice::render::ResourceSetLayoutBinding const> bindings
        ) noexcept -> ice::render::ResourceSetLayout = 0;

        virtual void destroy_resourceset_layout(
            ice::render::ResourceSetLayout resourceset_layout
        ) noexcept = 0;

        virtual bool create_resourcesets(
            ice::Span<ice::render::ResourceSetLayout const> resource_set_layouts,
            ice::Span<ice::render::ResourceSet> resource_sets_out
        ) noexcept = 0;

        virtual void destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const> resource_sets
        ) noexcept = 0;

        virtual auto create_pipeline_layout(
            ice::render::PipelineLayoutInfo const& info
        ) noexcept -> ice::render::PipelineLayout = 0;

        virtual void destroy_pipeline_layout(
            ice::render::PipelineLayout pipeline_layout
        ) noexcept = 0;

        virtual auto create_shader(
            ice::render::ShaderInfo const& shader_info
        ) noexcept -> ice::render::Shader = 0;

        virtual void destroy_shader(
            ice::render::Shader shader
        ) noexcept = 0;

        virtual auto create_pipeline(
            ice::render::PipelineInfo const& info
        ) noexcept -> ice::render::Pipeline = 0;

        virtual void destroy_pipeline(
            ice::render::Pipeline pipeline
        ) noexcept = 0;

        virtual auto create_buffer(
            ice::render::BufferType buffer_type,
            ice::u32 buffer_size
        ) noexcept -> ice::render::Buffer = 0;

        virtual void destroy_buffer(
            ice::render::Buffer buffer
        ) noexcept = 0;

        virtual void update_buffers(
            ice::Span<ice::render::BufferUpdateInfo const> update_infos
        ) noexcept = 0;

        virtual auto create_framebuffer(
            ice::vec2u extent,
            ice::render::RenderPass renderpass,
            ice::Span<ice::render::Image const> images
        ) noexcept -> ice::render::Framebuffer = 0;

        virtual void destroy_framebuffer(
            ice::render::Framebuffer framebuffer
        ) noexcept = 0;

        virtual auto create_image(
            ice::render::ImageInfo image_info,
            ice::Data data
        ) noexcept -> ice::render::Image = 0;

        virtual void destroy_image(
            ice::render::Image image
        ) noexcept = 0;

        virtual auto create_queue(
            ice::render::QueueID queue_id,
            ice::u32 queue_index,
            ice::u32 command_pools
        ) const noexcept -> ice::render::RenderQueue* = 0;

        virtual void destroy_queue(
            ice::render::RenderQueue* queue
        ) const noexcept = 0;

        virtual auto get_commands() noexcept -> ice::render::RenderCommands& = 0;

        virtual auto temp_submit_semaphore() noexcept -> ice::render::Semaphore = 0;
    };

} // namespace ice::render
