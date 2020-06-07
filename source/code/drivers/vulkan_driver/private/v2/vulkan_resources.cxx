#include <iceshard/renderer/vulkan/vulkan_resources.hxx>
#include "../vulkan_buffer.hxx"
#include "../vulkan_image.hxx"
#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto native_type_from_render_type(RenderResourceType type) noexcept
        {
            switch (type)
            {
            case iceshard::renderer::RenderResourceType::ResUniform:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case iceshard::renderer::RenderResourceType::ResSampler:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
            case iceshard::renderer::RenderResourceType::ResTexture2D:
                return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            default:
                break;
            }
            IS_ASSERT(false, "Invalid render resource type!");
            std::abort();
        }

        auto select_resource_set(VulkanResourceSet const& resource_set, RenderResourceType type) noexcept
        {
            if (type == RenderResourceType::ResUniform)
            {
                return resource_set.descriptor_sets[0];
            }
            else if (type == RenderResourceType::ResSampler)
            {
                return resource_set.descriptor_sets[1];
            }
            else if (type == RenderResourceType::ResTexture2D)
            {
                return resource_set.descriptor_sets[2];
            }
            IS_ASSERT(false, "Invalid render resource type!");
            std::abort();
        }

    } // namespace detail

    void create_resource_set(
        VkDevice device,
        VulkanFramebuffer framebuffer,
        VulkanResourcePool resource_pool,
        VulkanPipelineLayout pipeline_layout,
        VulkanResourceLayouts const& resource_layouts,
        core::stringid_arg_type name,
        core::pod::Array<RenderResource> const& resources,
        VulkanResourceSet& resource_set
    ) noexcept
    {
        resource_set.name = name;
        resource_set.pipeline_layout = pipeline_layout.layout;

        VkDescriptorSetLayout const descriptor_set_layouts[] = {
            resource_layouts.descriptor_set_uniforms,
            resource_layouts.descriptor_set_samplers,
            resource_layouts.descriptor_set_textures,
        };

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = resource_pool.descriptor_pool;
        alloc_info.descriptorSetCount = core::size(descriptor_set_layouts);
        alloc_info.pSetLayouts = descriptor_set_layouts;

        auto api_result = vkAllocateDescriptorSets(device, &alloc_info, resource_set.descriptor_sets);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor sets!");

        core::memory::stack_allocator_2048 temp_alloc;
        core::pod::Array<VkWriteDescriptorSet> write_descriptor_list{ temp_alloc };
        core::pod::array::reserve(write_descriptor_list, core::pod::array::size(resources));
        core::pod::Array<VkDescriptorImageInfo> write_image_info{ temp_alloc };
        core::pod::Array<VkDescriptorBufferInfo> write_buffer_info{ temp_alloc };

        // Write descriptor set values
        for (RenderResource resource : resources)
        {
            if (resource.type == RenderResourceType::ResSampler)
            {
                continue;
            }

            VkWriteDescriptorSet write_info;
            write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_info.pNext = nullptr;
            write_info.dstSet = detail::select_resource_set(resource_set, resource.type);
            write_info.dstBinding = resource.binding;
            write_info.dstArrayElement = 0;
            write_info.descriptorCount = 1;
            write_info.descriptorType = detail::native_type_from_render_type(resource.type);
            write_info.pBufferInfo = nullptr;
            write_info.pImageInfo = nullptr;

            if (resource.type == RenderResourceType::ResTexture2D)
            {
                VkDescriptorImageInfo image_info;
                image_info.sampler = nullptr;
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                if (resource.handle.texture >= api::v1_1::types::Texture::Attachment3)
                {
                    image_info.imageView = framebuffer_image(framebuffer, framebuffer_texture_from_handle(resource.handle.texture));
                }
                else
                {
                    image_info.imageView = reinterpret_cast<VulkanImage*>(resource.handle.texture)->native_view();
                }

                core::pod::array::push_back(write_image_info, image_info);
                write_info.pImageInfo = &write_image_info[core::pod::array::size(write_image_info) - 1];
            }

            if (resource.type == RenderResourceType::ResUniform)
            {
                auto const& uniform = resource.handle.uniform;
                // #todo: Fix this one!
                auto const* vulkan_buffer = reinterpret_cast<VulkanBuffer*>(uniform.buffer);

                VkDescriptorBufferInfo buffer_info;
                buffer_info.buffer = vulkan_buffer->native_handle();
                buffer_info.offset = uniform.offset;
                buffer_info.range = uniform.range;

                core::pod::array::push_back(write_buffer_info, buffer_info);
                write_info.pBufferInfo = &write_buffer_info[core::pod::array::size(write_buffer_info) - 1];
            }

            core::pod::array::push_back(write_descriptor_list, write_info);
        }

        vkUpdateDescriptorSets(
            device,
            core::pod::array::size(write_descriptor_list),
            core::pod::begin(write_descriptor_list),
            0, nullptr
        );
    }

    void update_resource_set(
        VkDevice device,
        VulkanFramebuffer framebuffer,
        VulkanResourceSet& resource_set,
        core::pod::Array<RenderResource> const& resources
    ) noexcept
    {
        core::memory::stack_allocator_2048 temp_alloc;
        core::pod::Array<VkWriteDescriptorSet> write_descriptor_list{ temp_alloc };
        core::pod::array::reserve(write_descriptor_list, core::pod::array::size(resources));
        core::pod::Array<VkDescriptorImageInfo> write_image_info{ temp_alloc };
        core::pod::Array<VkDescriptorBufferInfo> write_buffer_info{ temp_alloc };

        // Write descriptor set values
        for (RenderResource resource : resources)
        {
            if (resource.type == RenderResourceType::ResSampler)
            {
                continue;
            }

            VkWriteDescriptorSet write_info;
            write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_info.pNext = nullptr;
            write_info.dstSet = detail::select_resource_set(resource_set, resource.type);
            write_info.dstBinding = resource.binding;
            write_info.dstArrayElement = 0;
            write_info.descriptorCount = 1;
            write_info.descriptorType = detail::native_type_from_render_type(resource.type);
            write_info.pBufferInfo = nullptr;
            write_info.pImageInfo = nullptr;

            if (resource.type == RenderResourceType::ResTexture2D)
            {
                VkDescriptorImageInfo image_info;
                image_info.sampler = nullptr;
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                if (resource.handle.texture >= api::v1_1::types::Texture::Attachment3)
                {
                    image_info.imageView = framebuffer_image(framebuffer, framebuffer_texture_from_handle(resource.handle.texture));
                }
                else
                {
                    image_info.imageView = reinterpret_cast<VulkanImage*>(resource.handle.texture)->native_view();
                }

                core::pod::array::push_back(write_image_info, image_info);
                write_info.pImageInfo = &write_image_info[core::pod::array::size(write_image_info) - 1];
            }

            if (resource.type == RenderResourceType::ResUniform)
            {
                auto const& uniform = resource.handle.uniform;
                // #todo: Fix this one!
                auto const* vulkan_buffer = reinterpret_cast<VulkanBuffer*>(uniform.buffer);

                VkDescriptorBufferInfo buffer_info;
                buffer_info.buffer = vulkan_buffer->native_handle();
                buffer_info.offset = uniform.offset;
                buffer_info.range = uniform.range;

                core::pod::array::push_back(write_buffer_info, buffer_info);
                write_info.pBufferInfo = &write_buffer_info[core::pod::array::size(write_buffer_info) - 1];
            }

            core::pod::array::push_back(write_descriptor_list, write_info);
        }

        vkUpdateDescriptorSets(
            device,
            core::pod::array::size(write_descriptor_list),
            core::pod::begin(write_descriptor_list),
            0, nullptr
        );
    }

    void destroy_resource_set(
        VkDevice device,
        VulkanResourcePool resource_pool,
        VulkanResourceSet const& set
    ) noexcept
    {
        vkFreeDescriptorSets(
            device,
            resource_pool.descriptor_pool,
            core::size(set.descriptor_sets),
            set.descriptor_sets
        );
    }

} // namespace iceshard::renderer::vulkan