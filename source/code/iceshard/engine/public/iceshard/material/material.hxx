#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/renderer/render_api.hxx>

namespace iceshard
{

    struct Material
    {
        core::stringid_type texture_normal;
        core::stringid_type texture_diffuse;
        core::stringid_type texture_specular;
        core::stringid_type shader_vertex;
        core::stringid_type shader_fragment;
    };

    struct MaterialResources
    {
        iceshard::renderer::api::Pipeline pipeline;
        iceshard::renderer::api::ResourceSet resource;
    };

} // namespace iceshard
