/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/gfx/gfx_stage_registry.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::gfx
{

    class IceGfxStageRegistry : public ice::gfx::GfxStageRegistry
    {
    public:
        IceGfxStageRegistry(ice::Allocator& alloc) noexcept;
        ~IceGfxStageRegistry() noexcept override = default;

        void register_stage(
            ice::StringID_Arg key,
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void remove_stage(
            ice::StringID_Arg key
        ) noexcept override;

        bool query_stages(
            ice::Span<ice::StringID> stage_keys,
            ice::Array<ice::gfx::GfxStage*>& out_stages
        ) const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::gfx::GfxStage*> _stages;
    };

    IceGfxStageRegistry::IceGfxStageRegistry(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _stages{ _allocator }
    {
    }

    void IceGfxStageRegistry::register_stage(
        ice::StringID_Arg key,
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        ice::hashmap::set(_stages, ice::hash(key), stage);
    }

    void IceGfxStageRegistry::remove_stage(
        ice::StringID_Arg key
    ) noexcept
    {
        ice::hashmap::remove(_stages, ice::hash(key));
    }

    bool IceGfxStageRegistry::query_stages(
        ice::Span<ice::StringID> stage_keys,
        ice::Array<ice::gfx::GfxStage*>& out_stages
    ) const noexcept
    {
        bool result = true;
        for (ice::StringID_Arg key : stage_keys)
        {
            ice::gfx::GfxStage* const* stage_ptr = ice::hashmap::try_get(_stages, ice::hash(key));
            ice::array::push_back(out_stages, stage_ptr == nullptr ? nullptr : *stage_ptr);
            result |= stage_ptr != nullptr;
        }
        return result;
    }

    auto create_stage_registry(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxStageRegistry>
    {
        return ice::make_unique<IceGfxStageRegistry>(alloc, alloc);
    }

} // namespace ice::gfx
