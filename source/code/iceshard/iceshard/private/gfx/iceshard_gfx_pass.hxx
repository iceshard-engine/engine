#pragma once
#include <ice/pod/array.hxx>
#include <ice/engine_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class IceGfxStageBatch
    {
    public:
        IceGfxStageBatch(ice::Allocator& alloc) noexcept;
        ~IceGfxStageBatch() noexcept;

        bool has_work() const noexcept;

        bool contains_any(ice::Span<ice::StringID const> stage_names) const noexcept;

        bool has_dependency(ice::StringID_Arg name) const noexcept;

        void add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::StringID const> dependencies,
            ice::gfx::GfxStage* stage
        ) noexcept;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmd_buffer,
            ice::render::RenderCommands& cmds
        ) noexcept;

        void clear() noexcept;

    private:
        struct Entry
        {
            ice::StringID name;
            ice::gfx::GfxStage* stage;
            ice::u32 dependency_offset = 0;
            ice::u32 dependency_count = 0;
        };

        ice::pod::Array<Entry> _entries;
        ice::pod::Array<ice::StringID> _dependencies;
    };

    class IceGfxPass final : public GfxPass
    {
    public:
        IceGfxPass(
            ice::Allocator& alloc
        ) noexcept;
        ~IceGfxPass() noexcept;

        bool has_work() const noexcept;
        bool has_update_work() const noexcept;

        void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::StringID const> dependencies,
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void add_update_stage(
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void record_update_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmd_buffer,
            ice::render::RenderCommands& cmds
        ) noexcept;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmd_buffer,
            ice::render::RenderCommands& cmds
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Array<ice::gfx::IceGfxStageBatch*> _stage_batches;
        ice::pod::Array<ice::gfx::IceGfxStageBatch*> _free_batches;
        ice::pod::Array<ice::gfx::GfxStage*> _update_stages;
    };

} // namespace ice::gfx
