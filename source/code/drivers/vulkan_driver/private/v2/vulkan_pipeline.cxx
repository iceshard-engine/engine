#include <iceshard/renderer/vulkan/vulkan_pipeline.hxx>

#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    void query_vertex_input_descriptions(
        RenderPipelineLayout pipeline_layout,
        core::pod::Array<VkVertexInputBindingDescription>& bindings,
        core::pod::Array<VkVertexInputAttributeDescription>& attributes
    ) noexcept
    {
        if (pipeline_layout == RenderPipelineLayout::DebugUI)
        {
            core::pod::array::resize(bindings, 1);
            bindings[0].binding = 0;
            bindings[0].inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
            bindings[0].stride = sizeof(float) * 4 + sizeof(uint32_t);

            core::pod::array::resize(attributes, 3);
            attributes[0].binding = 0;
            attributes[0].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
            attributes[0].location = 0;
            attributes[0].offset = 0;

            attributes[1].binding = 0;
            attributes[1].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
            attributes[1].location = 1;
            attributes[1].offset = sizeof(float) * 2;

            attributes[2].binding = 0;
            attributes[2].format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
            attributes[2].location = 2;
            attributes[2].offset = sizeof(float) * 4;
        }
    }

    void create_graphics_pipeline(
        VulkanDevices devices,
        VulkanRenderPass renderpass,
        VulkanPipelineLayout layout,
        VulkanPipelineModules modules,
        VulkanPipeline& pipeline
    ) noexcept
    {
        pipeline.layout = layout.layout;

        VkPipelineShaderStageCreateInfo shader_stages[core::size(modules.modules)];

        uint32_t stage_idx = 0;
        for (; stage_idx < core::size(modules.modules); ++stage_idx)
        {
            if (modules.modules[stage_idx] == nullptr)
            {
                break;
            }

            auto& shader_stage = shader_stages[stage_idx];
            shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage.pNext = nullptr;
            shader_stage.pSpecializationInfo = nullptr;
            shader_stage.flags = 0;
            shader_stage.stage = modules.stage[stage_idx];
            shader_stage.module = modules.modules[stage_idx];
            shader_stage.pName = "main";
        }

        VkDynamicState dynamic_states[2]; // max: VK_DYNAMIC_STATE_RANGE_SIZE
        dynamic_states[0] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT;
        dynamic_states[1] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR;

        VkPipelineDynamicStateCreateInfo dynamic_state = {};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.pNext = nullptr;
        dynamic_state.pDynamicStates = dynamic_states;
        dynamic_state.dynamicStateCount = core::size(dynamic_states);

        core::memory::stack_allocator_2048 temp_alloc;
        core::pod::Array<VkVertexInputBindingDescription> vertex_input_bindings{ temp_alloc };
        core::pod::Array<VkVertexInputAttributeDescription> vertex_input_attributes{ temp_alloc };

        query_vertex_input_descriptions(
            layout.layout_type,
            vertex_input_bindings,
            vertex_input_attributes
        );

        VkPipelineVertexInputStateCreateInfo vi;
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = nullptr;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = core::pod::array::size(vertex_input_bindings);
        vi.pVertexBindingDescriptions = core::pod::array::begin(vertex_input_bindings);
        vi.vertexAttributeDescriptionCount = core::pod::array::size(vertex_input_attributes);
        vi.pVertexAttributeDescriptions = core::pod::array::begin(vertex_input_attributes);

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
        rs.cullMode = (layout.layout_type == RenderPipelineLayout::DebugUI ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.depthBiasEnable = VK_FALSE;
        rs.depthBiasConstantFactor = 0;
        rs.depthBiasClamp = 0;
        rs.depthBiasSlopeFactor = 0;
        rs.lineWidth = 1.0f;

        VkPipelineColorBlendStateCreateInfo blend_info;
        blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend_info.pNext = nullptr;
        blend_info.flags = 0;

        VkPipelineColorBlendAttachmentState att_state[1];
        att_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        att_state[0].blendEnable = VK_TRUE;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        blend_info.attachmentCount = 1;
        blend_info.pAttachments = att_state;
        blend_info.logicOpEnable = VK_FALSE;
        blend_info.logicOp = VK_LOGIC_OP_NO_OP;
        blend_info.blendConstants[0] = 1.0f;
        blend_info.blendConstants[1] = 1.0f;
        blend_info.blendConstants[2] = 1.0f;
        blend_info.blendConstants[3] = 1.0f;

        VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = nullptr;
        ds.flags = 0;
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
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
        vp.scissorCount = 1;
        vp.pScissors = NULL;
        vp.pViewports = NULL;

        VkGraphicsPipelineCreateInfo pipeline_info;
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.pNext = nullptr;
        pipeline_info.layout = layout.layout;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = 0;
        pipeline_info.flags = 0;
        pipeline_info.pVertexInputState = &vi;
        pipeline_info.pInputAssemblyState = &ia;
        pipeline_info.pRasterizationState = &rs;
        pipeline_info.pColorBlendState = &blend_info;
        pipeline_info.pTessellationState = nullptr;
        pipeline_info.pMultisampleState = &ms;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.pViewportState = &vp;
        pipeline_info.pDepthStencilState = &ds;
        pipeline_info.pStages = shader_stages;
        pipeline_info.stageCount = core::size(shader_stages);
        pipeline_info.renderPass = renderpass.renderpass;
        pipeline_info.subpass = 1;

        auto api_result = vkCreateGraphicsPipelines(
            devices.graphics.handle,
            nullptr,
            1,
            &pipeline_info,
            nullptr,
            &pipeline.pipeline
        );
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create pipeline object!");
    }

    void destroy_graphics_pipeline(
        VulkanDevices devices,
        VulkanPipeline pipeline
    ) noexcept
    {
        vkDestroyPipeline(devices.graphics.handle, pipeline.pipeline, nullptr);
    }

} // namespace iceshard::renderer::vulkan
