/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_image_storage_trait.hxx"
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/engine_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/shard_container.hxx>

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

    Trait_GfxImageStorage::Trait_GfxImageStorage(ice::Allocator& alloc, ice::TraitContext& ctx) noexcept
        : ice::Trait{ ctx }
        , _loaded_images{ alloc }
    {
        _context.bind<&Trait_GfxImageStorage::gfx_update>(ice::gfx::ShardID_GfxFrameUpdate);
    }

    auto Trait_GfxImageStorage::on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<>
    {
        GfxImageEntry* entry = ice::hashmap::try_get(_loaded_images, ice::hash(asset.name()));
        ICE_ASSERT_CORE(entry != nullptr);
        entry->released = true; // Mark as released
        co_return;
    }

    auto Trait_GfxImageStorage::gfx_update(ice::gfx::GfxFrameUpdate const& params) noexcept -> ice::Task<>
    {
        // Handle up to 4 requests at the same time each frame.
        ice::AssetRequest* request = params.assets.aquire_request(ice::render::AssetType_Texture2D, AssetState::Runtime);
        while(request != nullptr)
        {
            ice::AssetState const state = request->state();
            ICE_ASSERT_CORE(state == AssetState::Loaded); // The image needs to be loaded.

            ice::u64 const image_hash = ice::hash(request->asset_name());
            GfxImageEntry* entry = ice::hashmap::try_get(_loaded_images, image_hash);
            ICE_ASSERT_CORE(entry == nullptr || entry->released);

            using namespace ice::render;
            RenderDevice& device = params.context.device();

            if (entry && entry->image != Image::Invalid)
            {
                // Destroy the previous image object, as it might be outdated.
                device.destroy_image(entry->image);
            }

            ice::Data const metadata_data = co_await request->resource().load_metadata();
            if (metadata_data.location == nullptr)
            {
                request->resolve({ .result = ice::AssetRequestResult::Error });
                continue;
            }

            ice::Metadata const meta = ice::meta_load(metadata_data);

            ice::i32 image_format;
            ice::vec2i size;

            [[maybe_unused]]
            bool valid_data = ice::meta_read_int32(meta, "texture.format"_sid, image_format);
            valid_data &= ice::meta_read_int32(meta, "texture.size.x"_sid, size.x);
            valid_data &= ice::meta_read_int32(meta, "texture.size.y"_sid, size.y);

            [[maybe_unused]]
            ice::Data const* d = reinterpret_cast<ice::Data const*>(request->data().location);

            // Creates the image object
            ImageInfo image_info{
                .type = ImageType::Image2D,
                .format = (ImageFormat) image_format,
                .usage = ImageUsageFlags::Sampled | ImageUsageFlags::TransferDst,
                .width = (ice::u32) size.x,
                .height = (ice::u32) size.y,
            };
            Image image = device.create_image(image_info, {});
            if (image == Image::Invalid)
            {
                request->resolve({ .result = AssetRequestResult::Error });
                co_return;
            }

            ice::render::Buffer buffer = device.create_buffer(BufferType::Transfer, size.x * size.y);
            BufferUpdateInfo const buffer_updates[]{
                BufferUpdateInfo{
                    .buffer = buffer,
                    .data = *d,
                    .offset = 0
                }
            };

            device.update_buffers(buffer_updates);

            // RenderCommands& api = device.get_commands();
            // CommandBuffer const cmds = co_await params.frame_transfer;

            // api.update_texture_v2(
            //     cmds,
            //     image,
            //     buffer,
            //     ice::vec2u{ size }
            // );

            // co_await params.frame_end;

            // device.destroy_buffer(data_buffer);

            // ICE_LOG(LogSeverity::Info, LogTag::Game, "ShaderStorage - Loaded image: {}", request->asset_name());

            // // Allocates a handle for it... (TODO: Rework?)
            // ice::Memory const result = request->allocate(ice::size_of<ice::render::Image>);
            // *reinterpret_cast<Image*>(result.location) = image;

            // // Save the image handle
            // ice::hashmap::set(_loaded_images, image_hash, { .image = image });

            // // Reslove the request (will resume all awaiting tasks)
            // request->resolve({ .resolver = this, .result = AssetRequestResult::Success, .memory = result });

            // // Get the next queued request
            // request = update.assets.aquire_request(ice::render::AssetType_Texture2D, AssetState::Runtime);
        }

        co_return;
    }

} // namespace ice::gfx
