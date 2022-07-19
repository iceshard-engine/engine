#pragma once
#include <catch2/catch.hpp>
#include <ice/mem_types.hxx>

namespace Catch
{
    template<>
    struct StringMaker<ice::usize>
    {
        static std::string convert(ice::usize const& value)
        {
            return StringMaker<ice::usize::base_type>::convert(value.value);
        }
    };
}
