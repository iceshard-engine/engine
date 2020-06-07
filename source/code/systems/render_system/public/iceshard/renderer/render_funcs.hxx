#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/array.hxx>

#include <asset_system/asset.hxx>
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer
{

    void check() noexcept;

    auto create_texture(
        core::stringid_arg_type name,
        asset::AssetData texture_data
    ) noexcept -> api::Texture;

    void create_command_buffers(
        api::CommandBufferType type,
        core::pod::Array<api::CommandBuffer>& result
    ) noexcept;

} // namespace iceshard::renderer::api
