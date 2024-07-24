#pragma once
#include <ice/string/string.hxx>
#include <ice/assert_core.hxx>

#ifdef IM_ASSERT
#undef IM_ASSERT
#endif

#define IM_ASSERT(cond) ICE_ASSERT_CORE(cond)
#include <imgui/imgui.h>
#include <fmt/core.h>
#undef assert


namespace ImGui
{

    namespace Detail
    {

        auto TempBuffer() noexcept -> char*;
        auto TempBufferSize() noexcept -> size_t;
        void TextEx(char const* begin, char const* end) noexcept;

    } // namespace Detail

    template<typename... Args>
    void TextT(fmt::format_string<Args...> format, Args&&... args) noexcept
    {
        fmt::format_to_n_result<char*> result = fmt::format_to_n(
            Detail::TempBuffer(),
            Detail::TempBufferSize(),
            format,
            ice::forward<Args>(args)...
        );
        Detail::TextEx(Detail::TempBuffer(), result.out);
    }

} // namespace ImGui
