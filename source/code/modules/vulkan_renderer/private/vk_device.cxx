#include "vk_device.hxx"
#include "vk_swapchain.hxx"
#include "vk_render_surface.hxx"
#include "vk_image.hxx"
#include "vk_buffer.hxx"
#include "vk_utility.hxx"
#include "vk_buffer.hxx"
#include "vk_fence.hxx"

#include <ice/assert.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice::render::vk
{

    auto native_handle(CommandBuffer cmds) noexcept -> VkCommandBuffer;
    auto native_handle(Renderpass renderpass) noexcept -> VkRenderPass;
    auto native_handle(Framebuffer framebuffer) noexcept -> VkFramebuffer;
    auto native_handle(ResourceSetLayout resourceset_layout) noexcept -> VkDescriptorSetLayout;
    auto native_handle(ResourceSet resourceset_layout) noexcept -> VkDescriptorSet;
    auto native_handle(PipelineLayout pipeline_layout) noexcept -> VkPipelineLayout;
    auto native_handle(Pipeline pipeline_layout) noexcept -> VkPipeline;
    auto native_handle(Shader shader) noexcept -> VkShaderModule;
    auto native_handle(Sampler shader) noexcept -> VkSampler;

    VulkanRenderDevice::VulkanRenderDevice(
        ice::Allocator& alloc,
        VkDevice vk_device,
        VkPhysicalDevice vk_physical_device,
        VkPhysicalDeviceMemoryProperties const& memory_properties
    ) noexcept
        : _allocator{ alloc }
        , _gfx_thread_alloc{ _allocator, 1024 * 1024 * 2 }
        , _vk_device{ vk_device }
        , _vk_physical_device{ vk_physical_device }
        , _vk_memory_manager{ ice::make_unique<VulkanMemoryManager>(_allocator, _allocator, _vk_device, memory_properties) }
        , _vk_queues{ _allocator }
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = ice::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkResult result = vkCreateDescriptorPool(
            _vk_device,
            &pool_info,
            nullptr,
            &_vk_descriptor_pool
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to create descriptor pool!"
        );
    }

    VulkanRenderDevice::~VulkanRenderDevice() noexcept
    {
        vkDestroyDescriptorPool(
            _vk_device,
            _vk_descriptor_pool,
            nullptr
        );

        _vk_memory_manager = nullptr;
        vkDestroyDevice(_vk_device, nullptr);
    }

    auto VulkanRenderDevice::create_swapchain(
        ice::render::RenderSurface* surface
    ) noexcept -> ice::render::RenderSwapchain*
    {
        ICE_ASSERT(surface != nullptr, "Cannot create swapchain for nullptr surface!");
        VulkanRenderSurface const* const vk_surface = static_cast<VulkanRenderSurface*>(surface);

        ice::pod::Array<VkSurfaceFormatKHR> surface_formats{ _allocator };
        ice::render::vk::enumerate_objects(
            surface_formats,
            vkGetPhysicalDeviceSurfaceFormatsKHR,
            _vk_physical_device,
            vk_surface->handle()
        );

        ICE_ASSERT(
            ice::pod::array::empty(surface_formats) == false,
            "No supported image formats found for given surface!"
        );

        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            _vk_physical_device,
            vk_surface->handle(),
            &surface_capabilities
        );

        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to query surface capabilities!"
        );

        VkSurfaceFormatKHR surface_format = ice::pod::array::front(surface_formats);

        VkSwapchainCreateInfoKHR swapchain_info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_info.surface = vk_surface->handle();
        swapchain_info.imageFormat = surface_format.format;
        swapchain_info.minImageCount = surface_capabilities.minImageCount;
        swapchain_info.imageExtent = surface_capabilities.maxImageExtent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.oldSwapchain = VK_NULL_HANDLE;
        swapchain_info.clipped = false; // Clipped for android only?
        swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchain_info.imageColorSpace = surface_format.colorSpace;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;

        if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            swapchain_info.preTransform = surface_capabilities.currentTransform;
        }

        // Find a supported composite alpha mode - one of these is guaranteed to be set
        VkCompositeAlphaFlagBitsKHR composite_alpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (uint32_t i = 0; i < sizeof(composite_alpha_flags) / sizeof(composite_alpha_flags[0]); i++)
        {
            if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i])
            {
                swapchain_info.compositeAlpha = composite_alpha_flags[i];
                break;
            }
        } // Guaranteed to be suppoted

        //uint32_t queueFamilyIndices[2] = { (uint32_t)info.graphics_queue_family_index, (uint32_t)info.present_queue_family_index };
        //if (info.graphics_queue_family_index != info.present_queue_family_index)
        //{
        //    // If the graphics and present queues are from different queue families,
        //    // we either have to explicitly transfer ownership of images between the
        //    // queues, or we have to create the swapchain with imageSharingMode
        //    // as VK_SHARING_MODE_CONCURRENT
        //    swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        //    swapchain_ci.queueFamilyIndexCount = 2;
        //    swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
        //}

        VkSwapchainKHR vk_swapchain;
        result = vkCreateSwapchainKHR(
            _vk_device,
            &swapchain_info,
            nullptr,
            &vk_swapchain
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create swapchain!"
        );

        return _allocator.make<VulkanSwapchain>(
            vk_swapchain,
            surface_format.format,
            swapchain_info.imageExtent,
            _vk_device
        );
    }

    void VulkanRenderDevice::destroy_swapchain(ice::render::RenderSwapchain* swapchain) noexcept
    {
        _allocator.destroy(static_cast<VulkanSwapchain*>(swapchain));
    }

    auto VulkanRenderDevice::create_renderpass(ice::render::RenderpassInfo const& info) noexcept -> ice::render::Renderpass
    {
        ice::pod::Array<VkAttachmentDescription> attachments{ _allocator };
        ice::pod::array::reserve(attachments, static_cast<ice::u32>(info.attachments.size()));

        ice::pod::Array<VkSubpassDescription> subpass_list{ _allocator };
        ice::pod::array::reserve(subpass_list, static_cast<ice::u32>(info.subpasses.size()));

        ice::pod::Array<VkSubpassDependency> dependencies { _allocator };
        ice::pod::array::reserve(dependencies, static_cast<ice::u32>(info.dependencies.size()));

        ice::pod::Array<VkAttachmentReference> attachment_references{ _allocator };


        for (RenderAttachment const& attachment_info : info.attachments)
        {
            VkAttachmentDescription attachment{ }; // VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };
            attachment.flags = 0;
            attachment.format = native_enum_value(attachment_info.format);
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;

            for (AttachmentOperation op : attachment_info.operations)
            {
                native_enum_value(op, attachment.loadOp);
                native_enum_value(op, attachment.storeOp);
            }
            for (AttachmentOperation op : attachment_info.stencil_operations)
            {
                native_enum_value(op, attachment.stencilLoadOp);
                native_enum_value(op, attachment.stencilStoreOp);
            }

            attachment.initialLayout = native_enum_value(attachment_info.initial_layout);;
            attachment.finalLayout = native_enum_value(attachment_info.final_layout);

            ice::pod::array::push_back(attachments, attachment);
        }

        ice::u64 reference_count = 0;
        for (RenderSubPass const& subpass_info : info.subpasses)
        {
             reference_count += subpass_info.color_attachments.size()
                + subpass_info.input_attachments.size()
                + 1;

        }
        ice::pod::array::reserve(attachment_references, static_cast<ice::u32>(reference_count));

        auto store_references = [&attachment_references](ice::Span<AttachmentReference const> references) noexcept -> ice::u32
        {
            ice::u32 ref_index = ice::pod::array::size(attachment_references);
            for (AttachmentReference const& attachment_ref : references)
            {
                VkAttachmentReference reference{ }; // VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
                reference.attachment = attachment_ref.attachment_index;
                reference.layout = native_enum_value(attachment_ref.layout);

                ice::pod::array::push_back(attachment_references, reference);
            }
            return ref_index;
        };

        for (RenderSubPass const& subpass_info : info.subpasses)
        {
            ice::u32 const input_ref_idx = store_references(subpass_info.input_attachments);
            ice::u32 const color_ref_idx = store_references(subpass_info.color_attachments);

            VkSubpassDescription subpass{ }; // VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2 };
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = static_cast<ice::u32>(subpass_info.input_attachments.size());
            if (subpass.inputAttachmentCount > 0)
            {
                subpass.pInputAttachments = std::addressof(attachment_references[input_ref_idx]);
            }
            subpass.colorAttachmentCount = static_cast<ice::u32>(subpass_info.color_attachments.size());
            if (subpass.colorAttachmentCount > 0)
            {
                subpass.pColorAttachments = std::addressof(attachment_references[color_ref_idx]);
            }
            if (subpass_info.depth_stencil_attachment.layout != ImageLayout::Undefined)
            {
                ice::u32 const depth_ref_idx = store_references(ice::Span<AttachmentReference const>(&subpass_info.depth_stencil_attachment, 1llu));
                subpass.pDepthStencilAttachment = std::addressof(attachment_references[depth_ref_idx]);
            }

            ice::pod::array::push_back(subpass_list, subpass);
        }

        for (SubpassDependency const& dependency_info : info.dependencies)
        {
            VkSubpassDependency dependency{ VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2 };
            dependency.srcSubpass = dependency_info.source_subpass;
            dependency.srcStageMask = native_enum_value(dependency_info.source_stage);
            dependency.srcAccessMask = native_enum_value(dependency_info.source_access);
            dependency.dstSubpass = dependency_info.destination_subpass;
            dependency.dstStageMask = native_enum_value(dependency_info.destination_stage);
            dependency.dstAccessMask = native_enum_value(dependency_info.destination_access);
            dependency.dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;

            ice::pod::array::push_back(dependencies, dependency);
        }

        VkRenderPassCreateInfo renderpass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderpass_info.attachmentCount = ice::pod::array::size(attachments);
        renderpass_info.pAttachments = ice::pod::array::begin(attachments);
        renderpass_info.subpassCount = ice::pod::array::size(subpass_list);
        renderpass_info.pSubpasses = ice::pod::array::begin(subpass_list);
        renderpass_info.dependencyCount = ice::pod::array::size(dependencies);
        renderpass_info.pDependencies = ice::pod::array::begin(dependencies);

        VkRenderPass renderpass;
        VkResult result = vkCreateRenderPass(_vk_device, &renderpass_info, nullptr, &renderpass);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create render pass with given description!"
        );

        return static_cast<Renderpass>(reinterpret_cast<ice::uptr>(renderpass));
    }

    void VulkanRenderDevice::destroy_renderpass(ice::render::Renderpass renderpass) noexcept
    {
        vkDestroyRenderPass(
            _vk_device,
            native_handle(renderpass),
            nullptr
        );
    }

    auto VulkanRenderDevice::create_resourceset_layout(
        ice::Span<ice::render::ResourceSetLayoutBinding const> bindings
    ) noexcept -> ice::render::ResourceSetLayout
    {
        ice::pod::Array<VkDescriptorSetLayoutBinding> vk_bindings{ _allocator };
        ice::pod::array::reserve(vk_bindings, ice::size(bindings));

        for (ResourceSetLayoutBinding const& binding : bindings)
        {
            ice::pod::array::push_back(
                vk_bindings,
                VkDescriptorSetLayoutBinding{
                    .binding = binding.binding_index,
                    .descriptorType = native_enum_value(binding.resource_type),
                    .descriptorCount = binding.resource_count,
                    .stageFlags = native_enum_flags(binding.shader_stage_flags),
                    .pImmutableSamplers = nullptr
                }
            );
        }

        VkDescriptorSetLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = ice::size(bindings);
        layout_info.pBindings = ice::pod::begin(vk_bindings);

        VkDescriptorSetLayout vk_descriptor_set_layout = vk_nullptr;
        VkResult result = vkCreateDescriptorSetLayout(
            _vk_device,
            &layout_info,
            nullptr,
            &vk_descriptor_set_layout
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create `DescriptorSetLayout` with given bindings!"
        );

        return static_cast<ResourceSetLayout>(reinterpret_cast<ice::uptr>(vk_descriptor_set_layout));
    }

    void VulkanRenderDevice::destroy_resourceset_layout(
        ice::render::ResourceSetLayout resourceset_layout
    ) noexcept
    {
        vkDestroyDescriptorSetLayout(
            _vk_device,
            native_handle(resourceset_layout),
            nullptr
        );
    }

    bool VulkanRenderDevice::create_resourcesets(
        ice::Span<ice::render::ResourceSetLayout const> resource_set_layouts,
        ice::Span<ice::render::ResourceSet> resource_sets_out
    ) noexcept
    {
        static_assert(
            sizeof(ice::render::ResourceSet)
            ==
            sizeof(VkDescriptorSet),
            "Descriptor set handle has a different size than resource set handle!"
        );

        ICE_ASSERT(
            ice::size(resource_set_layouts) == ice::size(resource_sets_out),
            "The output span size does not match the size of provided layouts span."
        );

        //ice::pod::array::resize(resource_sets_out, ice::size(resource_set_layouts));

        VkDescriptorSetAllocateInfo descriptorset_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        descriptorset_info.descriptorPool = _vk_descriptor_pool;
        descriptorset_info.descriptorSetCount = ice::size(resource_set_layouts);
        descriptorset_info.pSetLayouts = reinterpret_cast<VkDescriptorSetLayout const*>(resource_set_layouts.data());

        VkResult result = vkAllocateDescriptorSets(
            _vk_device,
            &descriptorset_info,
            reinterpret_cast<VkDescriptorSet*>(resource_sets_out.data())
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't allocate new {} descriptor sets.",
            ice::size(resource_set_layouts)
        );
        return true;
    }

    void VulkanRenderDevice::destroy_resourcesets(
        ice::Span<ice::render::ResourceSet const> resource_sets
    ) noexcept
    {
        VkResult result = vkFreeDescriptorSets(
            _vk_device,
            _vk_descriptor_pool,
            ice::size(resource_sets),
            reinterpret_cast<VkDescriptorSet const*>(resource_sets.data())
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to free given {} descriptor sets.",
            ice::size(resource_sets)
        );
    }

    void VulkanRenderDevice::update_resourceset(
        ice::Span<ice::render::ResourceSetUpdateInfo const> update_infos
    ) noexcept
    {
        ice::pod::Array<VkWriteDescriptorSet> vk_writes{ _allocator };
        ice::pod::array::reserve(vk_writes, ice::size(update_infos));

        ice::pod::Array<VkDescriptorImageInfo> write_image_info{ _allocator };
        ice::pod::Array<VkDescriptorBufferInfo> write_buffer_info{ _allocator };

        for (ResourceSetUpdateInfo const& update_info : update_infos)
        {
            VkWriteDescriptorSet descriptor_set_write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            descriptor_set_write.dstBinding = update_info.binding_index;
            descriptor_set_write.dstArrayElement = update_info.array_element;
            descriptor_set_write.dstSet = native_handle(update_info.resource_set);
            descriptor_set_write.descriptorCount = ice::size(update_info.resources);
            descriptor_set_write.descriptorType = native_enum_value(update_info.resource_type);

            if (update_info.resource_type == ResourceType::SampledImage || update_info.resource_type == ResourceType::InputAttachment)
            {
                ice::u32 const image_info_index = ice::pod::array::size(write_image_info);

                for (ResourceUpdateInfo const& resource_info : update_info.resources)
                {
                    VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(
                        static_cast<ice::uptr>(resource_info.image)
                    );

                    VkDescriptorImageInfo image_info;
                    image_info.sampler = nullptr;
                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_info.imageView = image_ptr->vk_image_view;

                    ice::pod::array::push_back(write_image_info, image_info);
                }

                descriptor_set_write.pImageInfo = ice::addressof(write_image_info[image_info_index]);
            }

            if (update_info.resource_type == ResourceType::Sampler)
            {
                ice::u32 const image_info_index = ice::pod::array::size(write_image_info);

                for (ResourceUpdateInfo const& resource_info : update_info.resources)
                {
                    VkSampler vk_sampler = native_handle(resource_info.sampler);

                    VkDescriptorImageInfo sampler_info;
                    sampler_info.sampler = vk_sampler;

                    ice::pod::array::push_back(write_image_info, sampler_info);
                }

                descriptor_set_write.pImageInfo = ice::addressof(write_image_info[image_info_index]);
            }

            if (update_info.resource_type == ResourceType::UniformBuffer)
            {
                ice::u32 const buffer_info_index = ice::pod::array::size(write_buffer_info);

                for (ResourceUpdateInfo const& resource_info : update_info.resources)
                {
                    VulkanBuffer* const buffer_ptr = reinterpret_cast<VulkanBuffer*>(
                        static_cast<ice::uptr>(resource_info.uniform_buffer.buffer)
                    );

                    VkDescriptorBufferInfo buffer_info;
                    buffer_info.buffer = buffer_ptr->vk_buffer;
                    buffer_info.offset = resource_info.uniform_buffer.offset;
                    buffer_info.range = resource_info.uniform_buffer.size;

                    ice::pod::array::push_back(write_buffer_info, buffer_info);
                }

                descriptor_set_write.pBufferInfo = ice::addressof(write_buffer_info[buffer_info_index]);
            }

            ice::pod::array::push_back(vk_writes, descriptor_set_write);
        }

        vkUpdateDescriptorSets(
            _vk_device,
            ice::size(vk_writes),
            ice::pod::begin(vk_writes),
            0, nullptr
        );
    }

    auto VulkanRenderDevice::create_pipeline_layout(
        ice::render::PipelineLayoutInfo const& info
    ) noexcept -> ice::render::PipelineLayout
    {
        ice::pod::Array<VkPushConstantRange> vk_push_constants{ _allocator };
        ice::pod::array::reserve(vk_push_constants, ice::size(info.push_constants));

        ice::pod::Array<VkDescriptorSetLayout> vk_descriptorset_layouts{ _allocator };
        ice::pod::array::reserve(vk_descriptorset_layouts, ice::size(info.resource_layouts));

        for (PipelinePushConstant const& push_constant : info.push_constants)
        {
            ice::pod::array::push_back(
                vk_push_constants,
                VkPushConstantRange{
                    .stageFlags = native_enum_flags(push_constant.shader_stage_flags),
                    .offset = push_constant.offset,
                    .size = push_constant.size,
                }
            );
        }

        for (ResourceSetLayout layout : info.resource_layouts)
        {
            ice::pod::array::push_back(
                vk_descriptorset_layouts,
                native_handle(layout)
            );
        }

        VkPipelineLayoutCreateInfo pipeline_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_info.pushConstantRangeCount = ice::size(info.push_constants);
        pipeline_info.pPushConstantRanges = ice::pod::begin(vk_push_constants);
        pipeline_info.setLayoutCount = ice::size(info.resource_layouts);
        pipeline_info.pSetLayouts = ice::pod::begin(vk_descriptorset_layouts);

        VkPipelineLayout pipeline_layout = vk_nullptr;
        VkResult result = vkCreatePipelineLayout(
            _vk_device,
            &pipeline_info,
            nullptr,
            &pipeline_layout
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create `PieplineLayout` with given push constants and resource layouts!"
        );

        return static_cast<ice::render::PipelineLayout>(reinterpret_cast<ice::uptr>(pipeline_layout));
    }

    void VulkanRenderDevice::destroy_pipeline_layout(
        ice::render::PipelineLayout pipeline_layout
    ) noexcept
    {
        vkDestroyPipelineLayout(
            _vk_device,
            native_handle(pipeline_layout),
            nullptr
        );
    }

    auto VulkanRenderDevice::create_shader(
        ice::render::ShaderInfo const& info
    ) noexcept -> ice::render::Shader
    {
        VkShaderModuleCreateInfo shader_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shader_info.codeSize = info.shader_data.size;
        shader_info.pCode = reinterpret_cast<ice::u32 const*>(info.shader_data.location);

        VkShaderModule vk_shader;
        VkResult result = vkCreateShaderModule(
            _vk_device,
            &shader_info,
            nullptr,
            &vk_shader
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create Shader Module with given data!"
        );

        return static_cast<ice::render::Shader>(reinterpret_cast<ice::uptr>(vk_shader));
    }

    void VulkanRenderDevice::destroy_shader(
        ice::render::Shader shader
    ) noexcept
    {
        vkDestroyShaderModule(
            _vk_device,
            native_handle(shader),
            nullptr
        );
    }

    auto VulkanRenderDevice::create_pipeline(
        ice::render::PipelineInfo const& info
    ) noexcept -> ice::render::Pipeline
    {
        VkPipelineShaderStageCreateInfo shader_stages[10];
        ice::u32 const stage_count = ice::size(info.shaders_stages);

        uint32_t stage_idx = 0;
        for (; stage_idx < stage_count; ++stage_idx)
        {
            VkPipelineShaderStageCreateInfo& shader_stage = shader_stages[stage_idx];
            shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage.pNext = nullptr;
            shader_stage.pSpecializationInfo = nullptr;
            shader_stage.flags = 0;
            shader_stage.stage = native_enum_value(info.shaders_stages[stage_idx]);
            shader_stage.module = native_handle(info.shaders[stage_idx]);
            shader_stage.pName = "main";
        }

        VkDynamicState dynamic_states[2]; // max: VK_DYNAMIC_STATE_RANGE_SIZE
        dynamic_states[0] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT;
        dynamic_states[1] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR;

        VkPipelineDynamicStateCreateInfo dynamic_state{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.pDynamicStates = dynamic_states;
        dynamic_state.dynamicStateCount = ice::size(dynamic_states);

        ice::pod::Array<VkVertexInputBindingDescription> vertex_input_bindings{ _allocator };
        ice::pod::Array<VkVertexInputAttributeDescription> vertex_input_attributes{ _allocator };

        ice::pod::array::reserve(vertex_input_bindings, ice::size(info.shader_bindings));
        ice::pod::array::reserve(vertex_input_attributes, ice::size(info.shader_bindings) * 4);

        for (ice::render::ShaderInputBinding const& binding : info.shader_bindings)
        {
            VkVertexInputBindingDescription vk_binding{ };
            vk_binding.binding = binding.binding;
            vk_binding.stride = binding.stride;
            vk_binding.inputRate = static_cast<VkVertexInputRate>(binding.instanced);
            ice::pod::array::push_back(vertex_input_bindings, vk_binding);

            for (ice::render::ShaderInputAttribute const& attrib : binding.attributes)
            {
                VkVertexInputAttributeDescription vk_attrib{ };
                vk_attrib.format = native_enum_value(attrib.type);
                vk_attrib.location = attrib.location;
                vk_attrib.offset = attrib.offset;
                vk_attrib.binding = binding.binding;
                ice::pod::array::push_back(vertex_input_attributes, vk_attrib);
            }
        }

        VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex_input.vertexBindingDescriptionCount = ice::pod::array::size(vertex_input_bindings);
        vertex_input.pVertexBindingDescriptions = ice::pod::array::begin(vertex_input_bindings);
        vertex_input.vertexAttributeDescriptionCount = ice::pod::array::size(vertex_input_attributes);
        vertex_input.pVertexAttributeDescriptions = ice::pod::array::begin(vertex_input_attributes);

        VkPipelineInputAssemblyStateCreateInfo input_assembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        input_assembly.primitiveRestartEnable = VK_FALSE;
        input_assembly.topology = native_enum_value(info.primitive_topology);

        VkPipelineRasterizationStateCreateInfo rasterization{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        switch (info.cull_mode)
        {
        case CullMode::Disabled:
            rasterization.cullMode = VK_CULL_MODE_NONE;
            // [issue #34] Needs to be properly available in the creation API.
            if (ice::size(info.shaders_stages) == 5)
            {
                rasterization.polygonMode = VK_POLYGON_MODE_LINE;
            }
            break;
        case CullMode::FrontFace:
            rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case CullMode::BackFace:
            rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        }
        switch (info.front_face)
        {
        case FrontFace::ClockWise:
            rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
            break;
        case FrontFace::CounterClockWise:
            rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            break;
        }
        rasterization.depthClampEnable = VK_FALSE;
        rasterization.rasterizerDiscardEnable = VK_FALSE;
        rasterization.depthBiasEnable = VK_FALSE;
        rasterization.depthBiasConstantFactor = 0;
        rasterization.depthBiasClamp = 0;
        rasterization.depthBiasSlopeFactor = 0;
        rasterization.lineWidth = 1.0f;

        VkPipelineColorBlendStateCreateInfo blend_info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

        VkPipelineColorBlendAttachmentState attachment_state[1];
        attachment_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_state[0].blendEnable = VK_TRUE;
        attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        blend_info.attachmentCount = 1;
        blend_info.pAttachments = attachment_state;
        blend_info.logicOpEnable = VK_FALSE;
        blend_info.logicOp = VK_LOGIC_OP_NO_OP;
        blend_info.blendConstants[0] = 1.0f;
        blend_info.blendConstants[1] = 1.0f;
        blend_info.blendConstants[2] = 1.0f;
        blend_info.blendConstants[3] = 1.0f;

        VkPipelineDepthStencilStateCreateInfo depthstencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthstencil.depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE;
        depthstencil.depthWriteEnable = VK_TRUE;
        depthstencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthstencil.depthBoundsTestEnable = VK_FALSE;
        depthstencil.minDepthBounds = 0;
        depthstencil.maxDepthBounds = 0;
        depthstencil.stencilTestEnable = VK_FALSE;
        depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
        depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
        depthstencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthstencil.back.compareMask = 0;
        depthstencil.back.reference = 0;
        depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depthstencil.back.writeMask = 0;
        depthstencil.front = depthstencil.back;

        VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample.pSampleMask = nullptr;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.alphaToCoverageEnable = VK_FALSE;
        multisample.alphaToOneEnable = VK_FALSE;
        multisample.minSampleShading = 0.0;

        VkPipelineViewportStateCreateInfo viewport{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        viewport.pScissors = vk_nullptr;
        viewport.pViewports = vk_nullptr;

        VkPipelineTessellationStateCreateInfo tesselation{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
        tesselation.patchControlPoints = 4;

        VkGraphicsPipelineCreateInfo pipeline_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_info.layout = native_handle(info.layout);
        pipeline_info.basePipelineHandle = vk_nullptr;
        pipeline_info.basePipelineIndex = 0;
        pipeline_info.flags = 0;
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pRasterizationState = &rasterization;
        pipeline_info.pColorBlendState = &blend_info;
        pipeline_info.pTessellationState = &tesselation;
        pipeline_info.pMultisampleState = &multisample;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.pViewportState = &viewport;
        pipeline_info.pDepthStencilState = &depthstencil;
        pipeline_info.pStages = shader_stages;
        pipeline_info.stageCount = stage_count;
        pipeline_info.renderPass = native_handle(info.renderpass);
        pipeline_info.subpass = info.subpass_index;

        VkPipeline vk_pipeline;
        VkResult result = vkCreateGraphicsPipelines(
            _vk_device,
            vk_nullptr,
            1,
            &pipeline_info,
            nullptr,
            &vk_pipeline
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create graphics pipeline!"
        );

        return static_cast<Pipeline>(reinterpret_cast<ice::uptr>(vk_pipeline));
    }

    void VulkanRenderDevice::destroy_pipeline(
        ice::render::Pipeline pipeline
    ) noexcept
    {
        vkDestroyPipeline(
            _vk_device,
            native_handle(pipeline),
            nullptr
        );
    }

    auto VulkanRenderDevice::create_buffer(
        ice::render::BufferType buffer_type,
        ice::u32 buffer_size
    ) noexcept -> ice::render::Buffer
    {
        VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.usage = native_enum_value(buffer_type);
        buffer_info.size = buffer_size;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer vk_buffer = vk_nullptr;
        VkResult result = vkCreateBuffer(
            _vk_device,
            &buffer_info,
            nullptr,
            &vk_buffer
        );

        AllocationInfo memory_info;
        AllocationHandle memory_handle = _vk_memory_manager->allocate(
            vk_buffer,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            BufferType::Transfer == buffer_type ? AllocationType::TransferBuffer : AllocationType::Buffer,
            memory_info
        );

        VulkanBuffer* buffer_ptr = _allocator.make<VulkanBuffer>(vk_buffer, memory_handle);
        return static_cast<Buffer>(reinterpret_cast<ice::uptr>(buffer_ptr));
    }

    void VulkanRenderDevice::destroy_buffer(
        ice::render::Buffer buffer
    ) noexcept
    {
        VulkanBuffer* const buffer_ptr = reinterpret_cast<VulkanBuffer*>(static_cast<ice::uptr>(buffer));
        vkDestroyBuffer(
            _vk_device,
            buffer_ptr->vk_buffer,
            nullptr
        );
        _vk_memory_manager->release(buffer_ptr->vk_alloc_handle);
        _allocator.destroy(buffer_ptr);
    }

    void VulkanRenderDevice::update_buffers(
        ice::Span<ice::render::BufferUpdateInfo const> update_infos
    ) noexcept
    {
        ice::u32 const update_count = ice::size(update_infos);

        // [issue #49]: This part of the code is generally run on a graphics thread. Because of this we need to have a dedicated memory pool / allocator. So it won't interfer with
        //  other allocations done in the mean time on any other thrads.
        //  We should probably slowly invest some time into thread safe allocators.
        ice::pod::Array<AllocationHandle> allocation_handles{ _gfx_thread_alloc };
        ice::pod::array::reserve(allocation_handles, update_count);

        for (BufferUpdateInfo const& update_info : update_infos)
        {
            VulkanBuffer* const buffer_ptr = reinterpret_cast<VulkanBuffer*>(static_cast<ice::uptr>(update_info.buffer));
            ice::pod::array::push_back(
                allocation_handles,
                buffer_ptr->vk_alloc_handle
            );
        }

        // [issue #49]: This part of the code is generally run on a graphics thread. Because of this we need to have a dedicated memory pool / allocator. So it won't interfer with
        //  other allocations done in the mean time on any other thrads.
        //  We should probably slowly invest some time into thread safe allocators.
        ice::pod::Array<ice::Memory> data_blocks{ _gfx_thread_alloc };
        ice::pod::array::resize(data_blocks, update_count);

        _vk_memory_manager->map_memory(
            allocation_handles,
            data_blocks
        );

        for (ice::u32 idx = 0; idx < update_count; ++idx)
        {
            BufferUpdateInfo const& update_info = update_infos[idx];
            Memory& target = data_blocks[idx];

            ICE_ASSERT(
                target.size >= update_info.data.size,
                "Target memory buffer is smaller than provided size to copy!"
            );

            ice::memcpy(
                ice::memory::ptr_add(target.location, update_info.offset),
                update_info.data.location,
                update_info.data.size
            );
        }

        _vk_memory_manager->unmap_memory(
            allocation_handles
        );
    }

    auto VulkanRenderDevice::create_framebuffer(
        ice::vec2u extent,
        ice::render::Renderpass renderpass,
        ice::Span<ice::render::Image const> images
    ) noexcept -> ice::render::Framebuffer
    {
        VkRenderPass vk_renderpass = reinterpret_cast<VkRenderPass>(static_cast<ice::uptr>(renderpass));

        ice::pod::Array<VkImageView> vk_images{ _allocator };
        ice::pod::array::reserve(vk_images, static_cast<ice::u32>(images.size()));

        for (Image image : images)
        {
            VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));
            ice::pod::array::push_back(vk_images, image_ptr->vk_image_view);
        }

        VkFramebufferCreateInfo fb_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fb_info.renderPass = vk_renderpass;
        fb_info.attachmentCount = ice::pod::array::size(vk_images);
        fb_info.pAttachments = ice::pod::array::begin(vk_images);
        fb_info.width = extent.x;
        fb_info.height = extent.y;
        fb_info.layers = 1;

        VkFramebuffer vk_framebuffer;
        VkResult result = vkCreateFramebuffer(_vk_device, &fb_info, nullptr, &vk_framebuffer);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create framebuffer with given images!"
        );

        return static_cast<Framebuffer>(reinterpret_cast<ice::uptr>(vk_framebuffer));
    }

    void VulkanRenderDevice::destroy_framebuffer(
        ice::render::Framebuffer framebuffer
    ) noexcept
    {
        VkFramebuffer vk_framebuffer = reinterpret_cast<VkFramebuffer>(static_cast<ice::uptr>(framebuffer));
        vkDestroyFramebuffer(_vk_device, vk_framebuffer, nullptr);
    }

    auto VulkanRenderDevice::create_image(
        ice::render::ImageInfo const& image,
        ice::Data data
    ) noexcept -> ice::render::Image
    {
        VkImageCreateInfo image_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = native_enum_value(image.format);
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.extent.width = image.width;
        image_info.extent.height = image.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = native_enum_flags(image.usage);
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkImage vk_image;
        VkResult result = vkCreateImage(_vk_device, &image_info, nullptr, &vk_image);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create image object!"
        );

        AllocationType alloc_type = AllocationType::Implicit;
        if (has_flag(image.usage, ImageUsageFlags::ColorAttachment) || has_flag(image.usage, ImageUsageFlags::DepthStencilAttachment))
        {
            alloc_type = AllocationType::RenderTarget;
        }

        AllocationInfo memory_info;
        AllocationHandle memory_handle = _vk_memory_manager->allocate(
            vk_image,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            alloc_type,
            memory_info
        );

        VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = vk_image;
        view_info.format = image_info.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_R;
        view_info.components.g = VK_COMPONENT_SWIZZLE_G;
        view_info.components.b = VK_COMPONENT_SWIZZLE_B;
        view_info.components.a = VK_COMPONENT_SWIZZLE_A;
        if (image_info.format == VK_FORMAT_D16_UNORM
            || image_info.format == VK_FORMAT_D32_SFLOAT
            || image_info.format == VK_FORMAT_D16_UNORM_S8_UINT
            || image_info.format == VK_FORMAT_D24_UNORM_S8_UINT
            || image_info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        VkImageView vk_image_view;
        result = vkCreateImageView(_vk_device, &view_info, nullptr, &vk_image_view);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Coudln't create image view!"
        );

        VulkanImage* const image_ptr = _allocator.make<VulkanImage>(vk_image, vk_image_view, memory_handle);
        return static_cast<Image>(reinterpret_cast<ice::uptr>(image_ptr));
    }

    void VulkanRenderDevice::destroy_image(ice::render::Image image) noexcept
    {
        VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));
        vkDestroyImageView(_vk_device, image_ptr->vk_image_view, nullptr);
        _vk_memory_manager->release(image_ptr->vk_alloc_handle);
        vkDestroyImage(_vk_device, image_ptr->vk_image, nullptr);
        _allocator.destroy(image_ptr);
    }

    /*auto VulkanRenderDevice::queue_count() const noexcept -> ice::u32
    {
        return ice::pod::array::size(_vk_queues);
    }*/

    auto VulkanRenderDevice::create_queue(
        ice::render::QueueID queue_id,
        ice::u32 queue_index,
        ice::u32 command_pools
    ) const noexcept -> ice::render::RenderQueue*
    {
        ice::u32 const queue_family_index = static_cast<ice::u32>(queue_id);

        VkQueue queue = vk_nullptr;
        vkGetDeviceQueue(_vk_device, queue_family_index, queue_index, &queue);

        ice::pod::Array<VkCommandPool> cmd_pools{ _allocator };
        ice::pod::array::reserve(cmd_pools, command_pools);

        for (ice::u32 idx = 0; idx < command_pools; ++idx)
        {
            VkCommandPoolCreateInfo cmd_pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            cmd_pool_info.queueFamilyIndex = queue_family_index;
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            VkCommandPool vk_cmd_pool;
            VkResult result = vkCreateCommandPool(_vk_device, &cmd_pool_info, nullptr, &vk_cmd_pool);
            ICE_ASSERT(
                result == VkResult::VK_SUCCESS,
                "Failed to create command pool for device!"
            );

            ice::pod::array::push_back(cmd_pools, vk_cmd_pool);
        }

        return _allocator.make<VulkanQueue>(queue, _vk_device, ice::move(cmd_pools));
    }

    void VulkanRenderDevice::destroy_queue(ice::render::RenderQueue* queue) const noexcept
    {
        _allocator.destroy(static_cast<VulkanQueue*>(queue));
    }

    auto VulkanRenderDevice::create_fence() noexcept -> ice::render::RenderFence*
    {
        VkFenceCreateInfo fence_info{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        VkFence fence;
        VkResult result = vkCreateFence(_vk_device, &fence_info, nullptr, &fence);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to create fence for device!"
        );

        return _allocator.make<VulkanFence>(_vk_device, fence);
    }

    void VulkanRenderDevice::destroy_fence(ice::render::RenderFence* fence) noexcept
    {
        _allocator.destroy(static_cast<VulkanFence*>(fence));
    }

    auto VulkanRenderDevice::get_commands() noexcept -> ice::render::RenderCommands&
    {
        return _vk_render_commands;
    }

    void VulkanRenderDevice::wait_idle() const noexcept
    {
        vkDeviceWaitIdle(_vk_device);
    }

    auto VulkanRenderDevice::create_sampler(
        ice::render::SamplerInfo const& sampler_info
    ) noexcept -> ice::render::Sampler
    {
        VkSamplerCreateInfo vk_sampler_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        vk_sampler_info.minFilter = native_enum_value(sampler_info.min_filter);
        vk_sampler_info.magFilter = native_enum_value(sampler_info.mag_filter);
        vk_sampler_info.addressModeU = native_enum_value(sampler_info.address_mode.u);
        vk_sampler_info.addressModeV = native_enum_value(sampler_info.address_mode.v);
        vk_sampler_info.addressModeW = native_enum_value(sampler_info.address_mode.w);
        vk_sampler_info.anisotropyEnable = VK_FALSE;
        vk_sampler_info.maxAnisotropy = 1;
        vk_sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        vk_sampler_info.unnormalizedCoordinates = sampler_info.normalized_coordinates == false ? VK_TRUE : VK_FALSE; // Care, the target member is negated!
        vk_sampler_info.compareEnable = VK_FALSE;
        vk_sampler_info.compareOp = VK_COMPARE_OP_LESS;
        vk_sampler_info.mipmapMode = native_enum_value(sampler_info.mip_map_mode);
        vk_sampler_info.mipLodBias = 0.0f;
        vk_sampler_info.minLod = 0.0f;
        vk_sampler_info.maxLod = 0.0f;

        VkSampler vk_sampler = vk_nullptr;
        VkResult result = vkCreateSampler(
            _vk_device,
            &vk_sampler_info,
            nullptr,
            &vk_sampler
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to create sampler!"
        );

        return static_cast<ice::render::Sampler>(reinterpret_cast<ice::uptr>(vk_sampler));
    }

    void VulkanRenderDevice::destroy_sampler(
        ice::render::Sampler sampler
    ) noexcept
    {
        vkDestroySampler(
            _vk_device,
            native_handle(sampler),
            nullptr
        );
    }

    auto native_handle(CommandBuffer cmds) noexcept -> VkCommandBuffer
    {
        return reinterpret_cast<VkCommandBuffer>(static_cast<ice::uptr>(cmds));
    }

    auto native_handle(Buffer buffer) noexcept -> VkBuffer
    {
        return reinterpret_cast<VulkanBuffer*>(static_cast<ice::uptr>(buffer))->vk_buffer;
    }

    auto native_handle(Renderpass renderpass) noexcept -> VkRenderPass
    {
        return reinterpret_cast<VkRenderPass>(static_cast<ice::uptr>(renderpass));
    }

    auto native_handle(Framebuffer framebuffer) noexcept -> VkFramebuffer
    {
        return reinterpret_cast<VkFramebuffer>(static_cast<ice::uptr>(framebuffer));
    }

    auto native_handle(ResourceSetLayout resourceset_layout) noexcept -> VkDescriptorSetLayout
    {
        return reinterpret_cast<VkDescriptorSetLayout>(static_cast<ice::uptr>(resourceset_layout));
    }

    auto native_handle(ResourceSet resourceset) noexcept -> VkDescriptorSet
    {
        return reinterpret_cast<VkDescriptorSet>(static_cast<ice::uptr>(resourceset));
    }

    auto native_handle(PipelineLayout pipeline_layout) noexcept -> VkPipelineLayout
    {
        return reinterpret_cast<VkPipelineLayout>(static_cast<ice::uptr>(pipeline_layout));
    }

    auto native_handle(Pipeline pipeline) noexcept -> VkPipeline
    {
        return reinterpret_cast<VkPipeline>(static_cast<ice::uptr>(pipeline));
    }

    auto native_handle(Shader shader) noexcept -> VkShaderModule
    {
        return reinterpret_cast<VkShaderModule>(static_cast<ice::uptr>(shader));
    }

    auto native_handle(Sampler shader) noexcept -> VkSampler
    {
        return reinterpret_cast<VkSampler>(static_cast<ice::uptr>(shader));
    }

    void VulkanRenderCommands::begin(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(
            native_handle(cmds),
            &begin_info
        );
    }

    void VulkanRenderCommands::begin_renderpass(
        ice::render::CommandBuffer cmds,
        ice::render::Renderpass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::vec2u extent,
        ice::vec4f clear_color
    ) noexcept
    {
        VkClearValue clear_values[3];
        clear_values[0].color.float32[0] = clear_color.x;
        clear_values[0].color.float32[1] = clear_color.y;
        clear_values[0].color.float32[2] = clear_color.z;
        clear_values[0].color.float32[3] = clear_color.w;
        //clear_values[1] = clear_values[0];
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        begin_info.renderPass = native_handle(renderpass);
        begin_info.framebuffer = native_handle(framebuffer);
        begin_info.renderArea.offset.x = 0;
        begin_info.renderArea.offset.y = 0;
        begin_info.renderArea.extent = { extent.x, extent.y };
        begin_info.clearValueCount = 2;
        begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(
            native_handle(cmds),
            &begin_info,
            VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE
        );
    }

    void VulkanRenderCommands::set_viewport(
        ice::render::CommandBuffer cmds,
        ice::vec4u viewport_rect
    ) noexcept
    {
        VkViewport viewport{ };
        viewport.x = viewport_rect.x;
        viewport.y = viewport_rect.y;
        viewport.width = viewport_rect.z;
        viewport.height = viewport_rect.w;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(
            native_handle(cmds),
            0, 1,
            &viewport
        );
    }

    void VulkanRenderCommands::set_scissor(
        ice::render::CommandBuffer cmds,
        ice::vec4u scissor_rect
    ) noexcept
    {
        VkRect2D scissor{ };
        scissor.offset.x = scissor_rect.x;
        scissor.offset.y = scissor_rect.y;
        scissor.extent.width = scissor_rect.z;
        scissor.extent.height = scissor_rect.w;

        vkCmdSetScissor(
            native_handle(cmds),
            0, 1,
            &scissor
        );
    }

    void VulkanRenderCommands::bind_pipeline(
        ice::render::CommandBuffer cmds,
        ice::render::Pipeline pipeline
    ) noexcept
    {
        vkCmdBindPipeline(
            native_handle(cmds),
            VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
            native_handle(pipeline)
        );
    }

    void VulkanRenderCommands::bind_resource_set(
        ice::render::CommandBuffer cmds,
        ice::render::PipelineLayout pipeline_layout,
        ice::render::ResourceSet resource_set,
        ice::u32 first_set
    ) noexcept
    {
        VkDescriptorSet descriptor_set = native_handle(resource_set);

        vkCmdBindDescriptorSets(
            native_handle(cmds),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            native_handle(pipeline_layout),
            first_set, 1,
            &descriptor_set,
            0, nullptr
        );
    }

    void VulkanRenderCommands::bind_index_buffer(
        ice::render::CommandBuffer cmds,
        ice::render::Buffer buffer
    ) noexcept
    {
        vkCmdBindIndexBuffer(
            native_handle(cmds),
            native_handle(buffer),
            0,
            VkIndexType::VK_INDEX_TYPE_UINT16
        );
    }

    void VulkanRenderCommands::begin_renderpass(
        ice::render::CommandBuffer cmds,
        ice::render::Renderpass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::Span<ice::vec4f> clear_values,
        ice::vec2u extent
    ) noexcept
    {
        VkClearValue vk_clear_values[10]{ };

        ice::u32 idx = 0;
        for (ice::vec4f const& clear_value : clear_values)
        {
            ice::memcpy(vk_clear_values[idx].color.float32, clear_value.v, sizeof(ice::f32) * 4);
            idx += 1;
        }

        VkRenderPassBeginInfo begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        begin_info.renderPass = native_handle(renderpass);
        begin_info.framebuffer = native_handle(framebuffer);
        begin_info.renderArea.offset.x = 0;
        begin_info.renderArea.offset.y = 0;
        begin_info.renderArea.extent = { extent.x, extent.y };
        begin_info.clearValueCount = idx;
        begin_info.pClearValues = vk_clear_values;

        vkCmdBeginRenderPass(
            native_handle(cmds),
            &begin_info,
            VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE
        );
    }

    void VulkanRenderCommands::bind_vertex_buffer(
        ice::render::CommandBuffer cmds,
        ice::render::Buffer buffer,
        ice::u32 binding
    ) noexcept
    {
        VkBuffer const vk_buffer = native_handle(buffer);
        VkDeviceSize const vk_offsets = 0;

        vkCmdBindVertexBuffers(
            native_handle(cmds),
            binding, 1,
            &vk_buffer,
            &vk_offsets
        );
    }

    void VulkanRenderCommands::draw(
        ice::render::CommandBuffer cmds,
        ice::u32 vertex_count,
        ice::u32 instance_count,
        ice::u32 vertex_offset,
        ice::u32 instance_offset
    ) noexcept
    {
        vkCmdDraw(
            native_handle(cmds),
            vertex_count,
            instance_count,
            vertex_offset,
            instance_offset
        );
    }

    void VulkanRenderCommands::draw_indexed(
        ice::render::CommandBuffer cmds,
        ice::u32 index_count,
        ice::u32 instance_count
    ) noexcept
    {
        vkCmdDrawIndexed(
            native_handle(cmds),
            index_count,
            instance_count,
            0, 0, 0
        );
    }

    void VulkanRenderCommands::draw_indexed(
        ice::render::CommandBuffer cmds,
        ice::u32 index_count,
        ice::u32 instance_count,
        ice::u32 index_offset,
        ice::u32 vertex_offset,
        ice::u32 instance_offset
    ) noexcept
    {
        vkCmdDrawIndexed(
            native_handle(cmds),
            index_count,
            instance_count,
            index_offset,
            vertex_offset,
            instance_offset
        );
    }

    void VulkanRenderCommands::next_subpass(
        ice::render::CommandBuffer cmds,
        ice::render::SubPassContents contents
    ) noexcept
    {
        vkCmdNextSubpass(
            native_handle(cmds),
            contents == SubPassContents::Inline
                ? VK_SUBPASS_CONTENTS_INLINE
                : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        );
    }

    void VulkanRenderCommands::end_renderpass(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        vkCmdEndRenderPass(
            native_handle(cmds)
        );
    }

    void VulkanRenderCommands::end(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        vkEndCommandBuffer(
            native_handle(cmds)
        );
    }

    void VulkanRenderCommands::pipeline_image_barrier(
        ice::render::CommandBuffer cmds,
        ice::render::PipelineStage source_stage,
        ice::render::PipelineStage destination_stage,
        ice::Span<ice::render::ImageBarrier const> image_barriers
    ) noexcept
    {
        auto native_cb = native_handle(cmds);

        ice::u32 barrier_count = 0;
        VkImageMemoryBarrier barriers[4] {};
        for (ice::render::ImageBarrier const& barrier : image_barriers)
        {
            VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(barrier.image));

            VkImageMemoryBarrier& image_barrier = barriers[barrier_count];
            image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barrier.oldLayout = native_enum_value(barrier.source_layout);
            image_barrier.newLayout = native_enum_value(barrier.destination_layout);
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.image = image_ptr->vk_image;
            image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_barrier.subresourceRange.baseMipLevel = 0;
            image_barrier.subresourceRange.levelCount = 1;
            image_barrier.subresourceRange.baseArrayLayer = 0;
            image_barrier.subresourceRange.layerCount = 1;
            image_barrier.srcAccessMask = native_enum_value(barrier.source_access);
            image_barrier.dstAccessMask = native_enum_value(barrier.destination_access);

            barrier_count += 1;
        }

        vkCmdPipelineBarrier(
            native_cb,
            native_enum_value(source_stage),
            native_enum_value(destination_stage),
            0,
            0, nullptr,
            0, nullptr,
            barrier_count, barriers
        );
    }

    void VulkanRenderCommands::update_texture(
        ice::render::CommandBuffer cmds,
        ice::render::Image image,
        ice::render::Buffer image_contents,
        ice::vec2u extents
    ) noexcept
    {
        VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));

        auto native_cb = native_handle(cmds);
        auto buffer_handle = native_handle(image_contents);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image_ptr->vk_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

        vkCmdPipelineBarrier(
            native_cb,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent.width = extents.x;
        region.imageExtent.height = extents.y;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(
            native_cb,
            buffer_handle,
            image_ptr->vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );


        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            native_cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void VulkanRenderCommands::update_texture_v2(
        ice::render::CommandBuffer cmds,
        ice::render::Image image,
        ice::render::Buffer image_contents,
        ice::vec2u extents
    ) noexcept
    {
        VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));

        auto native_cb = native_handle(cmds);
        auto buffer_handle = native_handle(image_contents);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent.width = extents.x;
        region.imageExtent.height = extents.y;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(
            native_cb,
            buffer_handle,
            image_ptr->vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }

    void VulkanRenderCommands::push_constant(
        ice::render::CommandBuffer cmds,
        ice::render::PipelineLayout pipeline,
        ice::render::ShaderStageFlags shader_stages,
        ice::Data data,
        ice::u32 offset
    ) noexcept
    {
        vkCmdPushConstants(
            native_handle(cmds),
            native_handle(pipeline),
            native_enum_flags(shader_stages),
            offset,
            data.size,
            data.location
        );
    }

} // namespace ice::render::vk
