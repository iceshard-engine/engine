#pragma once
#include <ice/base.hxx>
#include <array>

namespace ice
{

    template<typename T, ice::u32 Size>
    using StaticArray = std::array<T, Size>;

}; // namespace ice
