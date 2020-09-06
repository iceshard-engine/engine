#include "iceshard_mesh_loader.hxx"

#include <iceshard/renderer/render_buffers.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_model.hxx>

#include <core/memory.hxx>
#include <core/pod/algorithm.hxx>

namespace iceshard
{

    MeshLoader::MeshLoader(
        core::allocator& alloc,
        asset::AssetSystem& asset_system,
        iceshard::renderer::RenderSystem& render_system
    ) noexcept
        : _allocator{ alloc }
        , _asset_system{ asset_system }
        , _render_system{ render_system }
        , _data_buffers { _allocator }
        , _available_ranges{ _allocator }
        , _loaded_meshes{ _allocator }
    {
    }

    MeshLoader::~MeshLoader() noexcept
    {
    }

    auto MeshLoader::get_mesh(core::stringid_arg_type mesh_name) noexcept -> MeshInfo
    {
        auto const mesh_hash = core::hash(mesh_name);

        if (core::pod::hash::has(_loaded_meshes, mesh_hash) == false)
        {
            asset::AssetData asset_data;

            auto const load_state = _asset_system.load(asset::Asset{ mesh_name, asset::AssetType::Mesh }, asset_data);
            IS_ASSERT(load_state != asset::AssetStatus::Invalid, "Invalid mesh name {}!", mesh_name);

            BufferRange buffer_range[2]{ { .buffer_index = -1 }, { .buffer_index = -1 } };
            for (auto const& candidate_range : _available_ranges)
            {
                int32_t const range_index = candidate_range.index_buffer ? 0 : 1;
                BufferRange& range = buffer_range[range_index];

                if (range.buffer_index == -1)
                {
                    if (asset_data.content.size() <= candidate_range.buffer_size)
                    {
                        range = candidate_range;
                    }
                }
                else
                {
                    if (asset_data.content.size() <= candidate_range.buffer_size && candidate_range.buffer_size < range.buffer_size)
                    {
                        range = candidate_range;
                    }
                }
            }

            if (buffer_range[0].buffer_index == -1)
            {
                BufferRange range;
                range.index_buffer = true;
                range.buffer_size = 1024 * 1024 * 32; // 64 MB vertex buffer
                range.buffer_offset = 0;
                range.buffer_index = core::pod::array::size(_data_buffers);

                auto const buffer = iceshard::renderer::create_buffer(
                    iceshard::renderer::api::BufferType::IndexBuffer,
                    range.buffer_size
                );

                core::pod::array::push_back(
                    _data_buffers,
                    buffer
                );

                buffer_range[0] = range;
            }
            else
            {
                core::pod::array::resize(
                    _available_ranges,
                    core::pod::remove_if(_available_ranges, [target_range = buffer_range[0]](BufferRange const& range) noexcept
                        {
                            return target_range.buffer_index == range.buffer_index
                                && target_range.buffer_offset == range.buffer_offset;
                        })
                );
            }

            if (buffer_range[1].buffer_index == -1)
            {
                BufferRange range;
                range.index_buffer = false;
                range.buffer_size = 1024 * 1024 * 128; // 64 MB vertex buffer
                range.buffer_offset = 0;
                range.buffer_index = core::pod::array::size(_data_buffers);

                auto const buffer = iceshard::renderer::create_buffer(
                    iceshard::renderer::api::BufferType::IndexBuffer,
                    range.buffer_size
                );

                core::pod::array::push_back(
                    _data_buffers,
                    buffer
                );

                buffer_range[1] = range;
            }
            else
            {
                core::pod::array::resize(
                    _available_ranges,
                    core::pod::remove_if(_available_ranges, [target_range = buffer_range[1]](BufferRange const& range) noexcept
                        {
                            return target_range.buffer_index == range.buffer_index
                                && target_range.buffer_offset == range.buffer_offset;
                        })
                );
            }

            {
                renderer::api::Buffer mapped_buffers[] = {
                    _data_buffers[buffer_range[0].buffer_index],
                    _data_buffers[buffer_range[1].buffer_index]
                };
                renderer::api::DataView mapped_views[2];

                auto mapped_buffers_array = core::pod::array::create_view(mapped_buffers);
                auto mapped_views_array = core::pod::array::create_view(mapped_views);

                auto const* const model = reinterpret_cast<renderer::v1::Model const*>(asset_data.content.data());

                renderer::map_buffers(mapped_buffers_array, mapped_views_array);
                memcpy(
                    core::memory::utils::pointer_add(mapped_views_array[0].data, buffer_range[0].buffer_offset),
                    model->indice_data,
                    model->indice_data_size
                );

                memcpy(
                    core::memory::utils::pointer_add(mapped_views_array[1].data, buffer_range[1].buffer_offset),
                    model->vertice_data,
                    model->vertice_data_size
                );
                renderer::unmap_buffers(mapped_buffers_array);

                core::pod::hash::set(_loaded_meshes, mesh_hash,
                    MeshInfo{
                        .vertice_buffer = mapped_buffers[1],
                        .indice_buffer = mapped_buffers[0],
                        .vertice_buffer_offset = buffer_range[1].buffer_offset,
                        .indice_buffer_offset = buffer_range[0].buffer_offset,
                    }
                );

                buffer_range[0].buffer_offset += model->indice_data_size;
                buffer_range[0].buffer_size -= model->indice_data_size;
                buffer_range[1].buffer_offset += model->vertice_data_size;
                buffer_range[1].buffer_size -= model->vertice_data_size;
            }

            core::pod::array::push_back(_available_ranges, buffer_range[0]);
            core::pod::array::push_back(_available_ranges, buffer_range[1]);
        }

        static MeshInfo const dummy_mesh_info;
        return core::pod::hash::get(_loaded_meshes, mesh_hash, dummy_mesh_info);
    }

} // namespace iceshard
