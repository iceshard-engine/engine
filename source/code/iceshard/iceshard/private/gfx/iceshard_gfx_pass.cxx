#include "iceshard_gfx_pass.hxx"
#include <ice/gfx/gfx_stage.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxStageBatch::IceGfxStageBatch(ice::Allocator& alloc) noexcept
        : _entries{ alloc }
        , _dependencies{ alloc }
    {
        ice::pod::array::reserve(_entries, 10);
        ice::pod::array::reserve(_dependencies, 25);
    }

    IceGfxStageBatch::~IceGfxStageBatch() noexcept
    {
    }

    bool IceGfxStageBatch::has_work() const noexcept
    {
        return ice::pod::array::empty(_entries) == false;
    }

    bool IceGfxStageBatch::contains_any(
        ice::Span<ice::StringID const> stage_names
    ) const noexcept
    {
        bool found = false;
        for (Entry const& entry : _entries)
        {
            for (ice::StringID const& stage_name : stage_names)
            {
                found |= ice::stringid_hash(entry.name) == ice::stringid_hash(stage_name);
            }
        }
        return found;
    }

    bool IceGfxStageBatch::has_dependency(
        ice::StringID_Arg name
    ) const noexcept
    {
        bool found = false;
        for (Entry const& entry : _entries)
        {
            ice::Span<StringID const> entry_deps{ ice::pod::array::begin(_dependencies) + entry.dependency_offset, entry.dependency_count };

            for (ice::StringID const& dependency : entry_deps)
            {
                found |= ice::stringid_hash(dependency) == ice::stringid_hash(name);
            }
        }
        return found;
    }

    void IceGfxStageBatch::add_stage(
        ice::StringID_Arg name,
        ice::Span<ice::StringID const> dependencies,
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        ice::pod::array::push_back(_entries,
            Entry{
                .name = name,
                .stage = stage,
                .dependency_offset = ice::size(_dependencies),
                .dependency_count = ice::size(dependencies)
            }
        );

        ice::pod::array::push_back(_dependencies, dependencies);
    }

    void IceGfxStageBatch::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept
    {
        for (Entry const& entry : _entries)
        {
            entry.stage->record_commands(frame, cmd_buffer, cmds);
        }
    }

    void IceGfxStageBatch::clear() noexcept
    {
        ice::pod::array::clear(_entries);
        ice::pod::array::clear(_dependencies);
    }

    IceGfxPass::IceGfxPass(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _stage_batches{ _allocator }
    {
        ice::pod::array::reserve(_stage_batches, 5);
        ice::pod::array::push_back(
            _stage_batches,
            _allocator.make<IceGfxStageBatch>(_allocator)
        );
    }

    IceGfxPass::~IceGfxPass() noexcept
    {
        for (IceGfxStageBatch* batch : _stage_batches)
        {
            _allocator.destroy(batch);
        }
    }

    bool IceGfxPass::has_work() const noexcept
    {
        bool has_work = false;
        for (IceGfxStageBatch* batch : _stage_batches)
        {
            has_work |= batch->has_work();
        }
        return has_work;
    }

    void IceGfxPass::add_stage(
        ice::StringID_Arg name,
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        ICE_ASSERT(
            ice::pod::array::empty(_stage_batches) == false,
            "There is not stage batch available to add a new gfx stage!"
        );

        IceGfxStageBatch* const batch = ice::pod::array::back(_stage_batches);
        if (batch != nullptr)
        {
            batch->add_stage(name, { }, stage);
        }
    }

    void IceGfxPass::add_stage(
        ice::StringID_Arg name,
        ice::Span<ice::StringID const> dependencies,
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        IceGfxStageBatch* target_batch = nullptr;

        {
            auto candidate_batch = ice::pod::array::rbegin(_stage_batches);
            auto const end_batch = ice::pod::array::rend(_stage_batches);

            while (candidate_batch != end_batch)
            {
                if ((*candidate_batch)->contains_any(dependencies) == false)
                {
                    target_batch = *candidate_batch;
                    candidate_batch += 1;
                }
                else
                {
                    candidate_batch = end_batch;
                }
            }
        }

        if (target_batch == nullptr)
        {
            target_batch = _allocator.make<IceGfxStageBatch>(_allocator);
            ice::pod::array::push_back(_stage_batches, target_batch);
        }

        if (target_batch->has_dependency(name))
        {
            IceGfxStageBatch* new_target_batch = _allocator.make<IceGfxStageBatch>(_allocator);

            ice::u32 const batch_count = ice::size(_stage_batches);
            ice::u32 target_batch_idx = 0;
            while(_stage_batches[target_batch_idx] != target_batch)
            {
                ++target_batch_idx;
            }

            // Add an empty dummy entry
            ice::pod::array::push_back(_stage_batches, nullptr);

            // Move everything down by one
            for (ice::u32 idx = batch_count; idx > target_batch_idx; --idx)
            {
                _stage_batches[idx] = _stage_batches[idx - 1];
            }

            _stage_batches[target_batch_idx] = new_target_batch;
            target_batch = new_target_batch;
        }

        target_batch->add_stage(name, dependencies, stage);
    }

    void IceGfxPass::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept
    {
        for (IceGfxStageBatch* batch : _stage_batches)
        {
            batch->record_commands(
                frame,
                cmd_buffer,
                cmds
            );
            batch->clear();
        }
    }

} // namespace ice::gfx
