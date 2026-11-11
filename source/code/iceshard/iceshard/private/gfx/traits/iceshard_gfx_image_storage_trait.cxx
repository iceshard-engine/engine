/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
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

    Trait_GfxImageStorage::Trait_GfxImageStorage(ice::Allocator& alloc, ice::TraitContext& ctx) noexcept
        : ice::Trait{ ctx }
        , ice::TraitDevUI{ {
            .category = "builtin.devui.strings/menu.category.engine-gfx|Engine/Gfx"_i18n,
            .name = "builtin.devui.strings/widget.image-storage.name|Images"_i18n,
        } }
        , _allocator{ alloc, "gfx-image-storage" }
        , _loaded_images{ _allocator }
    {
        _context.bind<&Trait_GfxImageStorage::gfx_update, Render>(ice::gfx::ShardID_RenderFrameUpdate);
        _context.bind<&Trait_GfxImageStorage::gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
    }

    void Trait_GfxImageStorage::build_content() noexcept
    {
#if ICE_WITH_DEVUI
        ImGui::TextT("Loaded image count: {}", _loaded_images.size());
        for (GfxImageEntry& entry : _loaded_images.values())
        {
            ice::String name = ice::stringid_hint(entry.asset.name());
            if (ImGui::TreeNodeEx(name.data()))
            {
                ice::Resource const* const resource = entry.asset.resource();
                if (resource == nullptr)
                {
                    ImGui::TextUnformatted("resource: <runtime-only>");
                }
                else
                {
                    ImGui::TextT("resource: {}", resource->origin());
                }

                ImGui::Checkbox("Preview", &entry.image_preview);
                if (entry.image_preview)
                {
                    ImGui::Begin(name, &entry.image_preview, ImGuiWindowFlags_AlwaysAutoResize);

                    ImTextureRef const ref{ static_cast<ImTextureID>(entry.image) };
                    ImGui::Image(ref, { static_cast<float>(entry.info.width), static_cast<float>(entry.info.height) });
                    ImGui::End();
                }
                ImGui::TreePop();
            }
        }
#endif
    }

    auto Trait_GfxImageStorage::on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<>
    {
        GfxImageEntry* entry = _loaded_images.try_get(ice::hash(asset.name()));
        ICE_ASSERT_CORE(entry != nullptr);
        entry->released = true; // Mark as released
        co_return;
    }

    auto Trait_GfxImageStorage::gfx_update(
        ice::gfx::RenderFrameUpdate const& params,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        ice::AssetRequest* request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
        ice::AssetResolveData resolve_success{ .resolver = this, .result = AssetRequestResult::Success };

        ice::u32 upload_count = 0;
        ice::AssetRequest* uploading_requests[4];
        ice::render::Buffer transfer_buffers[4];
        ice::render::BufferUpdateInfo update_infos[4];
        ice::render::Image created_images[4];
        ice::render::ImageInfo created_extents[4];

        while(request != nullptr && upload_count < 4)
        {
            ice::AssetState const state = request->state();
            ICE_ASSERT_CORE(state == AssetState::Loaded); // The image needs to be loaded.

            ice::StringID const nameid = request->asset_name();
            GfxImageEntry* entry = _loaded_images.try_get(ice::hash(nameid));
            if (entry && entry->image != Image::Invalid)
            {
                Image& image = request->new_result<Image>(resolve_success);
                image = entry->image;

                // Return the existing image object
                // TODO: Handle updates in later version.
                ice::Asset asset_result = request->resolve(resolve_success);

                // Get the next queued request
                request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
                continue;
            }

            ICE_ASSERT_CORE(entry == nullptr || entry->released);

            // The render device
            RenderDevice& device = params.context.device();

            // Get the image data
            Data const request_data = request->data();
            ImageInfo const* image_info = static_cast<ImageInfo const*>(request_data.location);
            Data const image_data = image_info->image_data();

            // Create image and transfer buffer
            created_images[upload_count] = device.create_image(*image_info, image_data);
            if (created_images[upload_count] == Image::Invalid)
            {
                request->resolve({ .result = AssetRequestResult::Error });
                continue;
            }

            // Store upload data
            uploading_requests[upload_count] = request;

            created_extents[upload_count] = *image_info;

            transfer_buffers[upload_count] = device.create_buffer(BufferType::Transfer, ice::u32(image_data.size.value));
            update_infos[upload_count] = BufferUpdateInfo{
                .buffer = transfer_buffers[upload_count],
                .data = image_data,
                .offset = 0_B
            };

            // Increase the upload count
            upload_count += 1;

            // Get the next queued request
            request = assets.aquire_request(ice::render::AssetCategory_Texture2D, AssetState::Runtime);
        }

        if (upload_count > 0)
        {
            // The render device
            RenderDevice& device = params.context.device();
            device.update_buffers({ update_infos, upload_count });

            TaskStage const stage_transfer = params.stages.frame_transfer;
            TaskStage const stage_end = params.stages.frame_end;

            RenderCommands& api = device.get_commands();
            CommandBuffer const cmds = co_await stage_transfer;

            ImageBarrier barriers[4]{ };
            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                barriers[idx].image = created_images[idx];
                barriers[idx].source_layout = ImageLayout::Undefined;
                barriers[idx].destination_layout = ImageLayout::TransferDstOptimal;
                barriers[idx].source_access = AccessFlags::None;
                barriers[idx].destination_access = AccessFlags::TransferWrite;
            }

            api.pipeline_image_barrier(
                cmds,
                PipelineStage::BottomOfPipe,
                PipelineStage::Transfer,
                { barriers, upload_count }
            );

            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                api.update_texture_v2(
                    cmds,
                    created_images[idx],
                    transfer_buffers[idx],
                    { created_extents[idx].width, created_extents[idx].height }
                );
            }

            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                barriers[idx].image = created_images[idx];
                barriers[idx].source_layout = ImageLayout::TransferDstOptimal;
                barriers[idx].destination_layout = ImageLayout::ShaderReadOnly;
                barriers[idx].source_access = AccessFlags::TransferWrite;
                barriers[idx].destination_access = AccessFlags::ShaderRead;
            }

            api.pipeline_image_barrier(
                cmds,
                PipelineStage::Transfer,
                PipelineStage::FramentShader,
                { barriers, upload_count }
            );

            co_await stage_end;

            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                device.destroy_buffer(transfer_buffers[idx]);
            }

            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                ice::AssetRequest* const uploaded_request = uploading_requests[idx];
                ICE_LOG(
                    LogSeverity::Info, LogTag::Game,
                    "TextureStorage - Loaded image: {}",
                    uploaded_request->asset_name()
                );

                Image& image = uploaded_request->new_result<Image>(resolve_success);
                image = created_images[idx];

                ice::u64 const asset_hash = ice::hash(uploaded_request->asset_name());
                ice::Asset asset = uploaded_request->resolve(resolve_success);

                // Make the result
                GfxImageEntry image_entry{ .asset = ice::move(asset), .image = created_images[idx] };
#if ICE_WITH_DEVUI
                image_entry.info = created_extents[idx];
#endif

                // Save the image handle
                _loaded_images.set(
                    asset_hash,
                    ice::move(image_entry)
                );
            }
        }
        co_return;
    }

    auto Trait_GfxImageStorage::gfx_shutdown(
        ice::render::RenderDevice& device
    ) noexcept -> ice::Task<>
    {
        for (ice::gfx::GfxImageEntry& entry : _loaded_images.values())
        {
            device.destroy_image(entry.image);
            entry.asset.release();
        }

        _loaded_images.clear();
        co_return;
    }

} // namespace ice::gfx
