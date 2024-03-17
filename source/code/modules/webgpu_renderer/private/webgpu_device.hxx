/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webgpu_utils.hxx"
#include "webgpu_commands.hxx"

#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_shader.hxx>

namespace ice::render::webgpu
{

    class WebGPUDevice : public ice::render::RenderDevice
    {
    public:
        WebGPUDevice(
            ice::Allocator& alloc,
            WGPUDevice device
        ) noexcept;
        ~WebGPUDevice() noexcept;

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

        virtual auto create_image(
            ice::render::ImageInfo const& image_info,
            ice::Data data = {}
        ) noexcept -> ice::render::Image override;

        virtual void destroy_image(
            ice::render::Image image
        ) noexcept override;

        virtual auto create_sampler(
            ice::render::SamplerInfo const& sampler_info
        ) noexcept -> ice::render::Sampler override;

        virtual void destroy_sampler(
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

        auto create_fence() noexcept -> ice::render::RenderFence* override;
        void destroy_fence(
            ice::render::RenderFence* fence
        ) noexcept override;

        virtual auto get_commands() noexcept -> ice::render::RenderCommands& override { return _commands; }

        virtual void wait_idle() const noexcept override { }

    private:
        ice::Allocator& _allocator;
        WGPUDevice _wgpu_device;
        WGPUQueue _wgpu_queue;
        WebGPUCommands _commands;
    };

} // namespace ice::render::webgpu
