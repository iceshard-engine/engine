#pragma once
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer::data
{

    using iceshard::renderer::api::v1_1::data::Mesh;
    using iceshard::renderer::api::v1_1::data::Model;
    using iceshard::renderer::api::v1_1::data::Texture;
    using iceshard::renderer::api::v1_1::data::Vertice;

    struct ModelView
    {
        uint32_t mesh_count;
        uint32_t vertice_data_size;
        uint32_t indice_data_size;
        Mesh const* mesh_list;
        Vertice const* vertice_data;
        core::math::u16 const* indice_data;
    };


} // namespace iceshard
