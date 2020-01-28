#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#include "vulkan_shader.hxx"
#include "pipeline/vulkan_vertex_descriptor.hxx"

#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

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
        VkPipelineLayout pipeline_layout,
        VkRenderPass render_pass
    ) noexcept -> core::memory::unique_pointer<VulkanPipeline>;

} // namespace render::vulkan
