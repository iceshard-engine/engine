#include "trait_render_texture_loader.hxx"

#include <ice/engine_runner.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/uri.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_request.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    IceWorldTrait_RenderTextureLoader::IceWorldTrait_RenderTextureLoader(
        ice::Allocator& alloc
    ) noexcept
        : _images{ alloc }
        , _tracked_images{ alloc }
    {
    }

    void IceWorldTrait_RenderTextureLoader::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Texture Loader :: Setup");

        using namespace ice::gfx;
        using namespace ice::render;
    }

    void IceWorldTrait_RenderTextureLoader::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Texture Loader :: Update");
    }

    void IceWorldTrait_RenderTextureLoader::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Texture Loader :: Cleanup");

        for (auto const& entry : _images)
        {
            ice::render::RenderDevice& device = gfx_device.device();
            device.destroy_image(entry);
        }
    }

    void IceWorldTrait_RenderTextureLoader::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::Array<ice::u64> remove_entries{ frame.allocator() };
        ice::array::reserve(remove_entries, 10);

        for (Entry const& entry : _tracked_images)
        {
            if (ice::asset_state(entry.asset_handle) == AssetState::Unknown)
            {
                portal.execute(
                    unload_image(
                        entry.image_index,
                        entry.image_hash,
                        runner
                    )
                );

                ice::array::push_back(
                    remove_entries,
                    entry.image_hash
                );
            }
        }

        for (ice::u64 key : remove_entries)
        {
            ice::hashmap::remove(_tracked_images, key);
        }


        ice::AssetStorage& storage = runner.asset_storage();
        ice::AssetRequest* const request = storage.aquire_request(ice::render::AssetType_Texture2D, AssetState::Runtime);

        if (request != nullptr)
        {
            portal.execute(
                load_image(request, runner)
            );
        }
    }

    auto IceWorldTrait_RenderTextureLoader::load_image(
        ice::AssetRequest* request,
        ice::EngineRunner& runner
    ) noexcept -> ice::Task<>
    {
        ice::Data image_data = request->data();
        ice::Metadata image_meta = request->resource().metadata();

        ice::render::ImageInfo const* const image_info =
            reinterpret_cast<ice::render::ImageInfo const*>(image_data.location);
        ice::u32 const image_data_size = image_info->width * image_info->height * 4;

        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        //ice::gfx::GfxResourceTracker& gfx_restracker = gfx_device.resource_tracker();

        ice::render::RenderDevice& device = gfx_device.device();

        ice::StringID const image_name = ice::stringid(request->resource().uri().path);
        ice::u64 const image_hash = ice::hash(image_name);

        if (ice::hashmap::has(_tracked_images, image_hash))
        {
            request->resolve(AssetRequest::Result::Skipped, { });
            co_return;
        }

        ice::Memory image_handle_data = request->allocate(ice::size_of<ice::render::Image>);
        ice::render::Image& image_handle = *reinterpret_cast<ice::render::Image*>(image_handle_data.location);

        co_await gfx_frame.frame_begin();

        ice::render::Buffer const data_buffer = device.create_buffer(
            ice::render::BufferType::Transfer,
            image_data_size
        );

        image_handle = device.create_image(*image_info, { });

        ice::render::BufferUpdateInfo const updates[1]{
            ice::render::BufferUpdateInfo
            {
                .buffer = data_buffer,
                .data = {
                    .location = image_info->data,
                    .size = image_data_size,
                    .alignment = ice::ualign::b_4
                }
            }
        };

        device.update_buffers({ updates, ice::count(updates) });

        struct : public ice::gfx::GfxFrameStage
        {
            void record_commands(
                ice::EngineFrame const& frame,
                ice::render::CommandBuffer cmds,
                ice::render::RenderCommands& api
            ) const noexcept override
            {
                api.update_texture(
                    cmds,
                    image,
                    image_data,
                    image_size
                );
            }

            ice::render::Image image;
            ice::render::Buffer image_data;
            ice::vec2u image_size;
        } frame_stage;

        frame_stage.image = image_handle;
        frame_stage.image_data = data_buffer;
        frame_stage.image_size = { image_info->width, image_info->height };

        // Await command recording stage
        //  Here we have access to a command buffer where we can record commands.
        //  These commands will be later executed on the graphics thread.
        co_await gfx_frame.frame_commands(&frame_stage);

        // Await end of graphics frame.
        //  Here we know that all commands have been executed
        //  and temporary objects can be destroyed.
        co_await gfx_frame.frame_end();

        device.destroy_buffer(data_buffer);

        co_await runner.thread_pool();

        ice::AssetHandle const* asset_handle = request->resolve(AssetRequest::Result::Success, image_handle_data);

        co_await runner.schedule_next_frame();

        ice::u32 const idx = ice::array::count(_images);
        ice::array::push_back(_images, image_handle);

        ICE_ASSERT(
            ice::hashmap::has(_tracked_images, image_hash) == false,
            "Hash map already contains entry for the given image!"
        );

        ice::hashmap::set(_tracked_images, image_hash, Entry{ asset_handle, image_hash, idx });
        co_return;
    }

    auto IceWorldTrait_RenderTextureLoader::unload_image(
        ice::u32 image_idx,
        ice::u64 image_hash,
        ice::EngineRunner& runner
    ) noexcept -> ice::Task<>
    {
        ice::render::Image image = _images[image_idx];

        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        //ice::gfx::GfxResourceTracker& gfx_restracker = gfx_device.resource_tracker();

        ice::render::RenderDevice& device = gfx_device.device();

        co_await gfx_frame.frame_begin();

        device.destroy_image(image);

        co_await runner.schedule_next_frame();

        if (ice::array::empty(_images) == false)
        {
            _images[image_idx] = ice::array::back(_images);
        }
        ice::array::pop_back(_images);

        co_return;
    }

    //struct : public ice::gfx::GfxFrameStage
    //{
    //    void record_commands(
    //        ice::EngineFrame const& frame,
    //        ice::render::CommandBuffer cmds,
    //        ice::render::RenderCommands& api
    //    ) const noexcept override
    //    {
    //        ImageBarrier barriers[4]{ };

    //        for (ice::u32 idx = 0; idx < image_count; ++idx)
    //        {
    //            barriers[idx].image = image[idx];
    //            barriers[idx].source_layout = ImageLayout::Undefined;
    //            barriers[idx].destination_layout = ImageLayout::TransferDstOptimal;
    //            barriers[idx].source_access = AccessFlags::None;
    //            barriers[idx].destination_access = AccessFlags::TransferWrite;
    //        }

    //        api.pipeline_image_barrier(
    //            cmds,
    //            PipelineStage::TopOfPipe,
    //            PipelineStage::Transfer,
    //            { barriers, image_count }
    //        );

    //        for (ice::u32 idx = 0; idx < image_count; ++idx)
    //        {
    //            api.update_texture_v2(
    //                cmds,
    //                image[idx],
    //                image_data[idx],
    //                image_size[idx]
    //            );
    //        }

    //        for (ice::u32 idx = 0; idx < image_count; ++idx)
    //        {
    //            barriers[idx].image = image[idx];
    //            barriers[idx].source_layout = ImageLayout::TransferDstOptimal;
    //            barriers[idx].destination_layout = ImageLayout::ShaderReadOnly;
    //            barriers[idx].source_access = AccessFlags::TransferWrite;
    //            barriers[idx].destination_access = AccessFlags::ShaderRead;
    //        }

    //        api.pipeline_image_barrier(
    //            cmds,
    //            PipelineStage::Transfer,
    //            PipelineStage::FramentShader,
    //            { barriers, image_count }
    //        );
    //    }

    //    ice::u32 image_count;
    //    ice::render::Image* image;
    //    ice::render::Buffer* image_data;
    //    ice::vec2u* image_size;
    //} frame_stage;

    //frame_stage.image_count = image_count;
    //frame_stage.image = operation.render_cache->tileset_images;
    //frame_stage.image_data = image_data_buffer;
    //frame_stage.image_size = image_extent;

    void register_trait_render_texture_loader(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            Constant_TraitName_RenderBase,
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderTextureLoader,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderTextureLoader>,
                .required_dependencies = trait_dependencies
            }
        );
    }

} // namespace ice
