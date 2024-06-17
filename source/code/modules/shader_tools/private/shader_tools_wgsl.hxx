#pragma once
#include <ice/string/heap_string.hxx>
#include <arctic/arctic_syntax_node.hxx>

namespace ice
{

    auto transpile_shader_asl_to_wgsl(
        ice::Allocator& allocator,
        ice::Data asl_source
    ) noexcept -> ice::HeapString<>;

} // namespace ice
