#pragma once
#include <ice/stringid.hxx>
//#include <ice/pod/array.hxx>
//#include <ice/engine_frame.hxx>
//#include <ice/gfx/gfx_pass.hxx>
//#include <ice/memory/proxy_allocator.hxx>

namespace ice::gfx
{

    class GfxStage;

    struct IceGfxStage
    {
        ice::StringID name;
        ice::gfx::GfxStage* stage;
    };

}
//
//    class IceGfxStageDescription
//    {
//    public:
//        IceGfxStageDescription(ice::Allocator& alloc) noexcept;
//        ~IceGfxStageDescription() noexcept;
//
//        bool has_work() const noexcept;
//
//        bool is_complete(ice::pod::Hash<ice::gfx::IceGfxStage> const& stages) noexcept;
//
//        bool contains_any(ice::Span<ice::StringID const> stage_names) const noexcept;
//
//        bool has_dependency(ice::StringID_Arg name) const noexcept;
//
//        void add_stage(
//            ice::StringID_Arg name,
//            ice::Span<ice::StringID const> dependencies
//        ) noexcept;
//
//        void clear() noexcept;
//
//        void execute_all(
//            ice::EngineFrame const& frame,
//            ice::render::CommandBuffer cmds,
//            ice::render::RenderCommands& api,
//            ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//        ) const noexcept;
//
//    private:
//        ice::memory::ProxyAllocator _allocator;
//
//        struct Entry
//        {
//            ice::StringID name;
//            ice::u32 dependency_offset = 0;
//            ice::u32 dependency_count = 0;
//        };
//
//        ice::pod::Array<Entry> _entries;
//        ice::pod::Array<ice::StringID> _dependencies;
//    };
//
//    class IceGfxPass final : public GfxDynamicPass
//    {
//    public:
//        IceGfxPass(
//            ice::Allocator& alloc
//        ) noexcept;
//        ~IceGfxPass() noexcept;
//
//        bool has_work() const noexcept;
//        bool is_complete(ice::pod::Hash<ice::gfx::IceGfxStage> const& stages) const noexcept;
//
//        void add_stage(
//            ice::StringID_Arg name,
//            ice::Span<ice::StringID const> dependencies
//        ) noexcept override;
//
//        void clear() noexcept override;
//
//        void execute_all(
//            ice::EngineFrame const& frame,
//            ice::render::CommandBuffer cmds,
//            ice::render::RenderCommands& api,
//            ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//        ) const noexcept;
//
//    private:
//        ice::memory::ProxyAllocator _allocator;
//        ice::pod::Array<ice::gfx::IceGfxStageDescription*> _stages;
//        ice::pod::Array<ice::gfx::IceGfxStageDescription*> _free_stages;
//    };
//
//} // namespace ice::gfx
