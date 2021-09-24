#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_entity.hxx>

namespace ice::ecs
{

    namespace detail
    {

        template<typename T>
        concept HasIdentifierMember = requires(T x) {
            { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
        };

        template<typename T>
        constexpr auto constexpr_sort_array(T const& arr) noexcept
        {
            auto result = arr;
            std::sort(std::begin(result), std::end(result));
            return result;
        }

    } // namespace detail

} // namespace ice::ecs
