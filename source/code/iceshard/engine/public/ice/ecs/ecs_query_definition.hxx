#pragma once
#include <ice/ecs/ecs_query_type.hxx>
#include <ice/ecs/ecs_query_details.hxx>

namespace ice::ecs
{

    //! \brief A static compile-time definition of a Query that can be executed during frame.
    //! \tparam ...QueryComponents Component types, with decorators, we want to access in the qyery.
    //!
    //! \example QueryDefinition<ComponentB&, const ComponentA*>
    template<ice::ecs::QueryType... QueryComponents>
    struct QueryDefinition
    {
        using Query = ice::ecs::Query<QueryComponents...>;

        static constexpr ice::ucount Constant_ComponentCount = sizeof...(QueryComponents);
        static constexpr ice::StaticArray<ice::ecs::detail::QueryTypeInfo, Constant_ComponentCount> Constant_Requirements =
            ice::ecs::detail::QueryRequirements<QueryComponents...>::Constant_Requirements;

        template<typename Fn>
        static void invoke_for_each_block(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
        {
            return ice::ecs::detail::invoke_for_each_block<Fn, QueryComponents...>(
                ice::forward<Fn>(fn),
                count,
                component_pointer_array
            );
        }

        template<typename Fn>
        static void invoke_for_each_entity(Fn&& fn, ice::u32 count, void** component_pointer_array) noexcept
        {
            return ice::ecs::detail::invoke_for_each_entity<Fn, QueryComponents...>(
                ice::forward<Fn>(fn),
                count,
                component_pointer_array
            );
        }

        // template<typename Fn>
        // static void invoke_for_each_entity_fn(void* fnp, ice::u32 count, void** component_pointer_array) noexcept
        // {
        //     return invoke_for_each_entity((Fn)fnp, count, component_pointer_array);
        // }

        // template<typename Fn>
        // static void invoke_for_each_entity_op(void* obj, ice::u32 count, void** component_pointer_array) noexcept
        // {
        //     return invoke_for_each_entity(*(Fn*)obj, count, component_pointer_array);
        // }

        ice::u32 const component_count = Constant_ComponentCount;

        ice::StaticArray<ice::ecs::detail::QueryTypeInfo, Constant_ComponentCount> const requirements = ice::ecs::detail::QueryRequirements<QueryComponents...>::Constant_Requirements;
    };

} // namespace ice::ecs
