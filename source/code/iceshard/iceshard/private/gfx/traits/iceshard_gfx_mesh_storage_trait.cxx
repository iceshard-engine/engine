#include "iceshard_gfx_mesh_storage_trait.hxx"

#include <ice/asset_storage.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/engine_types_mappers.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/render/render_device.hxx>

namespace ice::gfx
{

    static constexpr ice::vec2f Box2DVertices[]
    {
        {0.0f + 0.0f, 0.0f + 0.0f},
        {1.0f + 0.0f, 0.0f + 0.0f},
        {1.0f + 0.0f, 1.0f + 0.0f},
        {0.0f + 0.0f, 1.0f + 0.0f},
    };
    static constexpr ice::u16 Box2DIndices[]
    {
        0, 1, 2, 0, 2, 3
    };

    struct GfxStaticMesh : ice::TraitAttachedObject, ice::AssetDataBinding
    {
        GfxStaticMesh(
            ice::Allocator& alloc,
            ice::Memory const& allocated_data
        ) noexcept
            : AssetDataBinding{ .state = AssetState::Baked }
            , _allocator{ alloc }
            , _allocated_data{ allocated_data }
        {
            content = ice::data_view(_allocated_data);
        }
        ~GfxStaticMesh() noexcept override
        {
            _allocator.deallocate(_allocated_data);
        }

        ice::Allocator& _allocator;
        ice::Asset _asset; // keeps the asset alive always
        ice::Memory _allocated_data;
    };

    auto make_box2d_mesh(
        ice::Allocator& alloc,
        ice::AssetStorage& assets
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxStaticMesh>
    {
        ice::vec2f *vertices;
        ice::i16* indices;
        ice::render::MeshInfo* info;

        ice::ChunkedAllocRequest request{};
        request.include(info, 1);
        request.include(vertices, ice::count(Box2DVertices));
        request.include(indices, ice::count(Box2DIndices));
        ice::Memory memory = alloc.allocate(request);

        // Store the vertice data
        ice::memcpy(vertices, ice::data_view(Box2DVertices));
        ice::memcpy(indices, ice::data_view(Box2DIndices));

        // Store number of vertices for the box
        info->vertex_count = ice::count_of(Box2DVertices);
        info->index_count = ice::count_of(Box2DIndices);
        info->data.set_from_pointer(vertices);

        ice::UniquePtr<ice::gfx::GfxStaticMesh> result = ice::make_unique<ice::gfx::GfxStaticMesh>(
            alloc, alloc, memory
        );
        result->_asset = assets.bind_data(ice::render::AssetCategory_Mesh, "box-2d", *result);
        return result;
    }

    Trait_GfxMeshStorage::Trait_GfxMeshStorage(ice::Allocator& alloc, ice::TraitContext& ctx) noexcept
        : ice::Trait{ ctx }
        , ice::TraitDevUI{ {
            .category = "builtin.devui.strings/menu.category.engine-gfx|Engine/Gfx"_i18n,
            .name = "builtin.devui.strings/widget.mesh-storage.name|Meshes"_i18n,
        } }
        , _allocator{ alloc, "gfx-mesh-storage" }
        , _loaded_meshes{ _allocator }
    {
        _context.bind<&Trait_GfxMeshStorage::gfx_update, Render>(ice::gfx::ShardID_RenderFrameUpdate);
        _context.bind<&Trait_GfxMeshStorage::gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
    }

    auto Trait_GfxMeshStorage::activate(ice::WorldStateParams const& params) noexcept -> ice::Task<>
    {
        _context.attach(make_box2d_mesh(_allocator, params.assets));
        co_return;
    }

    auto Trait_GfxMeshStorage::deactivate(ice::WorldStateParams const& params) noexcept -> ice::Task<>
    {
        co_return;
    }

    auto Trait_GfxMeshStorage::on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<>
    {
        GfxMeshEntry* entry = _loaded_meshes.try_get(ice::hash(asset.name()));
        ICE_ASSERT_CORE(entry != nullptr);
        entry->released = true; // Mark as released
        co_return;
    }

    auto Trait_GfxMeshStorage::on_asset_loaded(ice::Asset asset) noexcept -> ice::Task<>
    {
        co_return;
    }

    void Trait_GfxMeshStorage::build_content() noexcept
    {
        ImGui::TextT("Loaded mesh count: {}", _loaded_meshes.size());
        for (GfxMeshEntry const& entry : _loaded_meshes)
        {
            ice::String name = ice::stringid_hint(entry.asset.name());
            if (ImGui::TreeNodeEx(name.data()))
            {
                ImGui::TextT("vertex count: {}", entry.mesh.vertex_count);
                ImGui::TextT("index count: {}", entry.mesh.index_count);

                ice::Resource const* const resource = entry.asset.resource();
                if (resource == nullptr)
                {
                    ImGui::TextUnformatted("resource: <runtime-only>");
                }
                ImGui::TreePop();
            }
        }
    }

    auto Trait_GfxMeshStorage::gfx_update(
        ice::gfx::RenderFrameUpdate const& params,
        ice::AssetStorage& assets
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        ice::AssetRequest* request = assets.aquire_request(ice::render::AssetCategory_Mesh, AssetState::Runtime);
        ice::AssetResolveData resolve_success{ .resolver = this, .result = AssetRequestResult::Success };

        ice::u32 upload_count = 0;
        ice::AssetRequest* uploading_requests[4];
        ice::u32 buffer_count = 0;
        ice::render::Mesh created_meshes[4];
        ice::render::BufferUpdateInfo update_infos[5];

        while(request != nullptr && upload_count < 4 && buffer_count < 4)
        {
            ice::AssetState const state = request->state();
            ICE_ASSERT_CORE(state == AssetState::Loaded); // The image needs to be loaded.

            ice::StringID const name_id = request->asset_name();
            GfxMeshEntry* entry = _loaded_meshes.try_get(ice::hash(name_id));
            if (entry && entry->mesh.vertex_buffer != Buffer::Invalid)
            {
                // If it was marked for release, restore the flag.
                entry->released = false;

                // Store existing information in the result.
                ice::render::Mesh& result = request->new_result<ice::render::Mesh>(resolve_success);

                // TODO: Before returning we might want to check if the data was updated?
                //  - I'm not sure if the asset system is currently able check if the underlying resource was changed.
                //  - It might be necessary to limit this to editor only workflows.
                result = entry->mesh;

                // Return the existing buffer object
                ice::Asset asset_result = request->resolve(resolve_success);

                // Get the next queued request
                request = assets.aquire_request(ice::render::AssetCategory_Mesh, AssetState::Runtime);
                continue;
            }

            ICE_ASSERT_CORE(entry == nullptr || entry->released);

            // The render device
            RenderDevice& device = params.context.device();

            // Get the image data
            Data const request_data = request->data();
            MeshInfo const* mesh_info = static_cast<MeshInfo const*>(request_data.location);
            Data const mesh_data{
                .location = mesh_info->data,
                .size = mesh_info->vertex_count.bytes() + mesh_info->index_count.bytes(),
                .alignment = ice::ualign::b_4,
            };

            auto make_buffer = [&](BufferType buffer_type, ice::Data const& buffer_data) noexcept -> Buffer
            {
                // Do nothing if zero data
                if (buffer_data.size <= 0_B)
                {
                    // Only consider success for index buffers
                    return Buffer::Invalid;
                }

                // Create next buffer (and optional index buffer)
                ice::render::Buffer const buffer = device.create_buffer(buffer_type, buffer_data.size.value);
                if (buffer != Buffer::Invalid)
                {
                    // Store update information
                    update_infos[buffer_count] = BufferUpdateInfo{
                        .buffer = buffer,
                        .data = buffer_data,
                        .offset = 0_B
                    };
                }
                return buffer;
            };

            ice::render::Mesh& mesh = created_meshes[upload_count];
            ice::Data const vertex_data = data_slice(mesh_data, mesh_info->vertex_count);
            ice::Data const index_data = data_slice(mesh_data, mesh_info->index_count, vertex_data.size);

            mesh.vertex_count = mesh_info->vertex_count;
            mesh.vertex_buffer = make_buffer(BufferType::Vertex, vertex_data);
            if (mesh.vertex_buffer == Buffer::Invalid)
            {
                request->resolve({ .result = AssetRequestResult::Error });
                continue;
            }

            buffer_count += 1;
            mesh.index_count = mesh_info->index_count;
            mesh.index_buffer = make_buffer(BufferType::Index, index_data);
            if (mesh.index_buffer == Buffer::Invalid && index_data.size > 0_B)
            {
                // delete the previous buffer before continuing
                device.destroy_buffer(mesh.vertex_buffer);
                buffer_count -= 1;

                request->resolve({ .result = AssetRequestResult::Error });
                continue;
            }
            // We only increase buffer count when the buffer was created
            if (index_data.size > 0_B)
            {
                buffer_count += 1;
            }

            // Increase the upload count
            uploading_requests[upload_count] = request;
            upload_count += 1;

            // Get the next queued request
            request = assets.aquire_request(ice::render::AssetCategory_Mesh, AssetState::Runtime);
        }

        if (upload_count > 0)
        {
            // The render device
            RenderDevice& device = params.context.device();
            device.update_buffers({ update_infos, buffer_count });

            for (ice::u32 idx = 0; idx < upload_count; ++idx)
            {
                ice::AssetRequest* const uploaded_request = uploading_requests[idx];
                ICE_LOG(
                    LogSeverity::Info, LogTag::Game,
                    "MeshStorage - Loaded mesh: {}",
                    uploaded_request->asset_name()
                );

                ice::render::Mesh& result = uploaded_request->new_result<ice::render::Mesh>(resolve_success);
                result = created_meshes[idx];

                ice::u64 const asset_hash = ice::hash(uploaded_request->asset_name());
                ice::Asset asset = uploaded_request->resolve(resolve_success);

                // Save the image handle
                _loaded_meshes.set(
                    asset_hash,
                    { .asset = ice::move(asset), .mesh = created_meshes[idx] }
                );
            }
        }
        co_return;
    }

    auto Trait_GfxMeshStorage::gfx_shutdown(ice::render::RenderDevice& device) noexcept -> ice::Task<>
    {
        for (ice::gfx::GfxMeshEntry& entry : _loaded_meshes.values())
        {
            device.destroy_buffer(entry.mesh.vertex_buffer);
            device.destroy_buffer(entry.mesh.index_buffer);
            entry.asset.release();
        }

        _loaded_meshes.clear();
        co_return;
    }


} // namespace ice
