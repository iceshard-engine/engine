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
#include "vulkan_device_memory_manager.hxx"
#include "vulkan_swapchain.hxx"
#include "vulkan_image.hxx"
#include "vulkan_shader.hxx"
#include "vulkan_buffer.hxx"
#include "pipeline/vulkan_descriptor_set_layout.hxx"
#include "pipeline/vulkan_descriptor_sets.hxx"
#include "pipeline/vulkan_vertex_descriptor.hxx"
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

            static const glm::mat4 instances[] = {
                glm::mat4{ 1.0f },
                glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 1.0f, 3.0f, 1.0f })
            };

            static auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
            static auto cam_pos = glm::vec3(-5, 3, -10);
            static auto view = glm::lookAt(cam_pos, // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),                 // and looks at the origin
                glm::vec3(0, -1, 0)                 // Head is up (set to 0,-1,0 to look upside-down)
            );
            static auto model = glm::mat4(1.0f);

            // Vulkan clip space has inverted Y and half Z.
            // clang-format off
            static auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.5f, 0.0f,
                0.0f, 0.0f, 0.5f, 1.0f);
            // clang-format on
            static auto MVP = clip * projection * view; // * model;

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
            , _vulkan_framebuffers{ _driver_allocator }
            , _vulkan_command_buffers{ _driver_allocator }
            , _vulkan_vertex_descriptors{ _driver_allocator }
            , _vulkan_buffers{ _driver_allocator }
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
            _vulkan_swapchain = render::vulkan::create_swapchain(_driver_allocator, _vulkan_physical_device.get());

            // Create depth buffer
            _vulkan_depth_image = render::vulkan::create_depth_buffer_image(_driver_allocator, _vulkan_physical_device.get());

            auto* physical_device = _vulkan_physical_device.get();
            auto graphics_device = physical_device->graphics_device()->native_handle();

            {
                api::BufferDataView data_view;

                _vulkan_uniform_buffer = vulkan::create_uniform_buffer(
                    _driver_allocator,
                    *_vulkan_physical_memory,
                    static_cast<uint32_t>(sizeof(detail::sample::MVP))
                );
                _vulkan_uniform_buffer->map_memory(data_view);
                IS_ASSERT(data_view.data_size >= sizeof(detail::sample::MVP), "Insufficient buffer size!");
                memcpy(data_view.data_pointer, &detail::sample::MVP, sizeof(detail::sample::MVP));
                _vulkan_uniform_buffer->unmap_memory();

                _vulkan_instance_buffer = vulkan::create_uniform_buffer(
                    _driver_allocator,
                    *_vulkan_physical_memory,
                    static_cast<uint32_t>(sizeof(detail::sample::instances))
                );

                _vulkan_instance_buffer->map_memory(data_view);
                IS_ASSERT(data_view.data_size >= sizeof(detail::sample::instances), "Insufficient buffer size!");
                memcpy(data_view.data_pointer, detail::sample::instances, sizeof(detail::sample::instances));
                _vulkan_instance_buffer->unmap_memory();
            }

            _vulkan_physical_device->graphics_device()->create_command_buffers(_vulkan_command_buffers, 1);

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
                auto const& formats = _vulkan_physical_device->surface_formats();

                _vulkan_render_pass = vulkan::create_render_pass(_driver_allocator, graphics_device, core::pod::array::front(formats).format, VK_FORMAT_D16_UNORM);
            }

            vulkan::create_framebuffers(
                _driver_allocator,
                _vulkan_framebuffers,
                graphics_device,
                _vulkan_render_pass->native_handle(),
                *_vulkan_depth_image.get(),
                *_vulkan_swapchain.get(),
                _vulkan_physical_device.get()
            );

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
            fmt::print("Selecting device at index: {}\n", 0);

            _vulkan_physical_device = core::memory::make_unique<vulkan::VulkanPhysicalDevice>(
                _driver_allocator,
                _driver_allocator,
                core::pod::array::front(devices_handles),
                _vulkan_surface->native_handle()
            );
            _vulkan_physical_memory = core::memory::make_unique<vulkan::VulkanDeviceMemoryManager>(
                _driver_allocator,
                _driver_allocator,
                _vulkan_physical_device.get(),
                _vulkan_physical_device->graphics_device()->native_handle()
            );
        }

        void release_devices() noexcept
        {
            _vulkan_physical_memory = nullptr;
            _vulkan_physical_device = nullptr;
        }

        void shutdown() noexcept
        {
            for (auto const& entry : _vulkan_vertex_descriptors)
            {
                _driver_allocator.destroy(entry.value);
            }

            auto* physical_device = _vulkan_physical_device.get();
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
            _vulkan_instance_buffer = nullptr;
            _vulkan_buffers.clear();

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

        auto command_buffer() noexcept -> render::CommandBuffer override
        {
            return CommandBuffer{ 0 };
        }

        auto create_vertex_buffer(uint32_t size) noexcept -> render::api::VertexBuffer override
        {
            auto vulkan_buffer = render::vulkan::create_vertex_buffer(
                _driver_allocator,
                *_vulkan_physical_memory,
                size
            );

            auto result = render::api::VertexBuffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
            _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

            return result;
        }

        auto current_frame_buffer() noexcept -> uintptr_t
        {
            return reinterpret_cast<uintptr_t>(_vulkan_framebuffers[_vulkan_current_framebuffer]->native_handle());
        }

        void load_shader(asset::AssetData shader_data) noexcept override
        {
            auto const& meta_view = shader_data.metadata;

            int32_t target = resource::get_meta_int32(meta_view, "shader.target"_sid);
            IS_ASSERT(target == 1, "Only explicit vulkan shaders are supported!");

            int32_t stage = resource::get_meta_int32(meta_view, "shader.stage"_sid);
            IS_ASSERT(stage == 1 || stage == 2, "Only vertex and fragment shaders are supported!");

            _shaders.emplace_back(
                vulkan::create_shader(
                    _driver_allocator,
                    _vulkan_physical_device->graphics_device()->native_handle(),
                    stage == 1 ? VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT : VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                    shader_data.content
                )
            );
        }

        void add_named_descriptor_set(
            [[maybe_unused]] core::cexpr::stringid_argument_type name,
            [[maybe_unused]] VertexBinding const& binding,
            [[maybe_unused]] VertexDescriptor const* descriptors,
            [[maybe_unused]] uint32_t descriptor_count) noexcept override
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
            core::cexpr::stringid_type const* descriptor_names,
            uint32_t descriptor_name_count
        ) noexcept -> api::RenderPipeline override
        {
            auto* physical_device = _vulkan_physical_device.get();
            auto graphics_device = physical_device->graphics_device()->native_handle();

            {
                core::pod::Array<VkDescriptorSetLayout> layouts{ _driver_allocator };
                core::pod::array::push_back(layouts, _vulkan_descriptor_sets_layout->native_handle());
                _vulkan_pipeline_layout = vulkan::create_pipeline_layout(_driver_allocator, graphics_device, layouts);

                core::pod::Array<vulkan::VulkanShader const*> shader_stages{ _driver_allocator };
                std::for_each(_shaders.begin(), _shaders.end(), [&](auto const& shader_ptr) noexcept
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
                    _vulkan_pipeline_layout.get(),
                    _vulkan_render_pass.get());
            }

            return api::RenderPipeline{ reinterpret_cast<uintptr_t>(_vulkan_pipeline->native_handle()) };
        }

        void swap() noexcept override
        {
            auto* physical_device = _vulkan_physical_device.get();
            auto graphics_device = physical_device->graphics_device();
            auto graphics_device_native = graphics_device->native_handle();
            auto swap_chain = _vulkan_swapchain->native_handle();

            {

                // Get the index of the next available swapchain image:
                auto api_result = vkAcquireNextImageKHR(
                    graphics_device_native,
                    swap_chain,
                    UINT64_MAX,
                    _quick_semaphore,
                    VK_NULL_HANDLE,
                    &_vulkan_current_framebuffer);

                // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
                // return codes
                IS_ASSERT(api_result == VK_SUCCESS, "Couldn't get next framebuffer image!");
            }

            [[maybe_unused]] auto const& surface_capabilities = physical_device->surface_capabilities();
            [[maybe_unused]] auto render_pass = _vulkan_render_pass->native_handle();

            auto cmd = _vulkan_command_buffers[0]->native_handle();

            VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = NULL;
            cmd_buf_info.flags = 0;
            cmd_buf_info.pInheritanceInfo = NULL;

            auto res = vkBeginCommandBuffer(cmd, &cmd_buf_info);
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
            rp_begin.framebuffer = reinterpret_cast<VkFramebuffer>(current_frame_buffer());
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent = surface_capabilities.maxImageExtent;
            rp_begin.clearValueCount = 2;
            rp_begin.pClearValues = clear_values;

            vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _vulkan_pipeline->native_handle());

            {
                auto new_view = glm::lookAt(detail::sample::cam_pos, // Camera is at (-5,3,-10), in World Space
                    glm::vec3(0, 0, 0),                              // and looks at the origin
                    glm::vec3(0, -1, 0)                              // Head is up (set to 0,-1,0 to look upside-down)
                );

                static float deg = 0.0f;
                deg += 3.0f;
                new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });

                if (deg >= 360.0f)
                    deg = 0.0f;

                auto MVP = detail::sample::clip * detail::sample::projection * new_view;

                render::api::BufferDataView data_view;
                _vulkan_uniform_buffer->map_memory(data_view);
                IS_ASSERT(data_view.data_size >= sizeof(MVP), "Insufficient buffer size!");
                memcpy(data_view.data_pointer, &MVP, sizeof(MVP));
                _vulkan_uniform_buffer->unmap_memory();
            }

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
                VkDeviceSize const offsets[2] = { 0, 0 };
                VkBuffer const buffers[2] = { _vulkan_buffers[0]->native_handle(), _vulkan_instance_buffer->native_handle() };
                vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);
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

            vkCmdDraw(cmd, 12 * 3, 2, 0, 0);
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
            present.pImageIndices = &_vulkan_current_framebuffer;
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
        core::pod::Hash<render::vulkan::VulkanVertexDescriptor*> _vulkan_vertex_descriptors;

        // Databuffers
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_uniform_buffer{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_instance_buffer{ nullptr, { core::memory::globals::null_allocator() } };

        core::Vector<core::memory::unique_pointer<render::vulkan::VulkanBuffer>> _vulkan_buffers;

        // The framebuffers
        uint32_t _vulkan_current_framebuffer = 0;
        core::pod::Array<vulkan::VulkanFramebuffer*> _vulkan_framebuffers;

        // The Vulkan pipeline.
        core::memory::unique_pointer<render::vulkan::VulkanPipelineLayout> _vulkan_pipeline_layout{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanPipeline> _vulkan_pipeline{ nullptr, { core::memory::globals::null_allocator() } };

        // Array vulkan devices.
        core::memory::unique_pointer<render::vulkan::VulkanPhysicalDevice> _vulkan_physical_device{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanDeviceMemoryManager> _vulkan_physical_memory{ nullptr, { core::memory::globals::null_allocator() } };

        core::pod::Array<render::vulkan::VulkanCommandBuffer*> _vulkan_command_buffers;

        // Shader stages
        std::vector<core::memory::unique_pointer<vulkan::VulkanShader>> _shaders;

        // Quick job
        VkSemaphore _quick_semaphore;
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
