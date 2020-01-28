#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard::renderer::api
{

    namespace v1_1
    {

        constexpr auto version_name = "v1.1"_sid;

        enum class RenderPass : uintptr_t { Invalid = 0x0 };

        enum class CommandBuffer : uintptr_t { Invalid = 0x0 };

        enum class ResourceSet : uintptr_t { Invalid = 0x0 };

        enum class Texture : uintptr_t { Invalid = 0x0 };

        enum class Sampler : uint32_t
        {
            Default,
            Invalid = std::numeric_limits<std::underlying_type_t<Sampler>>::max()
        };

    } // namespace v1

    using namespace v1_1;

} // namespace iceshard::renderer::api::v1
