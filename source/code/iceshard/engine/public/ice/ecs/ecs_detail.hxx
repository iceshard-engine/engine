#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_entity.hxx>

namespace ice::ecs
{

    namespace detail
    {

        // #todo move to a different file with implementation details / utility.
        template<typename T>
        concept HasIdentifierMember = requires(T x) {
            { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
        };

    } // namespace detail

} // namespace ice::ecs
