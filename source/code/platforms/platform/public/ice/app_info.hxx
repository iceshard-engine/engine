#pragma once
#include <ice/string_types.hxx>

namespace ice
{

    void app_location(ice::HeapString<>& out) noexcept;
    void working_directory(ice::HeapString<>& out) noexcept;

} // namespace ice
