#include <core/memory.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>

#include "api/v1/vulkan_render_api.hxx"

#include "vulkan_allocator.hxx"
#include "vulkan_surface.hxx"
#include "device/vulkan_physical_device.hxx"
#include "device/vulkan_command_buffer.hxx"
#include "vulkan_swapchain.hxx"
#include "vulkan_image.hxx"
#include "vulkan_shader.hxx"
#include "vulkan_buffer.hxx"
#include "pipeline/vulkan_descriptor_set_layout.hxx"
#include "pipeline/vulkan_descriptor_sets.hxx"
#include "pipeline/vulkan_pipeline_layout.hxx"
#include "vulkan_framebuffer.hxx"
#include "vulkan_pipeline.hxx"

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace render
{
    namespace detail
    {

        auto vk_iceshard_allocate(void* userdata, size_t size, size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void*
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->allocate(static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        auto vk_iceshard_reallocate(void* userdata, void* original, size_t size, size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void*
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->reallocate(original, static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        void vk_iceshard_free(void* userdata, void* memory) noexcept
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            allocator->deallocate(memory);
        }

        namespace sample
        {
            static const char vertShaderText[] =
                "#version 400\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (std140, binding = 0) uniform bufferVals {\n"
                "    mat4 mvp;\n"
                "} myBufferVals;\n"
                "layout (location = 0) in vec4 pos;\n"
                "layout (location = 1) in vec4 inColor;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main() {\n"
                "   outColor = inColor;\n"
                "   gl_Position = myBufferVals.mvp * pos;\n"
                "}\n";

            static const char fragShaderText[] =
                "#version 400\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (location = 0) in vec4 color;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main() {\n"
                "   outColor = color;\n"
                "}\n";

#define XYZ1(_x_, _y_, _z_) (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_) (_u_), (_v_)

            struct Vertex
            {
                float posX, posY, posZ, posW; // Position data
                float r, g, b, a;             // Color
            };

            static const Vertex g_vb_solid_face_colors_Data[] = {
                // red face
                { XYZ1(-1, -1, 1), XYZ1(1.f, 0.f, 0.f) },
                { XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f) },
                { XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f) },
                { XYZ1(1, -1, 1), XYZ1(1.f, 0.f, 0.f) },
                { XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 0.f) },
                { XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 0.f) },
                // green face
                { XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 0.f) },
                { XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f) },
                { XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f) },
                { XYZ1(-1, 1, -1), XYZ1(0.f, 1.f, 0.f) },
                { XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 0.f) },
                { XYZ1(1, 1, -1), XYZ1(0.f, 1.f, 0.f) },
                // blue face
                { XYZ1(-1, 1, 1), XYZ1(0.f, 0.f, 1.f) },
                { XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f) },
                { XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f) },
                { XYZ1(-1, 1, -1), XYZ1(0.f, 0.f, 1.f) },
                { XYZ1(-1, -1, 1), XYZ1(0.f, 0.f, 1.f) },
                { XYZ1(-1, -1, -1), XYZ1(0.f, 0.f, 1.f) },
                // yellow face
                { XYZ1(1, 1, 1), XYZ1(1.f, 1.f, 0.f) },
                { XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f) },
                { XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f) },
                { XYZ1(1, -1, 1), XYZ1(1.f, 1.f, 0.f) },
                { XYZ1(1, 1, -1), XYZ1(1.f, 1.f, 0.f) },
                { XYZ1(1, -1, -1), XYZ1(1.f, 1.f, 0.f) },
                // magenta face
                { XYZ1(1, 1, 1), XYZ1(1.f, 0.f, 1.f) },
                { XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f) },
                { XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f) },
                { XYZ1(1, 1, -1), XYZ1(1.f, 0.f, 1.f) },
                { XYZ1(-1, 1, 1), XYZ1(1.f, 0.f, 1.f) },
                { XYZ1(-1, 1, -1), XYZ1(1.f, 0.f, 1.f) },
                // cyan face
                { XYZ1(1, -1, 1), XYZ1(0.f, 1.f, 1.f) },
                { XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f) },
                { XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f) },
                { XYZ1(-1, -1, 1), XYZ1(0.f, 1.f, 1.f) },
                { XYZ1(1, -1, -1), XYZ1(0.f, 1.f, 1.f) },
                { XYZ1(-1, -1, -1), XYZ1(0.f, 1.f, 1.f) },
            };
#undef XYZ1
#undef UV

            static auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
            static auto view = glm::lookAt(glm::vec3(-5, 3, -10), // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),                        // and looks at the origin
                glm::vec3(0, -1, 0)                        // Head is up (set to 0,-1,0 to look upside-down)
            );
            static auto model = glm::mat4(1.0f);

            // Vulkan clip space has inverted Y and half Z.
            // clang-format off
            static auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f,-1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.0f, 0.0f, 0.5f, 1.0f);
            // clang-format on
            static auto MVP = clip * projection * view * model;

        } // namespace sample

        //void vk_iceshard_internal_allocate_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalAllocationNotification f;
        //}

        //void vk_iceshard_internal_free_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalFreeNotification f;
        //}

    } // namespace detail

    class VulkanRenderSystem : public render::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc) noexcept
            : render::RenderSystem{}
            , _driver_allocator{ "vulkan-driver", alloc }
            , _vulkan_allocator{ alloc }
            , _command_buffer{ alloc }
            , _vulkan_devices{ _driver_allocator }
            , _vulkan_framebuffers{ _driver_allocator }
            , _vulkan_command_buffers{ _driver_allocator }
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

            VkAllocationCallbacks alloc_callbacks = {};
            alloc_callbacks.pUserData = &_vulkan_allocator;
            alloc_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
            alloc_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
            alloc_callbacks.pfnFree = detail::vk_iceshard_free;
            alloc_callbacks.pfnInternalAllocation = nullptr;
            alloc_callbacks.pfnInternalFree = nullptr;

            auto vk_create_result = vkCreateInstance(&instance_create_info, &alloc_callbacks, &_vulkan_instance);
            IS_ASSERT(vk_create_result == VkResult::VK_SUCCESS, "Creation of Vulkan instance failed!");

            // Create the surface object
            _vulkan_surface = render::vulkan::create_surface(_driver_allocator, _vulkan_instance);

            enumerate_devices();

            // Create swap chain
            _vulkan_swapchain = render::vulkan::create_swapchain(_driver_allocator, core::pod::array::front(_vulkan_devices));

            // Create depth buffer
            _vulkan_depth_image = render::vulkan::create_depth_buffer_image(_driver_allocator, core::pod::array::front(_vulkan_devices));

            auto* physical_device = core::pod::array::front(_vulkan_devices);
            auto graphics_device = physical_device->graphics_device()->native_handle();

            {
                BufferDataView data_view;

                _vulkan_uniform_buffer = vulkan::create_uniform_buffer(
                    _driver_allocator,
                    physical_device,
                    VkDeviceSize{ sizeof(detail::sample::MVP) });
                _vulkan_uniform_buffer->map_memory(data_view);
                IS_ASSERT(data_view.data_size >= sizeof(detail::sample::MVP), "Insufficient buffer size!");
                memcpy(data_view.data_pointer, &detail::sample::MVP, sizeof(detail::sample::MVP));
                _vulkan_uniform_buffer->unmap_memory();

                _vulkan_vertex_buffer = vulkan::create_uniform_buffer(
                    _driver_allocator,
                    physical_device,
                    VkDeviceSize{ sizeof(detail::sample::g_vb_solid_face_colors_Data) });

                _vulkan_vertex_buffer->map_memory(data_view);
                IS_ASSERT(data_view.data_size >= sizeof(detail::sample::g_vb_solid_face_colors_Data), "Insufficient buffer size!");
                memcpy(data_view.data_pointer, detail::sample::g_vb_solid_face_colors_Data, sizeof(detail::sample::g_vb_solid_face_colors_Data));
                _vulkan_vertex_buffer->unmap_memory();
            }

            core::pod::array::front(_vulkan_devices)->graphics_device()->create_command_buffers(_vulkan_command_buffers, 1);

            // clang-format off
            _shaders.emplace_back(
                vulkan::create_shader(
                    _driver_allocator,
                    graphics_device ,
                    VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
                    { detail::sample::vertShaderText, sizeof(detail::sample::vertShaderText) }
                )
            );
            _shaders.emplace_back(
                vulkan::create_shader(
                    _driver_allocator,
                    graphics_device,
                    VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                    { detail::sample::fragShaderText, sizeof(detail::sample::fragShaderText) }
                )
            );
            // clang-format on

            {
                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = 0;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                layout_binding.pImmutableSamplers = nullptr;

                core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ _driver_allocator };
                core::pod::array::push_back(bindings, std::move(layout_binding));

                _vulkan_descriptor_sets_layout = vulkan::create_descriptor_set_layout(_driver_allocator, graphics_device, bindings);

                core::pod::Array<vulkan::VulkanDescriptorSetLayout*> layouts{ _driver_allocator };
                core::pod::array::push_back(layouts, _vulkan_descriptor_sets_layout.get());
                _vulkan_descriptor_sets = vulkan::create_vulkan_descriptor_sets(_driver_allocator, graphics_device, layouts);

                VkDescriptorBufferInfo buffer_info{};
                buffer_info.buffer = _vulkan_uniform_buffer->native_handle();
                buffer_info.offset = 0;
                buffer_info.range = sizeof(detail::sample::MVP);
                _vulkan_descriptor_sets->write_descriptor_set(0, VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer_info);
            }

            {
                auto const& formats = core::pod::array::front(_vulkan_devices)->surface_formats();

                _vulkan_render_pass = vulkan::create_render_pass(_driver_allocator, graphics_device, core::pod::array::front(formats).format, VK_FORMAT_D16_UNORM);
            }

            vulkan::create_framebuffers(
                _driver_allocator,
                _vulkan_framebuffers,
                graphics_device,
                _vulkan_render_pass->native_handle(),
                *_vulkan_depth_image.get(),
                *_vulkan_swapchain.get(),
                core::pod::array::front(_vulkan_devices));

            {
                core::pod::Array<VkDescriptorSetLayout> layouts{ _driver_allocator };
                core::pod::array::push_back(layouts, _vulkan_descriptor_sets_layout->native_handle());
                _vulkan_pipeline_layout = vulkan::create_pipeline_layout(_driver_allocator, graphics_device, layouts);

                core::pod::Array<vulkan::VulkanShader const*> shader_stages{ _driver_allocator };
                std::for_each(
                    _shaders.begin(), _shaders.end(),
                    [&](auto const& shader_ptr) noexcept {
                        core::pod::array::push_back(shader_stages, const_cast<vulkan::VulkanShader const*>(shader_ptr.get()));
                    });
                _vulkan_pipeline = vulkan::create_pipeline(_driver_allocator, graphics_device, shader_stages, _vulkan_pipeline_layout.get(), _vulkan_render_pass.get());
            }

            VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
            imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            imageAcquiredSemaphoreCreateInfo.pNext = NULL;
            imageAcquiredSemaphoreCreateInfo.flags = 0;

            auto res = vkCreateSemaphore(graphics_device, &imageAcquiredSemaphoreCreateInfo, NULL, &_quick_semaphore);
            assert(res == VK_SUCCESS);
        }

        void enumerate_devices() noexcept
        {
            IS_ASSERT(_vulkan_surface != nullptr, "Surface object does not exist!");

            uint32_t device_count;
            VkResult res = vkEnumeratePhysicalDevices(_vulkan_instance, &device_count, nullptr);
            IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query the number of available vulkan devices!");

            core::pod::Array<VkPhysicalDevice> devices_handles{ _driver_allocator };
            core::pod::array::resize(devices_handles, device_count);

            vkEnumeratePhysicalDevices(_vulkan_instance, &device_count, &devices_handles[0]);
            IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query available vulkan devices!");

            // Create VulkanPhysicalDevice objects from the handles.
            fmt::print("Available Vulkan devices: {}\n", device_count);
            for (const auto& physical_device_handle : devices_handles)
            {
                core::pod::array::push_back(
                    _vulkan_devices,
                    _driver_allocator.make<vulkan::VulkanPhysicalDevice>(
                        _driver_allocator,
                        physical_device_handle,
                        _vulkan_surface->native_handle()));
            }
        }

        void release_devices() noexcept
        {
            for (auto* vulkan_device : _vulkan_devices)
            {
                _driver_allocator.destroy(vulkan_device);
            }
            core::pod::array::clear(_vulkan_devices);
        }

        void shutdown() noexcept
        {
            auto* physical_device = core::pod::array::front(_vulkan_devices);
            vkDestroySemaphore(physical_device->graphics_device()->native_handle(), _quick_semaphore, nullptr);

            _vulkan_pipeline = nullptr;
            _vulkan_pipeline_layout = nullptr;

            for (auto* vulkan_framebuffer : _vulkan_framebuffers)
            {
                _driver_allocator.destroy(vulkan_framebuffer);
            }
            core::pod::array::clear(_vulkan_framebuffers);

            _vulkan_render_pass = nullptr;

            _vulkan_descriptor_sets = nullptr;
            _vulkan_descriptor_sets_layout = nullptr;

            _shaders.clear();

            core::pod::array::clear(_vulkan_command_buffers);

            _vulkan_uniform_buffer = nullptr;
            _vulkan_vertex_buffer = nullptr;

            _vulkan_depth_image = nullptr;

            _vulkan_swapchain = nullptr;

            release_devices();

            VkAllocationCallbacks alloc_callbacks = {};
            alloc_callbacks.pUserData = &_vulkan_allocator;
            alloc_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
            alloc_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
            alloc_callbacks.pfnFree = detail::vk_iceshard_free;
            alloc_callbacks.pfnInternalAllocation = nullptr;
            alloc_callbacks.pfnInternalFree = nullptr;

            _vulkan_surface = nullptr;

            // We need to provide the callbacks when destrying the instance.
            vkDestroyInstance(_vulkan_instance, &alloc_callbacks);
        }

        auto command_buffer() noexcept -> render::CommandBufferHandle override
        {
            return CommandBufferHandle{ 0 };
        }

        void swap() noexcept override
        {
            auto* physical_device = core::pod::array::front(_vulkan_devices);
            [[maybe_unused]] auto const& surface_capabilities = physical_device->surface_capabilities();
            auto graphics_device = physical_device->graphics_device();
            auto graphics_device_native = graphics_device->native_handle();
            auto swap_chain = _vulkan_swapchain->native_handle();
            [[maybe_unused]] auto render_pass = _vulkan_render_pass->native_handle();

            auto cmd = _vulkan_command_buffers[0]->native_handle();

            VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = NULL;
            cmd_buf_info.flags = 0;
            cmd_buf_info.pInheritanceInfo = NULL;

            auto res = vkBeginCommandBuffer(cmd, &cmd_buf_info);
            assert(res == VK_SUCCESS);

            // Get the index of the next available swapchain image:
            uint32_t current_buffer = 0;
            res = vkAcquireNextImageKHR(graphics_device_native, swap_chain, UINT64_MAX, _quick_semaphore, VK_NULL_HANDLE, &current_buffer);
            // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
            // return codes
            assert(res == VK_SUCCESS);

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
            rp_begin.renderPass = render_pass;
            rp_begin.framebuffer = _vulkan_framebuffers[current_buffer]->native_handle();
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent = surface_capabilities.maxImageExtent;
            rp_begin.clearValueCount = 2;
            rp_begin.pClearValues = clear_values;

            vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _vulkan_pipeline->native_handle());

            core::pod::Array<VkDescriptorSet> sets{ _driver_allocator };
            _vulkan_descriptor_sets->native_handles(sets);
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                _vulkan_pipeline_layout->native_handle(),
                0,
                core::pod::array::size(sets),
                core::pod::array::begin(sets),
                0,
                NULL);

            {
                VkDeviceSize const offsets[1] = { 0 };
                VkBuffer const buffers[1] = { _vulkan_vertex_buffer->native_handle() };
                vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
            }

            {
                VkViewport viewport{};
                viewport.height = (float)surface_capabilities.maxImageExtent.height;
                viewport.width = (float)surface_capabilities.maxImageExtent.width;
                viewport.minDepth = (float)0.0f;
                viewport.maxDepth = (float)1.0f;
                viewport.x = 0;
                viewport.y = 0;
                vkCmdSetViewport(cmd, 0, 1, &viewport);
            }

            {
                VkRect2D scissor{};
                scissor.extent = surface_capabilities.maxImageExtent;
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                vkCmdSetScissor(cmd, 0, 1, &scissor);
            }

            vkCmdDraw(cmd, 12 * 3, 1, 0, 0);
            vkCmdEndRenderPass(cmd);
            res = vkEndCommandBuffer(cmd);
            assert(res == VK_SUCCESS);

            VkFenceCreateInfo fenceInfo;
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.pNext = NULL;
            fenceInfo.flags = 0;

            VkFence drawFence;
            res = vkCreateFence(graphics_device_native, &fenceInfo, NULL, &drawFence);
            assert(res == VK_SUCCESS);

            const VkCommandBuffer cmd_bufs[] = { cmd };
            VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit_info[1] = {};
            submit_info[0].pNext = NULL;
            submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info[0].waitSemaphoreCount = 1;
            submit_info[0].pWaitSemaphores = &_quick_semaphore;
            submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
            submit_info[0].commandBufferCount = 1;
            submit_info[0].pCommandBuffers = cmd_bufs;
            submit_info[0].signalSemaphoreCount = 0;
            submit_info[0].pSignalSemaphores = NULL;

            auto queue = graphics_device->device_queue();
            IS_ASSERT(graphics_device->can_present(), "Cannot present images on this queue!");
            // Check if we can also present else find another 'Device' which can do it.

            /* Queue the command buffer for execution */
            res = vkQueueSubmit(queue, 1, submit_info, drawFence);
            assert(res == VK_SUCCESS);

            /* Now present the image in the window */

            VkSwapchainKHR swapchains[1]{ _vulkan_swapchain->native_handle() };

            VkPresentInfoKHR present;
            present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.pNext = NULL;
            present.swapchainCount = 1;
            present.pSwapchains = swapchains;
            present.pImageIndices = &current_buffer;
            present.pWaitSemaphores = NULL;
            present.waitSemaphoreCount = 0;
            present.pResults = NULL;

            /* Make sure command buffer is finished before presenting */
            do
            {
                constexpr auto FENCE_TIMEOUT = 100'000'000; // in ns
                res = vkWaitForFences(graphics_device_native, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
            } while (res == VK_TIMEOUT);

            vkDestroyFence(graphics_device_native, drawFence, nullptr);

            assert(res == VK_SUCCESS);
            res = vkQueuePresentKHR(queue, &present);
            assert(res == VK_SUCCESS);
        }

        ~VulkanRenderSystem() noexcept override
        {
            shutdown();
        }

    private:
        // Allocator for driver allocations.
        core::memory::proxy_allocator _driver_allocator;

        // Special allocator for vulkan render system.
        render::vulkan::VulkanAllocator _vulkan_allocator;

        // The Vulkan instance handle.
        VkInstance _vulkan_instance{};

        // The Vulkan surface instance.
        core::memory::unique_pointer<render::vulkan::VulkanSurface> _vulkan_surface{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan surface instance.
        core::memory::unique_pointer<render::vulkan::VulkanSwapchain> _vulkan_swapchain{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan depth buffer image.
        core::memory::unique_pointer<render::vulkan::VulkanImage> _vulkan_depth_image{ nullptr, { core::memory::globals::null_allocator() } };

        core::memory::unique_pointer<render::vulkan::VulkanRenderPass> _vulkan_render_pass{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan descriptor sets
        core::memory::unique_pointer<render::vulkan::VulkanDescriptorSetLayout> _vulkan_descriptor_sets_layout{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanDescriptorSets> _vulkan_descriptor_sets{ nullptr, { core::memory::globals::null_allocator() } };

        // Databuffers
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_uniform_buffer{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_vertex_buffer{ nullptr, { core::memory::globals::null_allocator() } };

        // The framebuffers
        core::pod::Array<vulkan::VulkanFramebuffer*> _vulkan_framebuffers;

        // The Vulkan pipeline.
        core::memory::unique_pointer<render::vulkan::VulkanPipelineLayout> _vulkan_pipeline_layout{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanPipeline> _vulkan_pipeline{ nullptr, { core::memory::globals::null_allocator() } };

        // Array vulkan devices.
        core::pod::Array<render::vulkan::VulkanPhysicalDevice*> _vulkan_devices;

        core::pod::Array<render::vulkan::VulkanCommandBuffer*> _vulkan_command_buffers;

        // Shader stages
        std::vector<core::memory::unique_pointer<vulkan::VulkanShader>> _shaders;

        // Quick job
        VkSemaphore _quick_semaphore;

    private:
        render::RenderCommandBuffer _command_buffer;
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
