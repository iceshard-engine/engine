#pragma once
#include <core/allocator.hxx>

namespace iceshard
{


    //! \brief The main class from the engine library.
    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        //! \brief Returns the engine revision.
        virtual auto revision() const noexcept -> uint32_t = 0;
    };


} // namespace iceshard
