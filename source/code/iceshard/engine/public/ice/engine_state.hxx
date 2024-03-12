#pragma once
#include <ice/engine_types.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    struct EngineStateGraph
    {
        using TypeTag = ice::StrongValue;

        ice::StringID value;
    };

    struct EngineState
    {
        ice::EngineStateGraph graph;
        ice::StringID value;
    };


    inline constexpr auto operator""_state_graph(char const* str, size_t len) noexcept
    {
        //auto const result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(std::string_view{ str, len }, 0x51A1'E069);
        //return ice::EngineStateGraph{ result.h[0] };
        return ice::EngineStateGraph{ ice::stringid(str, len) };
    }

    template<size_t Size>
    inline constexpr auto operator|(ice::EngineStateGraph graph, char const (&str)[Size]) noexcept
    {
        //auto const result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(std::string_view{ str }, 0x51A1'E420);
        //return ice::EngineState{ graph, result.h[0] };
        return ice::EngineState{ graph, ice::stringid(str, Size) };
    }

    inline constexpr bool operator==(ice::EngineState lhs, ice::EngineState rhs) noexcept
    {
        return lhs.graph == rhs.graph && lhs.value == rhs.value;
    }

} // namespace ice
