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
#include "vulkan_sampler.hxx"
#include "vulkan_shader.hxx"
#include "vulkan_buffer.hxx"
#include "pipeline/vulkan_descriptor_pool.hxx"
#include "pipeline/vulkan_descriptor_set_layout.hxx"
#include "pipeline/vulkan_descriptor_sets.hxx"
#include "pipeline/vulkan_vertex_descriptor.hxx"
#include "pipeline/vulkan_pipeline_layout.hxx"
#include "vulkan_framebuffer.hxx"
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
            , _vulkan_framebuffers{ _driver_allocator }
            , _vulkan_command_buffers{ _driver_allocator }
            , _vulkan_vertex_descriptors{ _driver_allocator }
            , _vulkan_buffers{ _driver_allocator }
            , _vulkan_descriptor_set_layouts{ _driver_allocator }
            , _vulkan_images{ _driver_allocator }
            , _vulkan_samplers{ _driver_allocator }
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

            // Create the surface object
            _vulkan_surface = render::vulkan::create_surface(_driver_allocator, _vulkan_instance);

            enumerate_devices();

            _vulkan_staging_buffer = render::vulkan::create_staging_buffer(_driver_allocator, *_vulkan_device_memory);

            // Create swap chain
            _vulkan_swapchain = render::vulkan::create_swapchain(_driver_allocator, _vulkan_physical_device.get());

            // Create depth buffer
            _vulkan_depth_image = render::vulkan::create_depth_buffer_image(_driver_allocator, *_vulkan_device_memory, _surface_extents);
            _vulkan_pp_image = render::vulkan::create_attachment_texture(_driver_allocator, *_vulkan_device_memory, _surface_extents);

            auto* physical_device = _vulkan_physical_device.get();
            auto graphics_device = physical_device->graphics_device()->native_handle();

            _vulkan_descriptor_pool = core::memory::make_unique<render::vulkan::VulkanDescriptorPool>(_driver_allocator, graphics_device);
            _vulkan_physical_device->graphics_device()->create_command_buffers(_vulkan_command_buffers, 2);

            _command_buffer_context.command_buffer = _vulkan_command_buffers[0]->native_handle();
            _command_buffer_context.render_pass_context = &_render_pass_context;

            {
                using iceshard::renderer::RenderPassStage;

                auto const& formats = _vulkan_physical_device->surface_formats();
                auto format = core::pod::array::front(formats).format;

                _vk_render_system = iceshard::renderer::vulkan::create_render_system(_driver_allocator, graphics_device);
                _vk_render_system->prepare(format, iceshard::renderer::RenderPassFeatures::None);
            }

            vulkan::create_framebuffers(
                _driver_allocator,
                _vulkan_framebuffers,
                graphics_device,
                _vk_render_system->renderpass_native(),
                *_vulkan_depth_image.get(),
                *_vulkan_swapchain.get(),
                nullptr,
                _vulkan_physical_device.get()
            );

            VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
            imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            imageAcquiredSemaphoreCreateInfo.pNext = NULL;
            imageAcquiredSemaphoreCreateInfo.flags = 0;

            auto res = vkCreateSemaphore(graphics_device, &imageAcquiredSemaphoreCreateInfo, NULL, &_quick_semaphore);
            assert(res == VK_SUCCESS);

            _render_pass_context.extent = _surface_extents;
            _render_pass_context.renderpass = _vk_render_system->renderpass_native();
            _render_pass_context.framebuffer = _vulkan_framebuffers[_vulkan_current_framebuffer]->native_handle();

            VkFenceCreateInfo fenceInfo;
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.pNext = NULL;
            fenceInfo.flags = 0;

            res = vkCreateFence(graphics_device, &fenceInfo, NULL, &_vulkan_draw_fence);
            assert(res == VK_SUCCESS);
        }

        void prepare() noexcept override
        {
            vkDeviceWaitIdle(_vulkan_physical_device->graphics_device()->native_handle());

            auto new_extents = _vulkan_physical_device->update_surface_capabilities().currentExtent;
            if (_surface_extents.width != new_extents.width || _surface_extents.height != new_extents.height)
            {
                [[maybe_unused]]
                auto graphics_device = _vulkan_physical_device->graphics_device()->native_handle();

                fmt::print("ReConstructing graphics pipelines!\n");
                _surface_extents = new_extents;
                _render_pass_context.extent = _surface_extents;

                //_vulkan_pipeline = nullptr;
                //_vulkan_pipeline_layout = nullptr;

                for (auto* vulkan_framebuffer : _vulkan_framebuffers)
                {
                    _driver_allocator.destroy(vulkan_framebuffer);
                }
                core::pod::array::clear(_vulkan_framebuffers);

                //for (auto& entry : _render_passes)
                //{
                //    vkDestroyRenderPass(graphics_device, entry.value, nullptr);
                //}
                //core::pod::hash::clear(_render_passes);

                _vulkan_pp_image = nullptr;
                _vulkan_depth_image = nullptr;
                _vulkan_swapchain = nullptr;

                // Create swap chain
                _vulkan_swapchain = render::vulkan::create_swapchain(_driver_allocator, _vulkan_physical_device.get());
                _vulkan_depth_image = render::vulkan::create_depth_buffer_image(_driver_allocator, *_vulkan_device_memory, _surface_extents);
                _vulkan_pp_image = render::vulkan::create_attachment_texture(_driver_allocator, *_vulkan_device_memory, _surface_extents);

                vulkan::create_framebuffers(
                    _driver_allocator,
                    _vulkan_framebuffers,
                    _vulkan_physical_device->graphics_device()->native_handle(),
                    _vk_render_system->renderpass_native(),
                    *_vulkan_depth_image.get(),
                    *_vulkan_swapchain.get(),
                    nullptr,
                    _vulkan_physical_device.get()
                );

                _render_pass_context.framebuffer = _vulkan_framebuffers[_vulkan_current_framebuffer]->native_handle();

                //{
                //    using iceshard::renderer::RenderPassType;
                //    using iceshard::renderer::vulkan::create_renderpass;

                //    auto const& formats = _vulkan_physical_device->surface_formats();


                //    auto format = core::pod::array::front(formats).format;
                //    auto fwd_rpass = create_renderpass<RenderPassType::Forward>(graphics_device, format);
                //    auto fwd_pp_rpass = create_renderpass<RenderPassType::ForwardPostProcess>(graphics_device, format);

                //    core::pod::hash::set(_render_passes, (uint64_t)RenderPassType::Forward, fwd_rpass);
                //    core::pod::hash::set(_render_passes, (uint64_t)RenderPassType::ForwardPostProcess, fwd_pp_rpass);
                //}

                //_render_pass_context.renderpass = core::pod::hash::get(_render_passes, 0llu, {});
                //_vulkan_physical_device->graphics_device()->create_command_buffers(_vulkan_command_buffers, 2);
                //_command_buffer_context.command_buffer = _vulkan_command_buffers[0]->native_handle();

                //RenderSystem::create_pipeline(render::pipeline::ImGuiPipeline);
            }
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

            _surface_extents = _vulkan_physical_device->surface_capabilities().currentExtent;

            _vulkan_device_memory = core::memory::make_unique<vulkan::VulkanDeviceMemoryManager>(
                _driver_allocator,
                _driver_allocator,
                _vulkan_physical_device.get(),
                _vulkan_physical_device->graphics_device()->native_handle()
            );
        }

        void release_devices() noexcept
        {
            _vulkan_device_memory = nullptr;
            _vulkan_physical_device = nullptr;
        }

        void shutdown() noexcept
        {
            auto device = _vulkan_physical_device->graphics_device()->native_handle();

            for (auto const& entry : _vulkan_vertex_descriptors)
            {
                _driver_allocator.destroy(entry.value);
            }

            vkDestroyFence(device, _vulkan_draw_fence, nullptr);
            vkDestroySemaphore(device, _quick_semaphore, nullptr);

            _vulkan_pipeline = nullptr;
            _vulkan_pipeline_layout = nullptr;

            for (auto* vulkan_framebuffer : _vulkan_framebuffers)
            {
                _driver_allocator.destroy(vulkan_framebuffer);
            }
            core::pod::array::clear(_vulkan_framebuffers);

            iceshard::renderer::vulkan::destroy_render_system(_driver_allocator, _vk_render_system);
            //for (auto& entry : _render_passes)
            //{
            //    vkDestroyRenderPass(device, entry.value, nullptr);
            //}
            //core::pod::hash::clear(_render_passes);

            _vulkan_descriptor_sets = nullptr;
            _vulkan_descriptor_set_layouts.clear();

            _vulkan_shaders.clear();
            _vulkan_samplers.clear();
            _vulkan_images.clear();

            core::pod::array::clear(_vulkan_command_buffers);

            _vulkan_staging_buffer = nullptr;
            _vulkan_buffers.clear();

            _vulkan_descriptor_pool = nullptr;

            _vulkan_pp_image = nullptr;
            _vulkan_depth_image = nullptr;

            _vulkan_swapchain = nullptr;

            release_devices();

            _vulkan_surface = nullptr;

            // We need to provide the callbacks when destroying the instance.
            vkDestroyInstance(_vulkan_instance, _vulkan_allocator.vulkan_callbacks());
        }

        auto command_buffer() noexcept -> render::api::CommandBuffer override
        {
            return render::api::CommandBuffer{ reinterpret_cast<uintptr_t>(&_command_buffer_context) };
        }

        auto descriptor_sets() noexcept -> render::api::DescriptorSets override
        {
            return render::api::DescriptorSets{ reinterpret_cast<uintptr_t>(_vulkan_descriptor_sets.get()) };
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

        void create_imgui_descriptor_sets() noexcept
        {
            auto const graphics_device_handle = _vulkan_physical_device->graphics_device()->native_handle();

            _vulkan_samplers.emplace_back(render::vulkan::create_sampler(_driver_allocator, graphics_device_handle));

            {
                core::pod::Array<VkDescriptorSetLayoutBinding> bindings{ _driver_allocator };

                //VkDescriptorSetLayoutBinding layout_binding = {};
                //layout_binding.binding = 0;
                //layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                //layout_binding.descriptorCount = 1;
                //layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                //layout_binding.pImmutableSamplers = nullptr;
                //core::pod::array::push_back(bindings, std::move(layout_binding));

                static VkSampler const _vulkan_immutable_samplers[]{
                    _vulkan_samplers[0]->native_handle()
                };

                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = 1;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                layout_binding.pImmutableSamplers = _vulkan_immutable_samplers;
                core::pod::array::push_back(bindings, std::move(layout_binding));

                layout_binding.binding = 2;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                layout_binding.pImmutableSamplers = nullptr;
                core::pod::array::push_back(bindings, std::move(layout_binding));

                _vulkan_descriptor_set_layouts.emplace_back(
                    vulkan::create_descriptor_set_layout(_driver_allocator, graphics_device_handle, bindings)
                );
            }

            _vulkan_descriptor_sets = vulkan::create_vulkan_descriptor_sets(
                _driver_allocator,
                *_vulkan_descriptor_pool,
                _vulkan_descriptor_set_layouts
            );

            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = _vulkan_images[0]->native_view();
            image_info.sampler = nullptr;
            _vulkan_descriptor_sets->write_descriptor_set(0, 2, VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, image_info);
        }

        auto current_framebuffer() noexcept -> render::api::Framebuffer override
        {
            return render::api::Framebuffer{ reinterpret_cast<uintptr_t>(_vulkan_framebuffers[_vulkan_current_framebuffer]->native_handle()) };
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

            VkCommandBuffer staging_cmds = _vulkan_command_buffers[1]->native_handle();

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

            auto result_handle = render::api::Texture{ reinterpret_cast<uintptr_t>(texture.get()) };
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
                    _vulkan_physical_device->graphics_device()->native_handle(),
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
            auto* physical_device = _vulkan_physical_device.get();
            auto graphics_device = physical_device->graphics_device()->native_handle();

            {
                _vulkan_pipeline_layout = vulkan::create_pipeline_layout(
                    _driver_allocator,
                    graphics_device,
                    _vulkan_descriptor_set_layouts
                );
                _render_pass_context.pipeline_layout = _vulkan_pipeline_layout->native_handle();

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
                    _vulkan_pipeline_layout.get(),
                    _vk_render_system->renderpass_native()
                );
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

            _render_pass_context.extent = _surface_extents;
            _render_pass_context.renderpass = _vk_render_system->renderpass_native();
            _render_pass_context.pipeline_layout = _vulkan_pipeline_layout->native_handle();
            _render_pass_context.framebuffer = _vulkan_framebuffers[_vulkan_current_framebuffer]->native_handle();


            if (_staging_cmds)
            {
                _staging_cmds = false;
                const VkCommandBuffer cmd_bufs[] = { _vulkan_command_buffers[1]->native_handle() };

                VkSubmitInfo submit_info[1] = {};
                submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info[0].commandBufferCount = 1;
                submit_info[0].pCommandBuffers = cmd_bufs;

                vkQueueSubmit(graphics_device->device_queue(), 1, submit_info, VK_NULL_HANDLE);
                vkQueueWaitIdle(graphics_device->device_queue());
            }

            const VkCommandBuffer cmd_bufs[] = { _vulkan_command_buffers[0]->native_handle() };
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
            auto res = vkQueueSubmit(queue, 1, submit_info, _vulkan_draw_fence);
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
                res = vkWaitForFences(graphics_device_native, 1, &_vulkan_draw_fence, VK_TRUE, FENCE_TIMEOUT);
            } while (res == VK_TIMEOUT);

            vkResetFences(graphics_device_native, 1, &_vulkan_draw_fence);

            assert(res == VK_SUCCESS);
            res = vkQueuePresentKHR(queue, &present);
            assert(res == VK_SUCCESS);
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
        auto get_pipeline() noexcept -> render::api::RenderPipeline
        {
            return api::RenderPipeline{ reinterpret_cast<uintptr_t>(_vulkan_pipeline->native_handle()) };
        }

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

        // The Vulkan surface instance.
        core::memory::unique_pointer<render::vulkan::VulkanSurface> _vulkan_surface{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan surface instance.
        core::memory::unique_pointer<render::vulkan::VulkanSwapchain> _vulkan_swapchain{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan depth buffer image.
        core::memory::unique_pointer<render::vulkan::VulkanImage> _vulkan_pp_image{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanImage> _vulkan_depth_image{ nullptr, { core::memory::globals::null_allocator() } };

        // The Vulkan descriptor sets
        core::memory::unique_pointer<render::vulkan::VulkanDescriptorPool> _vulkan_descriptor_pool{ nullptr, { core::memory::globals::null_allocator() } };
        core::Vector<core::memory::unique_pointer<render::vulkan::VulkanDescriptorSetLayout>> _vulkan_descriptor_set_layouts;
        core::memory::unique_pointer<render::vulkan::VulkanDescriptorSets> _vulkan_descriptor_sets{ nullptr, { core::memory::globals::null_allocator() } };
        core::pod::Hash<render::vulkan::VulkanVertexDescriptor*> _vulkan_vertex_descriptors;

        // Databuffers
        core::memory::unique_pointer<render::vulkan::VulkanBuffer> _vulkan_staging_buffer{ nullptr, { core::memory::globals::null_allocator() } };
        core::Vector<core::memory::unique_pointer<render::vulkan::VulkanBuffer>> _vulkan_buffers;

        // The framebuffers
        uint32_t _vulkan_current_framebuffer = 0;
        core::pod::Array<vulkan::VulkanFramebuffer*> _vulkan_framebuffers;

        // The Vulkan pipeline.
        core::memory::unique_pointer<render::vulkan::VulkanPipelineLayout> _vulkan_pipeline_layout{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanPipeline> _vulkan_pipeline{ nullptr, { core::memory::globals::null_allocator() } };

        // Array vulkan devices.
        core::memory::unique_pointer<render::vulkan::VulkanPhysicalDevice> _vulkan_physical_device{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::vulkan::VulkanDeviceMemoryManager> _vulkan_device_memory{ nullptr, { core::memory::globals::null_allocator() } };

        core::pod::Array<render::vulkan::VulkanCommandBuffer*> _vulkan_command_buffers;
        bool _staging_cmds = false;

        render::api::v1::vulkan::RenderPassContext _render_pass_context{};
        render::api::v1::vulkan::CommandBufferContext _command_buffer_context{};

        // Shader stages
        core::Vector<core::memory::unique_pointer<vulkan::VulkanImage>> _vulkan_images;
        core::Vector<core::memory::unique_pointer<vulkan::VulkanSampler>> _vulkan_samplers;
        core::Vector<core::memory::unique_pointer<vulkan::VulkanShader>> _vulkan_shaders;

        VkExtent2D _surface_extents{ };

        // Quick job
        VkSemaphore _quick_semaphore;


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
