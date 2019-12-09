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

    VulkanVertexDescriptor::VulkanVertexDescriptor(VulkanVertexDescriptor&& other) noexcept
        : _binding_description{ std::move(other._binding_description) }
        , _binding_attributes{ std::move(other._binding_attributes) }
    {
    }

    auto VulkanVertexDescriptor::operator=(VulkanVertexDescriptor&& other) noexcept -> VulkanVertexDescriptor&
    {
        if (this != &other)
        {
            _binding_description = other._binding_description;
            std::swap(_binding_attributes, other._binding_attributes);
        }
        return *this;
    }

    VulkanVertexDescriptor::VulkanVertexDescriptor(VulkanVertexDescriptor const& other) noexcept
        : _binding_description{ other._binding_description }
        , _binding_attributes{ std}
    {
    }

    auto VulkanVertexDescriptor::operator=(VulkanVertexDescriptor const& other) noexcept -> VulkanVertexDescriptor&
    {
        if (this != &other)
        {
            _binding_description = other._binding_description;
            std::swap(_binding_attributes, other._binding_attributes);
        }
        return *this;
    }

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
        while (descriptor_count > 0)
        {
            descriptor_count -= 1;

            auto& attrib_descriptor = _binding_attributes[descriptor_count];
            attrib_descriptor.binding = 0; // #todo?
            attrib_descriptor.location = descriptors[descriptor_count].descriptor_location;
            attrib_descriptor.format = detail::to_vk_enum(descriptors[descriptor_count].descriptor_type);

            if (descriptors[descriptor_count].descriptor_offset == VertexDescriptorOffset::Automatic)
            {
                attrib_descriptor.offset = attrib_auto_offset;
            }
            else
            {
                attrib_descriptor.offset = static_cast<uint32_t>(descriptors[descriptor_count].descriptor_offset);
                attrib_auto_offset = attrib_descriptor.offset;
            }

            attrib_auto_offset += detail::descriptor_type_size(descriptors[descriptor_count].descriptor_type);
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

    void VulkanVertexDescriptor::binding_attributes(uint32_t binding, core::pod::Array<VkVertexInputAttributeDescription>& attributes) noexcept
    {
        // We copy here the value to easily update the binding value and push back the result.
        for (auto attrib_description : _binding_attributes)
        {
            attrib_description.binding = binding;
            core::pod::array::push_back(attributes, std::move(attrib_description));
        }
    }

} // namespace render::vulkan
