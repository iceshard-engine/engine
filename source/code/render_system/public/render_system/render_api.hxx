#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace render::api
{

    namespace v1
    {

        struct iVec2
        {
            uint32_t x, y;
        };

        struct iRect
        {
            iVec2 offset;
            iVec2 size;
        };

        constexpr auto version_name = core::cexpr::stringid_cexpr("v1");

        enum class command_buffer_handle : uintptr_t;

        enum class render_pass_handle : uintptr_t;

        enum class frame_buffer_handle : uint32_t;

        struct api_interface
        {
            void (*cmd_begin_func)(command_buffer_handle, render_pass_handle, frame_buffer_handle, iRect);
            void (*cmd_end_func)(command_buffer_handle);

            void* reserved[32];
        };

    } // namespace v1

    using namespace render::api::v1;

} // namespace render::api
