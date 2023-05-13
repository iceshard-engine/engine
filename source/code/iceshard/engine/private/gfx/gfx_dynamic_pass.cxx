/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "gfx_dynamic_pass.hxx"
#include <ice/assert.hxx>

namespace ice::gfx
{

    GfxDynamicPassStageGroup::GfxDynamicPassStageGroup(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _entries{ _allocator }
        , _dependencies{ _allocator }
    {
        ice::array::reserve(_entries, 10);
        ice::array::reserve(_dependencies, 25);
    }

    auto GfxDynamicPassStageGroup::stage_count() const noexcept -> ice::u32
    {
        return ice::array::count(_entries);
    }

    bool GfxDynamicPassStageGroup::has_work() const noexcept
    {
        return ice::array::empty(_entries) == false;
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
            ice::Span<StringID const> entry_deps{ ice::array::begin(_dependencies) + entry.dependency_offset, entry.dependency_count };

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
        ice::array::push_back(
            _entries,
            Entry{
                .name = name,
                .dependency_offset = ice::count(_dependencies),
                .dependency_count = ice::count(dependencies)
            }
        );

        ice::array::push_back(_dependencies, dependencies);
    }

    void GfxDynamicPassStageGroup::clear() noexcept
    {
        ice::array::clear(_entries);
        ice::array::clear(_dependencies);
    }

    void GfxDynamicPassStageGroup::query_stage_order(
        ice::Array<ice::StringID_Hash>& stage_order_out
    ) const noexcept
    {
        for (Entry const& entry : _entries)
        {
            ice::array::push_back(stage_order_out, ice::stringid_hash(entry.name));
        }
    }

    IceGfxDynamicPass::IceGfxDynamicPass(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _stages{ _allocator }
        , _free_stages{ _allocator }
    {
        ice::array::reserve(_stages, 5);
        ice::array::reserve(_free_stages, 5);
        ice::array::push_back(
            _stages,
            _allocator.create<GfxDynamicPassStageGroup>(_allocator)
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

    auto IceGfxDynamicPass::stage_count() const noexcept -> ice::u32
    {
        ice::u32 result = 0;
        for (GfxDynamicPassStageGroup* stage_desc : _stages)
        {
            result += stage_desc->stage_count();
        }
        return result;
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

    void IceGfxDynamicPass::add_stage(
        ice::StringID_Arg stage_name,
        ice::Span<ice::StringID const> dependencies
    ) noexcept
    {
        GfxDynamicPassStageGroup* target_stage = nullptr;

        auto get_free_stage = [this]() noexcept
        {
            GfxDynamicPassStageGroup* free_batch = nullptr;
            if (ice::array::any(_free_stages))
            {
                free_batch = ice::array::back(_free_stages);
                free_batch->clear();
                ice::array::pop_back(_free_stages);
            }
            else
            {
                free_batch = _allocator.create<GfxDynamicPassStageGroup>(_allocator);
            }
            return free_batch;
        };

        {
            auto candidate_batch = ice::array::rbegin(_stages);
            auto const end_batch = ice::array::rend(_stages);

            while (candidate_batch != end_batch)
            {
                if ((*candidate_batch)->contains_any(dependencies) == false)
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
            ice::array::push_back(_stages, target_stage);
        }

        if (target_stage->has_dependency(stage_name))
        {
            GfxDynamicPassStageGroup* new_target_stage = get_free_stage();

            ice::u32 const batch_count = ice::count(_stages);
            ice::u32 target_batch_idx = 0;
            while (_stages[target_batch_idx] != target_stage)
            {
                ++target_batch_idx;
            }

            // Add an empty dummy entry
            ice::array::push_back(_stages, nullptr);

            // Move everything down by one
            for (ice::u32 idx = batch_count; idx > target_batch_idx; --idx)
            {
                _stages[idx] = _stages[idx - 1];
            }

            _stages[target_batch_idx] = new_target_stage;
            target_stage = new_target_stage;
        }

        target_stage->add_stage(stage_name, dependencies);
    }

    void IceGfxDynamicPass::clear() noexcept
    {
        // Copy all pointers so we dont need to re-allocate the array after a while
        ice::array::push_back(_free_stages, _stages);

        // Clear the stage batches array so it's empty but the underlying memory is not freed
        ice::array::clear(_stages);
    }

    void IceGfxDynamicPass::query_stage_order(
        ice::Array<ice::StringID_Hash>& stage_order_out
    ) const noexcept
    {
        bool valid_pass = ice::array::any(_stages);
        if (valid_pass == false)
        {
            return;
        }

        for (auto const* stage_group : _stages)
        {
            stage_group->query_stage_order(stage_order_out);
        }
    }

} // namespace ice::gfx
