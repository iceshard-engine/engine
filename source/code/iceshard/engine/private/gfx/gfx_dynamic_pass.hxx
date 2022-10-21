#pragma once
#include <ice/container/array.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class GfxDynamicPassStageGroup final
    {
    public:
        GfxDynamicPassStageGroup(ice::Allocator& alloc) noexcept;
        ~GfxDynamicPassStageGroup() noexcept = default;

        auto stage_count() const noexcept -> ice::u32;

        bool has_work() const noexcept;

        bool contains_any(ice::Span<ice::StringID const> stage_names) const noexcept;

        bool has_dependency(ice::StringID_Arg name) const noexcept;

        void add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::StringID const> dependencies
        ) noexcept;

        void clear() noexcept;

        void query_stage_order(
            ice::Array<ice::StringID_Hash>& stage_order_out
        ) const noexcept;

    private:
        ice::Allocator& _allocator;

        struct Entry
        {
            ice::StringID name;
            ice::u32 dependency_offset = 0;
            ice::u32 dependency_count = 0;
        };

        ice::Array<Entry> _entries;
        ice::Array<ice::StringID> _dependencies;
    };

    class IceGfxDynamicPass final : public GfxDynamicPass
    {
    public:
        IceGfxDynamicPass(
            ice::Allocator& alloc
        ) noexcept;
        ~IceGfxDynamicPass() noexcept;

        auto stage_count() const noexcept -> ice::u32;

        bool has_work() const noexcept;

        void add_stage(
            ice::StringID_Arg stage_name,
            ice::Span<ice::StringID const> dependencies
        ) noexcept override;

        void clear() noexcept override;

        void query_stage_order(ice::Array<ice::StringID_Hash>& stage_order_out) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::Array<ice::gfx::GfxDynamicPassStageGroup*> _stages;
        ice::Array<ice::gfx::GfxDynamicPassStageGroup*> _free_stages;
    };

} // namespace ice::gfx
