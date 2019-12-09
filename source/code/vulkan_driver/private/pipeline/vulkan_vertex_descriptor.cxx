#include "vulkan_vertex_descriptor.hxx"

namespace render::vulkan
{

    namespace detail
    {

        inline auto to_vk_enum(VertexBindingRate rate) noexcept -> VkVertexInputRate
        {
            switch (rate)
            {
            case render::VertexBindingRate::PerVertex:
                return VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
            case render::VertexBindingRate::PerInstance:
                return VkVertexInputRate::VK_VERTEX_INPUT_RATE_INSTANCE;
            default:
                break;
            }
            std::abort();
        }

        inline auto to_vk_enum(VertexDescriptorType type) noexcept -> VkFormat
        {
            switch (type)
            {
            case render::VertexDescriptorType::Float:
                return VkFormat::VK_FORMAT_R32_SFLOAT;
            case render::VertexDescriptorType::FloatVec2:
                return VkFormat::VK_FORMAT_R32G32_SFLOAT;
            case render::VertexDescriptorType::FloatVec3:
                return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
            case render::VertexDescriptorType::FloatVec4:
                return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;

            default:
                break;
            }
            std::abort();
        }

        inline auto descriptor_type_size(VertexDescriptorType type) noexcept -> uint32_t
        {
            switch (type)
            {
            case render::VertexDescriptorType::Float:
                return sizeof(float);
            case render::VertexDescriptorType::FloatVec2:
                return sizeof(float) * 2;
            case render::VertexDescriptorType::FloatVec3:
                return sizeof(float) * 3;
            case render::VertexDescriptorType::FloatVec4:
                return sizeof(float) * 4;
            default:
                break;
            }
            std::abort();
        }

    } // namespace detail

    VulkanVertexDescriptor::VulkanVertexDescriptor(
        core::allocator& alloc,
        render::VertexBinding const& binding,
        render::VertexDescriptor const* descriptors,
        uint32_t descriptor_count) noexcept
        : _binding_attributes{ alloc }
    {
        core::pod::array::resize(_binding_attributes, descriptor_count);

        // Fill attribute descriptor info
        uint32_t attrib_auto_offset = 0;

        uint32_t attrib_index = 0;
        while (attrib_index < descriptor_count)
        {
            auto& attrib_descriptor = _binding_attributes[attrib_index];
            attrib_descriptor.binding = 0; // #todo?
            attrib_descriptor.location = descriptors[attrib_index].descriptor_location;
            attrib_descriptor.format = detail::to_vk_enum(descriptors[attrib_index].descriptor_type);

            if (descriptors[attrib_index].descriptor_offset == VertexDescriptorOffset::Automatic)
            {
                attrib_descriptor.offset = attrib_auto_offset;
            }
            else
            {
                attrib_descriptor.offset = static_cast<uint32_t>(descriptors[attrib_index].descriptor_offset);
                attrib_auto_offset = attrib_descriptor.offset;
            }

            attrib_auto_offset += detail::descriptor_type_size(descriptors[attrib_index].descriptor_type);
            attrib_index += 1;
        }

        // Fill attribute binding info
        _binding_description.binding = binding.binding_location;
        _binding_description.inputRate = detail::to_vk_enum(binding.binding_rate);

        if (binding.binding_stride == VertexBindingStride::Automatic)
        {
            _binding_description.stride = attrib_auto_offset;
        }
        else
        {
            _binding_description.stride = static_cast<uint32_t>(binding.binding_stride);
        }
    }

    void VulkanVertexDescriptor::binding_attributes(uint32_t binding, core::pod::Array<VkVertexInputAttributeDescription>& attributes) const noexcept
    {
        // We copy here the value to easily update the binding value and push back the result.
        for (auto attrib_description : _binding_attributes)
        {
            attrib_description.binding = binding;
            core::pod::array::push_back(attributes, std::move(attrib_description));
        }
    }

} // namespace render::vulkan
