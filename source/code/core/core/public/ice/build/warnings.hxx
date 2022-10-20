#pragma once
#include <ice/build/platform.hxx>

#if ISP_COMPILER_MSVC

#   define ISC_WARNING_PUSH __pragma(warning(push))
#   define ISC_WARNING_SET(op, warn_value) __pragma(warning(op : warn_value))
#   define ISC_WARNING_POP __pragma(warning(pop))

#   define ISCW_OP_DISABLE disable

// declaration of '<...>' hides class member
#   define ISCW_DECLARATION_HIDES_CLASS_MEMBER(op) ISC_WARNING_SET(op, 4458)
// '<...>': unreferenced function with internal linkage has been removed
#   define ISCW_UNREFERENCED_INTERNAL_FUNCTION_REMOVED(op) ISC_WARNING_SET(op, 4505)
// '<...>': check operator precedence for possible error; use parentheses to clarify precedence
#   define ISCW_CHECK_OPERATOR_PRECEDENCE(op) ISC_WARNING_SET(op, 4554)
// operator '<...>': deprecated between enumerations of different types
#   define ISCW_OPERATOR_DEPRECATED_BETWEEN_UNRELATED_ENUMERATIONS(op) ISC_WARNING_SET(op, 5054)

#elif ISP_COMPILER_CLANG | ISP_COMPILER_GCC

#   define ISC_WARNING_PUSH __pragma(warning(push))
#   define ISC_WARNING_SET(op, warn_value) __pragma(warning(op : warn_value))
#   define ISC_WARNING_POP __pragma(warning(pop))

#   define ISCW_OP_DISABLE disable

#   define ISCW_DECLARATION_HIDES_CLASS_MEMBER(op)
#   define ISCW_UNREFERENCED_INTERNAL_FUNCTION_REMOVED(op)
#   define ISCW_CHECK_OPERATOR_PRECEDENCE(op)
#   define ISCW_OPERATOR_DEPRECATED_BETWEEN_UNRELATED_ENUMERATIONS(op)

#endif
