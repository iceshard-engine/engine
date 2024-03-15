/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
        return ice::EngineStateGraph{ ice::stringid(str, len) };
    }

    template<size_t Size>
    inline constexpr auto operator|(ice::EngineStateGraph graph, char const (&str)[Size]) noexcept
    {
        return ice::EngineState{ graph, ice::stringid(str, Size) };
    }

    inline constexpr bool operator==(ice::EngineState lhs, ice::EngineState rhs) noexcept
    {
        return lhs.graph == rhs.graph && lhs.value == rhs.value;
    }

} // namespace ice
