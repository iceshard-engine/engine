#include "gfx_dynamic_pass.hxx"
#include <ice/assert.hxx>

namespace ice::gfx
{

    GfxDynamicPassStageGroup::GfxDynamicPassStageGroup(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _entries{ _allocator }
        , _dependencies{ _allocator }
    {
        ice::pod::array::reserve(_entries, 10);
        ice::pod::array::reserve(_dependencies, 25);
    }

    bool GfxDynamicPassStageGroup::has_work() const noexcept
    {
        return ice::pod::array::empty(_entries) == false;
    }

    bool GfxDynamicPassStageGroup::contains_any(
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

    bool GfxDynamicPassStageGroup::has_dependency(
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

    void GfxDynamicPassStageGroup::add_stage(
        ice::StringID_Arg name,
        ice::Span<ice::StringID const> dependencies
    ) noexcept
    {
        ice::pod::array::push_back(_entries,
            Entry{
                .name = name,
                .dependency_offset = ice::size(_dependencies),
                .dependency_count = ice::size(dependencies)
            }
        );

        ice::pod::array::push_back(_dependencies, dependencies);
    }

    void GfxDynamicPassStageGroup::clear() noexcept
    {
        ice::pod::array::clear(_entries);
        ice::pod::array::clear(_dependencies);
    }

    void GfxDynamicPassStageGroup::query_stage_order(
        ice::pod::Array<ice::StringID_Hash>& stage_order_out
    ) const noexcept
    {
        for (Entry const& entry : _entries)
        {
            ice::pod::array::push_back(stage_order_out, ice::stringid_hash(entry.name));
        }
    }

    IceGfxDynamicPass::IceGfxDynamicPass(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _special_stages{ { .name = ice::stringid_invalid }, { .name = ice::stringid_invalid } }
        , _stages{ _allocator }
        , _free_stages{ _allocator }
    {
        ice::pod::array::reserve(_stages, 5);
        ice::pod::array::reserve(_free_stages, 5);
        ice::pod::array::push_back(
            _stages,
            _allocator.make<GfxDynamicPassStageGroup>(_allocator)
        );
    }

    IceGfxDynamicPass::~IceGfxDynamicPass() noexcept
    {
        for (GfxDynamicPassStageGroup* stage_desc : _stages)
        {
            _allocator.destroy(stage_desc);
        }
        for (GfxDynamicPassStageGroup* stage_desc : _free_stages)
        {
            _allocator.destroy(stage_desc);
        }
    }

    bool IceGfxDynamicPass::has_work() const noexcept
    {
        bool has_work = false;
        for (GfxDynamicPassStageGroup* stage_desc : _stages)
        {
            has_work |= stage_desc->has_work();
        }
        return has_work;
    }

    void IceGfxDynamicPass::add_stages(
        ice::Span<ice::gfx::GfxStageInfo const> stage_infos
    ) noexcept
    {
        for (ice::gfx::GfxStageInfo const& stage_info : stage_infos)
        {
            add_stage(stage_info);
        }
    }

    void IceGfxDynamicPass::add_stage(
        ice::gfx::GfxStageInfo const& stage_info
    ) noexcept
    {
        switch (stage_info.type)
        {
        case GfxStageType::InitialStage:
            _special_stages[0] = stage_info;
            return;
        case GfxStageType::FinalStage:
            _special_stages[1] = stage_info;
            return;
        default:
            break;
        }


        GfxDynamicPassStageGroup* target_stage = nullptr;

        auto get_free_stage = [this]() noexcept
        {
            GfxDynamicPassStageGroup* free_batch = nullptr;
            if (ice::pod::array::any(_free_stages))
            {
                free_batch = ice::pod::array::back(_free_stages);
                free_batch->clear();
                ice::pod::array::pop_back(_free_stages);
            }
            else
            {
                free_batch = _allocator.make<GfxDynamicPassStageGroup>(_allocator);
            }
            return free_batch;
        };

        {
            auto candidate_batch = ice::pod::array::rbegin(_stages);
            auto const end_batch = ice::pod::array::rend(_stages);

            while (candidate_batch != end_batch)
            {
                if ((*candidate_batch)->contains_any(stage_info.dependencies) == false)
                {
                    target_stage = *candidate_batch;
                    candidate_batch += 1;
                }
                else
                {
                    candidate_batch = end_batch;
                }
            }
        }

        if (target_stage == nullptr)
        {
            target_stage = get_free_stage();
            ice::pod::array::push_back(_stages, target_stage);
        }

        if (target_stage->has_dependency(stage_info.name))
        {
            GfxDynamicPassStageGroup* new_target_stage = get_free_stage();

            ice::u32 const batch_count = ice::size(_stages);
            ice::u32 target_batch_idx = 0;
            while (_stages[target_batch_idx] != target_stage)
            {
                ++target_batch_idx;
            }

            // Add an empty dummy entry
            ice::pod::array::push_back(_stages, nullptr);

            // Move everything down by one
            for (ice::u32 idx = batch_count; idx > target_batch_idx; --idx)
            {
                _stages[idx] = _stages[idx - 1];
            }

            _stages[target_batch_idx] = new_target_stage;
            target_stage = new_target_stage;
        }

        target_stage->add_stage(stage_info.name, stage_info.dependencies);
    }

    void IceGfxDynamicPass::clear() noexcept
    {
        // Copy all pointers so we dont need to re-allocate the array after a while
        ice::pod::array::push_back(_free_stages, _stages);

        // Clear the stage batches array so it's empty but the underlying memory is not freed
        ice::pod::array::clear(_stages);
    }

    void IceGfxDynamicPass::query_stage_order(
        ice::pod::Array<ice::StringID_Hash>& stage_order_out
    ) const noexcept
    {
        bool valid_pass = ice::pod::array::any(_stages);
        valid_pass &= _special_stages[0].name != ice::stringid_invalid;
        valid_pass &= _special_stages[1].name != ice::stringid_invalid;

        if (valid_pass == false)
        {
            return;
        }

        ice::pod::array::push_back(stage_order_out, ice::stringid_hash(_special_stages[0].name));
        for (auto const* stage_group : _stages)
        {
            stage_group->query_stage_order(stage_order_out);
        }
        ice::pod::array::push_back(stage_order_out, ice::stringid_hash(_special_stages[1].name));
    }

} // namespace ice::gfx
