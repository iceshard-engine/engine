#pragma once
#include <core/allocator.hxx>
#include <asset_system/asset.hxx>
#include <asset_system/asset_system.hxx>

#include <iceshard/renderer/render_api.hxx>
#include <iceshard/renderer/render_system.hxx>

namespace iceshard
{

    struct BufferRange
    {
        bool index_buffer;
        int32_t buffer_index;
        uint32_t buffer_offset;
        uint32_t buffer_size;
    };

    struct MeshInfo
    {
        renderer::api::Buffer vertice_buffer;
        renderer::api::Buffer indice_buffer;
        uint32_t vertice_buffer_offset;
        uint32_t indice_buffer_offset;
    };

    class MeshLoader
    {
    public:
        MeshLoader(
            core::allocator& alloc,
            asset::AssetSystem& _asset_system,
            iceshard::renderer::RenderSystem& render_system
        ) noexcept;

        ~MeshLoader() noexcept;

        auto get_mesh(core::stringid_arg_type mesh_name) noexcept -> MeshInfo;

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;
        iceshard::renderer::RenderSystem& _render_system;

        core::pod::Array<renderer::api::Buffer> _data_buffers;
        core::pod::Array<BufferRange> _available_ranges;

        core::pod::Hash<MeshInfo> _loaded_meshes;
    };

} // namespace iceshard
