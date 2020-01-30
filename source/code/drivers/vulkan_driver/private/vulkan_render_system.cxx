#include <core/memory.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

#include "api/v1/vulkan_render_api.hxx"

#include "vulkan_allocator.hxx"
#include "vulkan_device_memory_manager.hxx"
#include "vulkan_image.hxx"
#include "vulkan_shader.hxx"
#include "vulkan_buffer.hxx"
#include "pipeline/vulkan_vertex_descriptor.hxx"
#include "vulkan_pipeline.hxx"

#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_system.hxx>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace render
{

    class VulkanRenderSystem : public render::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc) noexcept
            : render::RenderSystem{}
            , _driver_allocator{ "vulkan-driver", alloc }
            , _vulkan_allocator{ alloc }
            , _vulkan_vertex_descriptors{ _driver_allocator }
            , _vulkan_buffers{ _driver_allocator }
            , _vulkan_images{ _driver_allocator }
            , _vulkan_shaders{ _driver_allocator }
        {
            initialize();
        }

        void initialize() noexcept
        {
            const char* instanceExtensionNames[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            };

            VkApplicationInfo app_info = {};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext = nullptr;
            app_info.pApplicationName = "IceShard (vulkan)";
            app_info.applicationVersion = 1;
            app_info.pEngineName = "IceShard";
            app_info.engineVersion = 1;
            app_info.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo instance_create_info = {};
            instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instance_create_info.pNext = nullptr;
            instance_create_info.flags = 0;
            instance_create_info.pApplicationInfo = &app_info;
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.ppEnabledLayerNames = nullptr;
            instance_create_info.enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensionNames));
            instance_create_info.ppEnabledExtensionNames = &instanceExtensionNames[0];

            auto vk_create_result = vkCreateInstance(&instance_create_info, _vulkan_allocator.vulkan_callbacks(), &_vulkan_instance);
            IS_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

            _vk_render_system = iceshard::renderer::vulkan::create_render_system(_driver_allocator, _vulkan_instance);

            _surface_extents = _vk_render_system->render_area();

            _vulkan_device_memory = core::memory::make_unique<vulkan::VulkanDeviceMemoryManager>(
                _driver_allocator,
                _driver_allocator,
                _vk_render_system->devices()
                );

            _vulkan_staging_buffer = render::vulkan::create_staging_buffer(_driver_allocator, *_vulkan_device_memory);

            _command_buffer_context.command_buffer = _vk_render_system->v1_secondary_cmd_buffer();
            _command_buffer_context.render_pass_context = &_render_pass_context;
            _command_buffer_context.render_pass_context->framebuffer = _vk_render_system->v1_current_framebuffer();


            VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
            imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            imageAcquiredSemaphoreCreateInfo.pNext = NULL;
            imageAcquiredSemaphoreCreateInfo.flags = 0;

            _render_pass_context.extent = _surface_extents;
            _render_pass_context.renderpass = _vk_render_system->v1_renderpass();

            VkFenceCreateInfo fenceInfo;
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.pNext = NULL;
            fenceInfo.flags = 0;

            auto res = vkCreateFence(_vk_render_system->v1_graphics_device(), &fenceInfo, NULL, &_vulkan_draw_fence);
            assert(res == VK_SUCCESS);

        }

        void prepare() noexcept override
        {
            vkDeviceWaitIdle(_vk_render_system->v1_graphics_device());

            auto new_extents = _vk_render_system->render_area();
            if (_surface_extents.width != new_extents.width || _surface_extents.height != new_extents.height)
            {
                fmt::print("ReConstructing graphics pipelines!\n");
                _surface_extents = new_extents;
                _render_pass_context.extent = _surface_extents;

                _vk_render_system->prepare(new_extents, iceshard::renderer::RenderPassFeatures::None);

                _render_pass_context.framebuffer = _vk_render_system->v1_current_framebuffer();
            }
        }

        void shutdown() noexcept
        {
            auto device = _vk_render_system->v1_graphics_device();

            for (auto const& entry : _vulkan_vertex_descriptors)
            {
                _driver_allocator.destroy(entry.value);
            }

            vkDestroyFence(device, _vulkan_draw_fence, nullptr);

            _vulkan_pipeline = nullptr;

            _vulkan_shaders.clear();
            _vulkan_images.clear();

            _vulkan_staging_buffer = nullptr;
            _vulkan_buffers.clear();

            _vulkan_device_memory = nullptr;

            iceshard::renderer::vulkan::destroy_render_system(_driver_allocator, _vk_render_system);

            // We need to provide the callbacks when destroying the instance.
            vkDestroyInstance(_vulkan_instance, _vulkan_allocator.vulkan_callbacks());
        }

        auto create_buffer(render::api::BufferType type, uint32_t size) noexcept -> render::api::Buffer override
        {
            auto vulkan_buffer = render::vulkan::create_buffer(
                _driver_allocator,
                type,
                size,
                *_vulkan_device_memory
            );

            auto result = render::api::Buffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
            _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

            return result;
        }

        auto create_vertex_buffer(uint32_t size) noexcept -> render::api::VertexBuffer override
        {
            auto vulkan_buffer = render::vulkan::create_vertex_buffer(
                _driver_allocator,
                *_vulkan_device_memory,
                size
            );

            auto result = render::api::VertexBuffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
            _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

            return result;
        }

        auto create_uniform_buffer(uint32_t size) noexcept -> render::api::UniformBuffer override
        {
            auto vulkan_buffer = render::vulkan::create_uniform_buffer(
                _driver_allocator,
                *_vulkan_device_memory,
                size
            );

            auto result = render::api::UniformBuffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
            _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

            return result;
        }

        auto current_framebuffer() noexcept -> render::api::Framebuffer override
        {
            IS_ASSERT(false, "This API function is deprecated! DO NOT USE!");
            return render::api::Framebuffer::Invalid; // No longer supported
        }

        auto create_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<iceshard::renderer::RenderResource> const& resources
        ) noexcept -> iceshard::renderer::ResourceSet override
        {
            return _vk_render_system->create_resource_set(name, resources);
        }

        void update_resource_set(
            core::stringid_arg_type name,
            core::pod::Array<iceshard::renderer::RenderResource> const& resources
        ) noexcept override
        {
            return _vk_render_system->update_resource_set(name, resources);
        }

        void destroy_resource_set(
            core::stringid_arg_type name
        ) noexcept override
        {
            return _vk_render_system->destroy_resource_set(name);
        }


        auto load_texture(asset::AssetData texture_data) noexcept -> render::api::Texture override
        {
            int32_t width = resource::get_meta_int32(texture_data.metadata, "texture.extents.width"_sid);
            int32_t height = resource::get_meta_int32(texture_data.metadata, "texture.extents.height"_sid);

            render::api::BufferDataView data_view;
            _vulkan_staging_buffer->map_memory(data_view);
            std::memcpy(data_view.data_pointer, texture_data.content._data, texture_data.content._size);
            _vulkan_staging_buffer->unmap_memory();

            VkExtent2D image_extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            auto texture = render::vulkan::create_texture_2d(
                _driver_allocator,
                *_vulkan_device_memory,
                image_extent
            );

            VkCommandBuffer staging_cmds = _vk_render_system->v1_transfer_cmd_buffer();

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(staging_cmds, &beginInfo);

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture->native_handle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0; // TODO
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

            vkCmdPipelineBarrier(
                staging_cmds,
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
            region.imageExtent.width = image_extent.width;
            region.imageExtent.height = image_extent.height;
            region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(
                staging_cmds,
                _vulkan_staging_buffer->native_handle(),
                texture->native_handle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );


            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                staging_cmds,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            vkEndCommandBuffer(staging_cmds);
            _staging_cmds = true;

            auto result_handle = render::api::Texture{ reinterpret_cast<uintptr_t>(texture->native_view()) };
            _vulkan_images.emplace_back(std::move(texture));

            return result_handle;
        }

        void load_shader(asset::AssetData shader_data) noexcept override
        {
            auto const& meta_view = shader_data.metadata;

            int32_t target = resource::get_meta_int32(meta_view, "shader.target"_sid);
            IS_ASSERT(target == 1, "Only explicit vulkan shaders are supported!");

            int32_t stage = resource::get_meta_int32(meta_view, "shader.stage"_sid);
            IS_ASSERT(stage == 1 || stage == 2, "Only vertex and fragment shaders are supported!");

            _vulkan_shaders.emplace_back(
                vulkan::create_shader(
                    _driver_allocator,
                    _vk_render_system->v1_graphics_device(),
                    stage == 1 ? VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT : VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                    shader_data.content
                )
            );
        }

        void add_named_vertex_descriptor_set(
            [[maybe_unused]] core::stringid_arg_type name,
            [[maybe_unused]] VertexBinding const& binding,
            [[maybe_unused]] VertexDescriptor const* descriptors,
            [[maybe_unused]] uint32_t descriptor_count
        ) noexcept override
        {
            auto hash_value = static_cast<uint64_t>(name.hash_value);
            IS_ASSERT(core::pod::hash::has(_vulkan_vertex_descriptors, hash_value) == false, "A descriptor set with this name {} was already defined!", name);

            // clang-format off
            core::pod::hash::set(_vulkan_vertex_descriptors, hash_value, _driver_allocator.make<render::vulkan::VulkanVertexDescriptor>(
                _driver_allocator,
                binding,
                descriptors,
                descriptor_count
                )
            );
            // clang-format on
        }

        auto create_pipeline(
            core::stringid_type const* descriptor_names,
            uint32_t descriptor_name_count
        ) noexcept -> api::RenderPipeline override
        {
            auto graphics_device = _vk_render_system->v1_graphics_device();

            {
                core::pod::Array<vulkan::VulkanShader const*> shader_stages{ _driver_allocator };
                std::for_each(_vulkan_shaders.begin(), _vulkan_shaders.end(), [&](auto const& shader_ptr) noexcept
                    {
                        core::pod::array::push_back(shader_stages, const_cast<vulkan::VulkanShader const*>(shader_ptr.get()));
                    });

                uint32_t descriptor_index = 0;

                core::pod::Array<vulkan::VulkanVertexDescriptor const*> vertex_descriptors{ _driver_allocator };
                while (descriptor_index < descriptor_name_count)
                {
                    auto descriptor_name_hash = static_cast<uint64_t>(descriptor_names[descriptor_index].hash_value);

                    auto const* descriptor = core::pod::hash::get<vulkan::VulkanVertexDescriptor*>(
                        _vulkan_vertex_descriptors,
                        descriptor_name_hash,
                        nullptr
                        );
                    IS_ASSERT(descriptor != nullptr, "Unknown descriptor name {}!", descriptor_names[descriptor_index]);

                    core::pod::array::push_back(vertex_descriptors, descriptor);
                    descriptor_index += 1;
                }

                _vulkan_pipeline = vulkan::create_pipeline(
                    _driver_allocator,
                    graphics_device,
                    shader_stages,
                    vertex_descriptors,
                    _vk_render_system->resource_layouts().pipeline_layout,
                    _vk_render_system->v1_renderpass()
                );
            }

            return api::RenderPipeline{ reinterpret_cast<uintptr_t>(_vulkan_pipeline->native_handle()) };
        }

        auto acquire_command_buffer(iceshard::renderer::RenderPassStage stage) noexcept -> iceshard::renderer::CommandBuffer override
        {
            return _vk_render_system->acquire_command_buffer(stage);
        }

        void submit_command_buffer(iceshard::renderer::CommandBuffer cb) noexcept override
        {
            _vk_render_system->submit_command_buffer(cb);
        }

        void swap() noexcept override
        {
            auto graphics_device = _vk_render_system->v1_graphics_device();
            auto graphics_queue = _vk_render_system->v1_graphics_queue();


            _vk_render_system->v1_acquire_next_image();


            _render_pass_context.extent = _surface_extents;
            _render_pass_context.renderpass = _vk_render_system->v1_renderpass();
            _render_pass_context.pipeline_layout = _vk_render_system->resource_layouts().pipeline_layout;
            _render_pass_context.framebuffer = _vk_render_system->v1_current_framebuffer();


            if (_staging_cmds)
            {
                _staging_cmds = false;
                const VkCommandBuffer cmd_bufs[] = { _vk_render_system->v1_transfer_cmd_buffer() };

                VkSubmitInfo submit_info[1] = {};
                submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info[0].commandBufferCount = 1;
                submit_info[0].pCommandBuffers = cmd_bufs;

                vkQueueSubmit(graphics_queue, 1, submit_info, VK_NULL_HANDLE);
                vkQueueWaitIdle(graphics_queue);
            }

            {
                auto cmds = _vk_render_system->v1_primary_cmd_buffer();

                VkCommandBufferBeginInfo cmd_buf_info = {};
                cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                cmd_buf_info.pNext = nullptr;
                cmd_buf_info.flags = 0;
                cmd_buf_info.pInheritanceInfo = nullptr;

                auto api_result = vkBeginCommandBuffer(cmds, &cmd_buf_info);
                IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");

                VkClearValue clear_values[2];
                clear_values[0].color.float32[0] = 0.2f;
                clear_values[0].color.float32[1] = 0.2f;
                clear_values[0].color.float32[2] = 0.2f;
                clear_values[0].color.float32[3] = 0.2f;
                clear_values[1].depthStencil.depth = 1.0f;
                clear_values[1].depthStencil.stencil = 0;

                VkRenderPassBeginInfo rp_begin;
                rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rp_begin.pNext = NULL;
                rp_begin.renderPass = _vk_render_system->v1_renderpass();
                rp_begin.framebuffer = _vk_render_system->v1_current_framebuffer();
                rp_begin.renderArea.offset.x = 0;
                rp_begin.renderArea.offset.y = 0;
                rp_begin.renderArea.extent = _surface_extents;
                rp_begin.clearValueCount = 2;
                rp_begin.pClearValues = clear_values;

                vkCmdBeginRenderPass(cmds, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdNextSubpass(cmds, VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

                _vk_render_system->v1_execute_subpass_commands(cmds);

                vkCmdEndRenderPass(cmds);
                vkEndCommandBuffer(cmds);
            }



            const VkCommandBuffer cmd_bufs[] = { _vk_render_system->v1_primary_cmd_buffer() };
            VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit_info[1] = {};
            submit_info[0].pNext = NULL;
            submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info[0].waitSemaphoreCount = 1;
            submit_info[0].pWaitSemaphores = _vk_render_system->v1_framebuffer_semaphore();
            submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
            submit_info[0].commandBufferCount = 1;
            submit_info[0].pCommandBuffers = cmd_bufs;
            submit_info[0].signalSemaphoreCount = 0;
            submit_info[0].pSignalSemaphores = NULL;


            /* Queue the command buffer for execution */
            auto res = vkQueueSubmit(graphics_queue, 1, submit_info, _vulkan_draw_fence);
            assert(res == VK_SUCCESS);

            /* Now present the image in the window */


            /* Make sure command buffer is finished before presenting */
            do
            {
                constexpr auto FENCE_TIMEOUT = 100'000'000; // in ns
                res = vkWaitForFences(graphics_device, 1, &_vulkan_draw_fence, VK_TRUE, FENCE_TIMEOUT);
            } while (res == VK_TIMEOUT);

            vkResetFences(graphics_device, 1, &_vulkan_draw_fence);

            _vk_render_system->v1_present();
        }

        void initialize_render_interface(render::api::RenderInterface** render_interface) noexcept
        {
            *render_interface = render::api::render_api_instance;
        }

        ~VulkanRenderSystem() noexcept override
        {
            shutdown();
        }

    public: // RenderSystem.v2
        auto renderpass(iceshard::renderer::RenderPassStage stage) noexcept -> iceshard::renderer::RenderPass
        {
            return _vk_render_system->renderpass(stage);
        }

    private:
        // Allocator for driver allocations.
        core::memory::proxy_allocator _driver_allocator;

        // Special allocator for vulkan render system.
        render::vulkan::VulkanAllocator _vulkan_allocator;

        // The Vulkan instance handle.
        VkInstance _vulkan_instance{};

        VkFence _vulkan_draw_fence = nullptr;

        // The Vulkan descriptor sets
        core::pod::Hash<render::vulkan::VulkanVertexDescriptor*> _vulkan_vertex_descriptors;

        // Databuffers
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_staging_buffer{ nullptr, { core::memory::globals::null_allocator() } };
        core::Vector<core::memory::unique_pointer<render::vulkan::VulkanBuffer>> _vulkan_buffers;

        // The Vulkan pipeline.
        core::memory::unique_pointer<render::vulkan::VulkanPipeline> _vulkan_pipeline{ nullptr, { core::memory::globals::null_allocator() } };

        // Array vulkan devices.
        core::memory::unique_pointer<render::vulkan::VulkanDeviceMemoryManager> _vulkan_device_memory{ nullptr, { core::memory::globals::null_allocator() } };

        render::api::v1::vulkan::RenderPassContext _render_pass_context{};
        render::api::v1::vulkan::CommandBufferContext _command_buffer_context{};

        // Shader stages
        core::Vector<core::memory::unique_pointer<vulkan::VulkanImage>> _vulkan_images;
        core::Vector<core::memory::unique_pointer<vulkan::VulkanShader>> _vulkan_shaders;

        bool _staging_cmds;

        VkExtent2D _surface_extents{ };

        ////////////////////////////////////////////////////////////////
        // New RenderSystem object
        iceshard::renderer::vulkan::VulkanRenderSystem* _vk_render_system = nullptr;
    };

} // namespace render

extern "C"
{
    __declspec(dllexport) auto create_render_system(
        core::allocator& alloc,
        core::cexpr::stringid_hash_type api_version,
        void* api_instance) -> render::RenderSystem*
    {
        render::RenderSystem* result = nullptr;
        if (api_version == render::api::v1::version_name.hash_value)
        {
            render::api::v1::vulkan::init_api(render::api::render_api_instance);
            render::api::v1::vulkan::init_api(api_instance);
            result = alloc.make<render::VulkanRenderSystem>(alloc);
        }
        return result;
    }

    __declspec(dllexport) void release_render_system(
        core::allocator& alloc,
        render::RenderSystem* driver)
    {
        alloc.destroy(driver);
    }
}
