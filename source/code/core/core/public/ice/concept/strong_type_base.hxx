#pragma once
#include <type_traits>

namespace ice
{

    namespace detail
    {

        template<typename T>
        concept HasMemberValue = requires(T) {
            { T::value } -> std::convertible_to<decltype(T::value)>;
        };

        template<typename T>
        concept OnlyMemberValue = HasMemberValue<T> && sizeof(T) == sizeof(T::value);

        template<typename T, typename ExpectedTag>
        concept HasAliasTypeTag = std::is_same_v<typename T::TypeTag, ExpectedTag>;

        template<typename Type>
        struct ExtractTemplateType { };

        template<template <typename> typename TType, typename ExtractedType>
        struct ExtractTemplateType<TType<ExtractedType>>
        {
            using Type = ExtractedType;
        };

    } // namespace detail

} // namespace ice
