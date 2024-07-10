#pragma once
#include <ice/shard.hxx>
#include <ice/expected.hxx>
#include <ice/engine_types.hxx>

namespace ice
{

    enum class TraitTaskType : ice::u8
    {
        Frame,
        Runner,
    };

} // namespace ice

namespace ice::detail
{

    struct TraitContextImpl;

} // namespace ice::detail
