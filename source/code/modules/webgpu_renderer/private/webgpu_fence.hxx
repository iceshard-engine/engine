/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_fence.hxx>
#include <ice/sync_manual_events.hxx>

#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    // Fences are more of a mock in this API, they aren't really needed it seems.
    class WebGPUCallbackFence : public ice::render::RenderFence
    {
    public:
        WebGPUCallbackFence() noexcept
            : _barrier{ }
        {
        }

        void set() noexcept
        {
            _barrier.set();
        }

        bool wait(ice::u64 timeout_ns) noexcept override
        {
            if (_barrier.is_set() == false)
            {
                _barrier.wait();
            }
            return true;
        }

        void reset() noexcept override
        {
            _barrier.reset(1u);
        }

    public:
        ice::ManualResetBarrier _barrier;
    };

} // namespace ice::render::webgpu
