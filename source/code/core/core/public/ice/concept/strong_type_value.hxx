/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/concept/strong_type_base.hxx>

namespace ice
{

    //! \brief Type tag to enable utility functions for strongly typed values.
    struct StrongValue { };

    //! \brief Concept used to determine if a struct is considerd a strong type wrapper.
    template<typename T>
    concept StrongValueType = std::is_pod_v<T>
        && ice::detail::HasAliasTypeTag<T, ice::StrongValue> // using TypeTag = ice::StrongValue;
        && ice::detail::OnlyMemberValue<T>;

    template<typename T> requires ice::StrongValueType<T>
    constexpr bool operator==(T left, T right) noexcept
    {
        return left.value == right.value;
    }


    //! \brief Type tag to enable utility functions for strongly typed values across multiple types.
    //!
    //! \note This tag should only be used for classe that may define an \b extended version for debug purposes.
    //! \warning This definition might be aliased to ice::StrongValue in ice::build::Configuration::Release builds.
    template<typename Tag>
    struct TaggedStrongValue { };

    //! \brief Concept used to determine if a struct is considerd a tagged strong type wrapper.
    //!
    //! \note Compared to ice::StrongValueType this concept drops the single member requirement.
    template<typename T>
    concept TaggedStrongValueType = std::is_pod_v<T>
        && ice::detail::HasAliasTypeTag<T, ice::TaggedStrongValue<typename ice::detail::ExtractTemplateType<typename T::TypeTag>::Type>>;

    //! \brief Concept used to ensure two strong types have the same tag type.
    template<typename T, typename U>
    concept SameTaggedTypes = std::is_same_v<typename T::TypeTag, typename U::TypeTag>
        && TaggedStrongValueType<T>
        && TaggedStrongValueType<U>;

    template<typename T, typename U> requires ice::SameTaggedTypes<T, U>
    constexpr bool operator==(T left, U right) noexcept
    {
        return left.value == right.value;
    }

} // namespace ice
