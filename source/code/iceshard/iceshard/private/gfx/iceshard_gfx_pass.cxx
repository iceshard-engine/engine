//#include "iceshard_gfx_pass.hxx"
//#include <ice/gfx/gfx_stage.hxx>
//#include <ice/assert.hxx>
//
//namespace ice::gfx
//{
//
//    IceGfxStageDescription::IceGfxStageDescription(ice::Allocator& alloc) noexcept
//        : _allocator{ alloc, "iceshard-gfx-stage-batch" }
//        , _entries{ _allocator }
//        , _dependencies{ _allocator }
//    {
//        ice::pod::array::reserve(_entries, 10);
//        ice::pod::array::reserve(_dependencies, 25);
//    }
//
//    IceGfxStageDescription::~IceGfxStageDescription() noexcept
//    {
//    }
//
//    bool IceGfxStageDescription::has_work() const noexcept
//    {
//        return ice::pod::array::empty(_entries) == false;
//    }
//
//    bool IceGfxStageDescription::is_complete(
//        ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//    ) noexcept
//    {
//        bool found_all = true;
//        for (Entry const& entry : _entries)
//        {
//            found_all &= ice::pod::hash::has(stages, ice::hash(entry.name));
//        }
//        return found_all;
//    }
//
//    bool IceGfxStageDescription::contains_any(
//        ice::Span<ice::StringID const> stage_names
//    ) const noexcept
//    {
//        bool found = false;
//        for (Entry const& entry : _entries)
//        {
//            for (ice::StringID const& stage_name : stage_names)
//            {
//                found |= ice::stringid_hash(entry.name) == ice::stringid_hash(stage_name);
//            }
//        }
//        return found;
//    }
//
//    bool IceGfxStageDescription::has_dependency(
//        ice::StringID_Arg name
//    ) const noexcept
//    {
//        bool found = false;
//        for (Entry const& entry : _entries)
//        {
//            ice::Span<StringID const> entry_deps{ ice::pod::array::begin(_dependencies) + entry.dependency_offset, entry.dependency_count };
//
//            for (ice::StringID const& dependency : entry_deps)
//            {
//                found |= ice::stringid_hash(dependency) == ice::stringid_hash(name);
//            }
//        }
//        return found;
//    }
//
//    void IceGfxStageDescription::add_stage(
//        ice::StringID_Arg name,
//        ice::Span<ice::StringID const> dependencies
//    ) noexcept
//    {
//        ice::pod::array::push_back(_entries,
//            Entry{
//                .name = name,
//                .dependency_offset = ice::size(_dependencies),
//                .dependency_count = ice::size(dependencies)
//            }
//        );
//
//        ice::pod::array::push_back(_dependencies, dependencies);
//    }
//
//    void IceGfxStageDescription::clear() noexcept
//    {
//        ice::pod::array::clear(_entries);
//        ice::pod::array::clear(_dependencies);
//    }
//
//    void IceGfxStageDescription::execute_all(
//        ice::EngineFrame const& frame,
//        ice::render::CommandBuffer cmds,
//        ice::render::RenderCommands& api,
//        ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//    ) const noexcept
//    {
//        for (Entry const& entry : _entries)
//        {
//            ice::gfx::GfxStage* stage = ice::pod::hash::get(stages, ice::hash(entry.name), IceGfxStage{ }).stage;
//            stage->record_commands(
//                frame,
//                cmds,
//                api
//            );
//        }
//    }
//
//    IceGfxPass::IceGfxPass(ice::Allocator& alloc) noexcept
//        : _allocator{ alloc, "iceshard-gfx-pass" }
//        , _stages{ _allocator }
//        , _free_stages{ _allocator }
//    {
//        ice::pod::array::reserve(_stages, 5);
//        ice::pod::array::reserve(_free_stages, 5);
//        ice::pod::array::push_back(
//            _stages,
//            _allocator.make<IceGfxStageDescription>(_allocator)
//        );
//    }
//
//    IceGfxPass::~IceGfxPass() noexcept
//    {
//        for (IceGfxStageDescription* stage_desc : _stages)
//        {
//            _allocator.destroy(stage_desc);
//        }
//        for (IceGfxStageDescription* stage_desc : _free_stages)
//        {
//            _allocator.destroy(stage_desc);
//        }
//    }
//
//    bool IceGfxPass::has_work() const noexcept
//    {
//        bool has_work = false;
//        for (IceGfxStageDescription* stage_desc : _stages)
//        {
//            has_work |= stage_desc->has_work();
//        }
//        return has_work;
//    }
//
//    bool IceGfxPass::is_complete(
//        ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//    ) const noexcept
//    {
//        bool found_all = true;
//        for (IceGfxStageDescription* stage_desc : _stages)
//        {
//            found_all &= stage_desc->is_complete(stages);
//        }
//        return found_all;
//    }
//
//    void IceGfxPass::add_stage(
//        ice::StringID_Arg name,
//        ice::Span<ice::StringID const> dependencies
//    ) noexcept
//    {
//        IceGfxStageDescription* target_stage = nullptr;
//
//        auto get_free_stage = [this]() noexcept
//        {
//            IceGfxStageDescription* free_batch = nullptr;
//            if (ice::pod::array::any(_free_stages))
//            {
//                free_batch = ice::pod::array::back(_free_stages);
//                free_batch->clear();
//                ice::pod::array::pop_back(_free_stages);
//            }
//            else
//            {
//                free_batch = _allocator.make<IceGfxStageDescription>(_allocator);
//            }
//            return free_batch;
//        };
//
//        {
//            auto candidate_batch = ice::pod::array::rbegin(_stages);
//            auto const end_batch = ice::pod::array::rend(_stages);
//
//            while (candidate_batch != end_batch)
//            {
//                if ((*candidate_batch)->contains_any(dependencies) == false)
//                {
//                    target_stage = *candidate_batch;
//                    candidate_batch += 1;
//                }
//                else
//                {
//                    candidate_batch = end_batch;
//                }
//            }
//        }
//
//        if (target_stage == nullptr)
//        {
//            target_stage = get_free_stage();
//            ice::pod::array::push_back(_stages, target_stage);
//        }
//
//        if (target_stage->has_dependency(name))
//        {
//            IceGfxStageDescription* new_target_stage = get_free_stage();
//
//            ice::u32 const batch_count = ice::size(_stages);
//            ice::u32 target_batch_idx = 0;
//            while(_stages[target_batch_idx] != target_stage)
//            {
//                ++target_batch_idx;
//            }
//
//            // Add an empty dummy entry
//            ice::pod::array::push_back(_stages, nullptr);
//
//            // Move everything down by one
//            for (ice::u32 idx = batch_count; idx > target_batch_idx; --idx)
//            {
//                _stages[idx] = _stages[idx - 1];
//            }
//
//            _stages[target_batch_idx] = new_target_stage;
//            target_stage = new_target_stage;
//        }
//
//        target_stage->add_stage(name, dependencies);
//    }
//
//    void IceGfxPass::clear() noexcept
//    {
//        // Copy all pointers so we dont need to re-allocate the array after a while
//        ice::pod::array::push_back(_free_stages, _stages);
//
//        // Clear the stage batches array so it's empty but the underlying memory is not freed
//        ice::pod::array::clear(_stages);
//    }
//
//    void IceGfxPass::execute_all(
//        ice::EngineFrame const& frame,
//        ice::render::CommandBuffer cmds,
//        ice::render::RenderCommands& api,
//        ice::pod::Hash<ice::gfx::IceGfxStage> const& stages
//    ) const noexcept
//    {
//        for (IceGfxStageDescription* stage_desc : _stages)
//        {
//            stage_desc->execute_all(frame, cmds, api, stages);
//        }
//    }
//
//} // namespace ice::gfx
