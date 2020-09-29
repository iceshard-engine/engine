#pragma once
#include <core/allocator.hxx>
#include <iceshard/material/material.hxx>
#include <iceshard/material/material_system.hxx>

#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_pipeline.hxx>

#include <asset_system/asset_system.hxx>

namespace iceshard
{

    struct MaterialInfo
    {
        core::stringid_type name;
        MaterialResources resources;
    };

    class IceshardMaterialSystem : public MaterialSystem
    {
    public:
        IceshardMaterialSystem(
            core::allocator& alloc,
            asset::AssetSystem& asset_system,
            iceshard::renderer::RenderSystem& render_system
        ) noexcept;
        ~IceshardMaterialSystem() noexcept override;

        bool create_material(
            core::stringid_arg_type name,
            Material const& definition,
            iceshard::renderer::RenderPipelineLayout layout
        ) noexcept override;

        bool get_material(core::stringid_arg_type name, MaterialResources& resources) noexcept override;

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;
        iceshard::renderer::RenderSystem& _render_system;

        core::pod::Hash<MaterialInfo> _material_resources;

        iceshard::renderer::CommandBuffer _command_buffer{ iceshard::renderer::CommandBuffer::Invalid };
        iceshard::renderer::Buffer _transfer_buffer{ iceshard::renderer::Buffer::Invalid };
    };

} // namespace iceshard
