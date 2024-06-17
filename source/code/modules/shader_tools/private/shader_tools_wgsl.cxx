#include "shader_tools_wgsl.hxx"

namespace ice
{

    auto transpile_shader_asl_to_wgsl(
        ice::Allocator& allocator,
        ice::Data asl_source
    ) noexcept -> ice::HeapString<>
    {
        ice::HeapString<> result{ allocator };
        return result;
    }

} // namespace ice
