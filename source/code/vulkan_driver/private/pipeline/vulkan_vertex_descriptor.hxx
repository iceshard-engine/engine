#pragma once
#include <core/allocator.hxx>
#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <render_system/render_vertex_descriptor.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanVertexDescriptor
    {
    public:
        VulkanVertexDescriptor(
            core::allocator& alloc,
            render::VertexBinding const& binding,
            render::VertexDescriptor const* descriptors,
            uint32_t decriptor_count) noexcept;

        auto binding_description() const noexcept -> VkVertexInputBindingDescription { return _binding_description; }

        void binding_attributes(uint32_t binding, core::pod::Array<VkVertexInputAttributeDescription>& attributes) const noexcept;

    private:
        VkVertexInputBindingDescription _binding_description;
        core::pod::Array<VkVertexInputAttributeDescription> _binding_attributes;
    };

} // namespace render::vulkan
