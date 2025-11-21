/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_image_storage_trait.hxx"

#include <ice/asset_storage.hxx>
#include <ice/config.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_types_mappers.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource.hxx>
#include <ice/shard_container.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/world/world_updater.hxx>

namespace ice::gfx
{

    auto internal_load_image(
        ice::AssetRequest* request,
        ice::TaskScheduler& final_thread,
        ice::HashMap<ice::gfx::GfxImageEntry>& entries
    ) noexcept -> ice::Task<>
    {
        request->data();

        co_await final_thread;
    }

    Trait_GfxImageStorage::Trait_GfxImageStorage(ice::TraitContext& ctx, ice::Allocator& alloc) noexcept
        : ice::Trait{ ctx }
        , _loaded_images{ alloc }
    {
        _context.bind<&Trait_GfxImageStorage::gfx_update, Render>(ice::gfx::ShardID_RenderFrameUpdate);
        _context.bind<&Trait_GfxImageStorage::gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
    }

    auto Trait_GfxImageStorage::on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<>
    {
        GfxImageEntry* entry = ice::hashmap::try_get(_loaded_images, ice::hash(asset.name()));
        ICE_ASSERT_CORE(entry != nullptr);
        entry->released = true; // Mark as released
        co_return;
    }

    auto Trait_GfxImageStorage::gfx_update(
        ice::gfx::RenderFrameUpdate const& params,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
    {
        // Handle up to 4 requests at the same time each frame.
        ice::AssetRequest* request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
        while(request != nullptr)
        {
            ice::AssetState const state = request->state();
            ICE_ASSERT_CORE(state == AssetState::Loaded); // The image needs to be loaded.

            ice::StringID const image_hash = request->asset_name();
            GfxImageEntry* entry = ice::hashmap::try_get(_loaded_images, ice::hash(image_hash));
            ICE_ASSERT_CORE(entry == nullptr || entry->released);

            using namespace ice::render;
            RenderDevice& device = params.context.device();

            if (entry && entry->image != Image::Invalid)
            {
                // Destroy the previous image object, as it might be outdated.
                device.destroy_image(entry->image);
            }

            ice::Data const metadata_data = request->metadata();
            if (metadata_data.location == nullptr)
            {
                request->resolve({ .result = ice::AssetRequestResult::Error });
                request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
                continue;
            }

            ice::Config const meta = ice::config::from_data(metadata_data);

            ice::i32 image_format;
            ice::vec2i size;

            [[maybe_unused]]
            bool valid_data = ice::config::get(meta, "texture.format", image_format);
            valid_data &= ice::config::get(meta, "texture.size.x", size.x);
            valid_data &= ice::config::get(meta, "texture.size.y", size.y);

            [[maybe_unused]]
            ice::Data const texture_data = request->data();

            // Creates the image object
            ImageInfo image_info{
                .type = ImageType::Image2D,
                .format = (ImageFormat) image_format,
                .usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst,
                .width = (ice::u32) size.x,
                .height = (ice::u32) size.y,
            };
            Image image = device.create_image(image_info, texture_data);
            if (image == Image::Invalid)
            {
                request->resolve({ .result = AssetRequestResult::Error });
                co_return;
            }

            ice::render::Buffer transfer_buffer = device.create_buffer(BufferType::Transfer, ice::u32(texture_data.size.value));
            BufferUpdateInfo const buffer_updates[]{
                BufferUpdateInfo{
                    .buffer = transfer_buffer,
                    .data = texture_data,
                    .offset = 0
                }
            };

            device.update_buffers(buffer_updates);

            RenderCommands& api = device.get_commands();
            CommandBuffer const cmds = co_await params.stages.frame_transfer;

            ImageBarrier barriers[4]{ };

            for (ice::u32 idx = 0; idx < 1; ++idx)
            {
                barriers[idx].image = image;
                barriers[idx].source_layout = ImageLayout::Undefined;
                barriers[idx].destination_layout = ImageLayout::TransferDstOptimal;
                barriers[idx].source_access = AccessFlags::None;
                barriers[idx].destination_access = AccessFlags::TransferWrite;
            }

            api.pipeline_image_barrier(
                cmds,
                PipelineStage::TopOfPipe,
                PipelineStage::Transfer,
                { barriers, 1 }
            );

            api.update_texture_v2(
                cmds,
                image,
                transfer_buffer,
                ice::vec2u{ size }
            );

            for (ice::u32 idx = 0; idx < 1; ++idx)
            {
                barriers[idx].image = image;
                barriers[idx].source_layout = ImageLayout::TransferDstOptimal;
                barriers[idx].destination_layout = ImageLayout::ShaderReadOnly;
                barriers[idx].source_access = AccessFlags::TransferWrite;
                barriers[idx].destination_access = AccessFlags::ShaderRead;
            }

            api.pipeline_image_barrier(
                cmds,
                PipelineStage::Transfer,
                PipelineStage::FramentShader,
                { barriers, 1 }
            );

            co_await params.stages.frame_end;

            device.destroy_buffer(transfer_buffer);

            ICE_LOG(LogSeverity::Info, LogTag::Game, "TextureStorage - Loaded image: {}", request->asset_name());

            // Allocates a handle for it... (TODO: Rework?)
            ice::Memory const result = request->allocate(ice::size_of<ice::render::Image>);
            *reinterpret_cast<Image*>(result.location) = image;

            // Reslove the request (will resume all awaiting tasks)
            ice::Asset asset = request->resolve({ .resolver = this, .result = AssetRequestResult::Success, .memory = result });
            // send("iceshard:images-internal:loaded"_shardid, asset);

            // Save the image handle
            ice::hashmap::set(_loaded_images, ice::hash(image_hash), { .asset = ice::move(asset), .image = image});

            // // Reslove the request (will resume all awaiting tasks)
            // request->resolve({ .resolver = this, .result = AssetRequestResult::Success, .memory = result });

            // Get the next queued request
            request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
        }

        co_return;
    }

    auto Trait_GfxImageStorage::gfx_shutdown(
        ice::render::RenderDevice& device
    ) noexcept -> ice::Task<>
    {
        for (ice::gfx::GfxImageEntry& entry : ice::hashmap::values(_loaded_images))
        {
            device.destroy_image(entry.image);
            entry.asset.release();
        }

        ice::hashmap::clear(_loaded_images);
        co_return;
    }

} // namespace ice::gfx
