#include "vulkan_pipeline.hxx"

namespace render::vulkan
{

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipeline pipeline) noexcept
        : _device_handle{ device }
        , _native_handle{ pipeline }
    {
    }

    VulkanPipeline::~VulkanPipeline() noexcept
    {
        vkDestroyPipeline(_device_handle, _native_handle, nullptr);
    }

    auto create_pipeline(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VulkanShader const*> shaders,
        VulkanPipelineLayout const* pipeline_layout,
        VulkanRenderPass const* render_pass) noexcept -> core::memory::unique_pointer<VulkanPipeline>
    {
        core::pod::Array<VkPipelineShaderStageCreateInfo> shader_stages_info{ alloc };
        core::pod::array::resize(shader_stages_info, core::pod::array::size(shaders));

        uint32_t stage_index = 0;
        for (auto const* shader : shaders)
        {
            shader_stages_info[stage_index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stages_info[stage_index].pNext = nullptr;
            shader_stages_info[stage_index].pSpecializationInfo = nullptr;
            shader_stages_info[stage_index].flags = 0;
            shader_stages_info[stage_index].stage = shader->stage();
            shader_stages_info[stage_index].pName = "main";
            stage_index += 1;
        }

        VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
        memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pNext = nullptr;
        dynamicState.pDynamicStates = dynamicStateEnables;
        dynamicState.dynamicStateCount = 0;

        VkVertexInputBindingDescription vi_binding;
        VkVertexInputAttributeDescription vi_attribs[2];

        vi_binding.binding = 0;
        vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vi_binding.stride = 32; // 32bytes for now 16 (pos) + 16 (color)
        vi_attribs[0].binding = 0;
        vi_attribs[0].location = 0;
        vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vi_attribs[0].offset = 0;
        vi_attribs[1].binding = 0;
        vi_attribs[1].location = 1;
        vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vi_attribs[1].offset = 16;

        VkPipelineVertexInputStateCreateInfo vi;
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = nullptr;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &vi_binding;
        vi.vertexAttributeDescriptionCount = 2;
        vi.pVertexAttributeDescriptions = vi_attribs;

        VkPipelineInputAssemblyStateCreateInfo ia;
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.pNext = nullptr;
        ia.flags = 0;
        ia.primitiveRestartEnable = VK_FALSE;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rs;
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.pNext = nullptr;
        rs.flags = 0;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_BACK_BIT;
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rs.depthClampEnable = VK_TRUE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.depthBiasEnable = VK_FALSE;
        rs.depthBiasConstantFactor = 0;
        rs.depthBiasClamp = 0;
        rs.depthBiasSlopeFactor = 0;
        rs.lineWidth = 1.0f;

        VkPipelineColorBlendStateCreateInfo cb;
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.pNext = nullptr;
        cb.flags = 0;

        VkPipelineColorBlendAttachmentState att_state[1];
        att_state[0].colorWriteMask = 0xf;
        att_state[0].blendEnable = VK_FALSE;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cb.attachmentCount = 1;
        cb.pAttachments = att_state;
        cb.logicOpEnable = VK_FALSE;
        cb.logicOp = VK_LOGIC_OP_NO_OP;
        cb.blendConstants[0] = 1.0f;
        cb.blendConstants[1] = 1.0f;
        cb.blendConstants[2] = 1.0f;
        cb.blendConstants[3] = 1.0f;

        VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = nullptr;
        ds.flags = 0;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.minDepthBounds = 0;
        ds.maxDepthBounds = 0;
        ds.stencilTestEnable = VK_FALSE;
        ds.back.failOp = VK_STENCIL_OP_KEEP;
        ds.back.passOp = VK_STENCIL_OP_KEEP;
        ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
        ds.back.compareMask = 0;
        ds.back.reference = 0;
        ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.back.writeMask = 0;
        ds.front = ds.back;

        VkPipelineMultisampleStateCreateInfo ms;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext = nullptr;
        ms.flags = 0;
        ms.pSampleMask = nullptr;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        ms.sampleShadingEnable = VK_FALSE;
        ms.alphaToCoverageEnable = VK_FALSE;
        ms.alphaToOneEnable = VK_FALSE;
        ms.minSampleShading = 0.0;

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.pNext = NULL;
        vp.flags = 0;
        vp.viewportCount = 1;
        dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
        vp.scissorCount = 1;
        dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
        vp.pScissors = NULL;
        vp.pViewports = NULL;

        VkGraphicsPipelineCreateInfo pipeline;
        pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline.pNext = nullptr;
        pipeline.layout = pipeline_layout->native_handle();
        pipeline.basePipelineHandle = VK_NULL_HANDLE;
        pipeline.basePipelineIndex = 0;
        pipeline.flags = 0;
        pipeline.pVertexInputState = &vi;
        pipeline.pInputAssemblyState = &ia;
        pipeline.pRasterizationState = &rs;
        pipeline.pColorBlendState = &cb;
        pipeline.pTessellationState = nullptr;
        pipeline.pMultisampleState = &ms;
        pipeline.pDynamicState = &dynamicState;
        pipeline.pViewportState = &vp;
        pipeline.pDepthStencilState = &ds;
        pipeline.pStages = &shader_stages_info[0];
        pipeline.stageCount = core::pod::array::size(shader_stages_info);
        pipeline.renderPass = render_pass->native_handle();
        pipeline.subpass = 0;

        VkPipeline pipeline_handle;
        auto api_result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline, nullptr, &pipeline_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline object!");

        return core::memory::make_unique<VulkanPipeline>(alloc, device, pipeline_handle);
    }

} // namespace render::vulkan
