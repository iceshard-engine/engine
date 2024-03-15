/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_graph.hxx>
#include <ice/container/array.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/task.hxx>

namespace ice::gfx
{

    //! \brief A runtime object created from a graph.
    //! \details Graph runtimes handle all resource render API calls and lifetimes.
    //!   They will call `GfxStage::initialize` and `GfxStage::cleanup` if necessary and handle stages updates in order.
    struct GfxGraphRuntime
    {
        virtual ~GfxGraphRuntime() noexcept = default;

        //! \brief Prepares the runtime for the next execution phase.
        //! \details The runtime will check and prepare the graph for execution if all necessary stages are available and everything was initialized.
        //!   In the case where some stages are still not available, the execute will be skipped.
        //!
        //! \param[in] stages Access to graphics frame stages that can be awaited on tasks.
        //! \param[in] stage_registry Registry with all currently availale stages.
        //! \param[out] out_tasks Tasks container that should be executed to finish preparation. Tasks may execute on sub-threads.
        //! \return 'true' In preparation tasks where created, 'false' otherwise.
        virtual bool prepare(
            ice::gfx::GfxFrameStages& stages,
            ice::gfx::GfxStageRegistry const& stage_registry,
            ice::TaskContainer& out_tasks
        ) noexcept = 0;

        //! \brief Tries to execute the graph runtime for the given frame.
        //! \param[in] frame Fram object to be used for rendering.
        //! \param[in] fence Fence to be notified if GPU work was scheduled. If no work was pushed, the fence state is unchanged.
        //! \returns 'true' if GPU work was scheduled, 'false' otherwise.
        virtual bool execute(
            ice::EngineFrame const& frame,
            ice::render::RenderFence& fence
        ) noexcept = 0;
    };

    auto create_graph_runtime(
        ice::Allocator& alloc,
        ice::gfx::GfxContext& device,
        ice::gfx::GfxGraph const& base_definition,
        ice::gfx::GfxGraph const* dynamic_definition = nullptr
    ) noexcept -> ice::UniquePtr<GfxGraphRuntime>;

} // namespace ice::gfx
