/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_shader_storage_trait.hxx"

#include <ice/asset_storage.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_types_mappers.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/render/render_device.hxx>
#include <ice/resource.hxx>
#include <ice/shard_container.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/world/world_updater.hxx>

namespace ice::gfx
{

    auto internal_load_shader(
        ice::AssetRequest* request,
        ice::TaskScheduler& final_thread,
        ice::HashMap<ice::gfx::GfxShaderEntry>& entries
    ) noexcept -> ice::Task<>
    {
        request->data();

        co_await final_thread;
    }

    Trait_GfxShaderStorage::Trait_GfxShaderStorage(ice::TraitContext& ctx, ice::Allocator& alloc) noexcept
        : ice::Trait{ ctx }
        , ice::TraitDevUI{ {.category="Engine/Gfx", .name="Shaders"} }
        , _loaded_shaders{ alloc }
    {
        _context.bind<&Trait_GfxShaderStorage::gfx_update, Render>(ice::gfx::ShardID_RenderFrameUpdate);
        _context.bind<&Trait_GfxShaderStorage::gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
        _context.bind<&Trait_GfxShaderStorage::on_asset_loaded>("iceshard:shaders-internal:loaded`ice::Asset"_shardid);
    }

    void Trait_GfxShaderStorage::build_content() noexcept
    {
        ImGui::TextT("Loaded shaders: {}", ice::hashmap::count(_loaded_shaders));
        if (ImGui::BeginCombo("##shader-list", "Shader to preview", ImGuiComboFlags_WidthFitPreview))
        {
            for (GfxShaderEntry& entry : ice::hashmap::values(_loaded_shaders))
            {
                ice::URI const uri = entry.asset.uri();
                ImGui::Selectable(uri.path()._data, &entry.devui_loaded);
            }
            ImGui::EndCombo();
        }

        for (GfxShaderEntry& entry : ice::hashmap::values(_loaded_shaders))
        {
            if (entry.devui_loaded)
            {
                if (entry.asset.available(ice::AssetState::Raw))
                {
                    ice::Data const rawdata = ice::wait_for_result(entry.asset.data(ice::AssetState::Raw));
                    ImGui::TextUnformatted((char const*)rawdata.location,(char const*)rawdata.location + rawdata.size.value);
                    ImGui::Separator();
                }
            }
        }
    }

    auto Trait_GfxShaderStorage::on_asset_released(ice::Asset const &asset) noexcept -> ice::Task<>
    {
        GfxShaderEntry* entry = ice::hashmap::try_get(_loaded_shaders, ice::hash(asset.name()));
        ICE_ASSERT_CORE(entry != nullptr);
        entry->released = true; // Mark as released
        co_return;
    }

    auto Trait_GfxShaderStorage::on_asset_loaded(ice::Asset asset) noexcept -> ice::Task<>
    {
        ICE_LOG(LogSeverity::Info, LogTag::Game, "ShaderStorage - Loaded shader: {}", asset.resource()->name());
        co_return;
    }

    auto Trait_GfxShaderStorage::gfx_update(
        ice::gfx::RenderFrameUpdate const& update,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
    {
        // Handle up to 4 requests at the same time each frame.
        ice::AssetRequest* request = assets.aquire_request(ice::render::AssetCategory_Shader, AssetState::Runtime);
        while(request != nullptr)
        {
            ice::AssetState const state = request->state();
            ICE_ASSERT_CORE(state == AssetState::Loaded); // The shader needs to be loaded.

            ice::u64 const shader_hash = ice::hash(request->asset_name());
            GfxShaderEntry* entry = ice::hashmap::try_get(_loaded_shaders, shader_hash);
            ICE_ASSERT_CORE(entry == nullptr || entry->released);

            using namespace ice::render;
            RenderDevice& device = update.context.device();

            if (entry && entry->shader != Shader::Invalid)
            {
                // Destroy the previous shader object, as it might be outdated.
                device.destroy_shader(entry->shader);
            }

            ice::Data const* d = reinterpret_cast<ice::Data const*>(request->data().location);

            // Creates the shader object
            Shader shader = device.create_shader({ .shader_data = *d });
            if (shader == Shader::Invalid)
            {
                request->resolve({ .result = AssetRequestResult::Error });
                co_return;
            }

            // Allocates a handle for it... (TODO: Rework?)
            ice::Memory const result = request->allocate(ice::size_of<ice::render::Shader>);
            *reinterpret_cast<Shader*>(result.location) = shader;

            // Reslove the request (will resume all awaiting tasks)
            ice::Asset asset = request->resolve({ .resolver = this, .result = AssetRequestResult::Success, .memory = result });
            send("iceshard:shaders-internal:loaded"_shardid, asset);

            // Save the shader handle
            ice::hashmap::set(_loaded_shaders, shader_hash, { .asset = ice::move(asset), .shader = shader, });

            // Get the next queued request
            request = assets.aquire_request(ice::render::AssetCategory_Shader, AssetState::Runtime);
        }

        co_return;
    }

    auto Trait_GfxShaderStorage::gfx_shutdown(ice::render::RenderDevice& device) noexcept -> ice::Task<>
    {
        for (ice::gfx::GfxShaderEntry& entry : ice::hashmap::values(_loaded_shaders))
        {
            device.destroy_shader(entry.shader);
            entry.asset.release();
        }

        ice::hashmap::clear(_loaded_shaders);
        co_return;
    }

} // namespace ice::gfx
