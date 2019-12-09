#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <vulkan/vulkan.h>

#include "vulkan_shader.hxx"
#include "pipeline/vulkan_pipeline_layout.hxx"
#include "pipeline/vulkan_vertex_descriptor.hxx"
#include "render_pass/vulkan_renderpass.hxx"

namespace render::vulkan
{

    class VulkanPipeline
    {
    public:
        VulkanPipeline(VkDevice device, VkPipeline pipeline) noexcept;
        ~VulkanPipeline() noexcept;

        auto native_handle() const noexcept -> VkPipeline { return _native_handle; }

    private:
        VkDevice _device_handle;
        VkPipeline _native_handle;
    };

    auto create_pipeline(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VulkanShader const*> shader_stages,
        core::pod::Array<VulkanVertexDescriptor const*> vertex_descriptors,
        VulkanPipelineLayout const* pipeline_layout,
        VulkanRenderPass const* render_pass) noexcept -> core::memory::unique_pointer<VulkanPipeline>;

} // namespace render::vulkan
