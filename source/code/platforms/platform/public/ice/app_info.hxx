#pragma once
#include <ice/string_types.hxx>

namespace ice
{

    void app_location(ice::HeapString<char8_t>& out) noexcept;

    void working_directory(ice::HeapString<char8_t>& out) noexcept;

} // namespace ice
