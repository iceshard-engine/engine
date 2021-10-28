#pragma once

namespace ice::ecs
{

    //! \brief A static compile-time definition of a Query that can be executed during frame.
    //! \tparam ...QueryComponents Component types, with decorators, we want to access in the qyery.
    //!
    //! \example QueryDefinition<ComponentB&, const ComponentA*>
    template<typename... QueryComponents>
    struct QueryDefinition
    {

    };

    //! \brief A query holds information about all archetypes that should be iterated but it's not yet executed.
    template<typename... QueryComponents>
    struct Query
    {

    };

    //! \brief A single state after a successfull execution. This state can be iterated over to access all associated entities.
    //!
    //! \detail This state could be cached accross frames, however currently this is not the case.
    //! \detail To iterate over a query state you need to do it via the query you used to create the state.
    struct QueryState
    {

    };

} // namespace ice::ecs
