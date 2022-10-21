#pragma once
#include <ice/base.hxx>

namespace ice
{

    //! \brief The logic implemented by a collectiont type when working with data. (Copying, Moving, Removing, etc.)
    //! \note The picked logic will affect performance, but may also impose restrictions.
    enum class ContainerLogic : ice::u8
    {
        //! \brief The collection only handles plain old data and is allowed to memcopy values.
        //! \pre The type stored in the collection satisfies `std::is_pod`
        Trivial,

        //! \brief The collection handles complex data types and properly implements copy and move semantics.
        Complex,
    };


    //! \brief A concept that ensures only types that can be trivially copyable can be 'forced' to use trifial Logic.
    template<typename Type>
    concept TrivialContainerLogicAllowed = std::is_trivially_copyable_v<Type> && std::is_trivially_destructible_v<Type> && std::is_default_constructible_v<Type>;

    //! \brief A helper used to pick the desired logic when it comes to handling data in collections.
    //!
    //! \note Only plain-old-data types use trivial collection logic by default.
    template<typename Type>
    static constexpr ice::ContainerLogic Constant_DefaultContainerLogic = (std::is_pod_v<Type> || std::is_enum_v<Type>)
        ? ContainerLogic::Trivial
        : ContainerLogic::Complex;


} // namespace ice
