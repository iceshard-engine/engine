/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>

namespace ice::gfx
{

    namespace detail
    {

        struct GfxTaskOperationData
        {
            std::coroutine_handle<> coroutine = nullptr;
            ice::gfx::detail::GfxTaskOperationData* next = nullptr;
        };

        template<typename Target, typename Data = ice::gfx::detail::GfxTaskOperationData>
        struct GfxTaskOperation
        {
            inline GfxTaskOperation(Target& target) noexcept;
            inline GfxTaskOperation(Target& target, Data data) noexcept;

            inline bool await_ready() const noexcept { return false; }
            inline void await_suspend(std::coroutine_handle<> coro) noexcept;
            inline void await_resume() const noexcept { }

        protected:
            Target& _target;
            Data _data;
        };

        template<typename Target, typename Data>
        inline GfxTaskOperation<Target, Data>::GfxTaskOperation(Target& target) noexcept
            : _target{ target }
            , _data{ }
        { }

        template<typename Target, typename Data>
        inline GfxTaskOperation<Target, Data>::GfxTaskOperation(Target& target, Data data) noexcept
            : _target{ target }
            , _data{ data }
        { }

        template<typename Target, typename Data>
        inline void GfxTaskOperation<Target, Data>::await_suspend(std::coroutine_handle<> coro) noexcept
        {
            _data.coroutine = coro;
            _target.schedule_internal(_data);
        }

    } // namespace detail

    class GfxFrame;
    class GfxFrameStage;

    struct GfxAwaitExecuteStageData
    {
        ice::gfx::GfxFrameStage const* stage;
        std::coroutine_handle<> coroutine = nullptr;
        ice::gfx::GfxAwaitExecuteStageData* next = nullptr;
    };

    struct GfxAwaitBeginFrameData : ice::gfx::detail::GfxTaskOperationData { };
    struct GfxAwaitEndFrameData : ice::gfx::detail::GfxTaskOperationData { };

    using GfxAwaitBeginFrame = ice::gfx::detail::GfxTaskOperation<ice::gfx::GfxFrame, ice::gfx::GfxAwaitBeginFrameData>;
    using GfxAwaitExecuteStage = ice::gfx::detail::GfxTaskOperation<ice::gfx::GfxFrame, ice::gfx::GfxAwaitExecuteStageData>;
    using GfxAwaitEndFrame = ice::gfx::detail::GfxTaskOperation<ice::gfx::GfxFrame, ice::gfx::GfxAwaitEndFrameData>;

} // namespace ice::gfx
