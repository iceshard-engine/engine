/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    struct URI;
    struct Metadata;
    struct ResourceHandle;
    struct ResourceTrackerCreateInfo;

    class Resource;
    class ResourceProvider;
    class ResourceTracker;
    class LooseResource;

    enum class ResourceFlags : ice::u32;
    enum class ResourceStatus : ice::u32;

    //! \brief Callback function to be used when evaluating sub-resource selection depending on flags.
    //! \note This function might not be called if expected and current flags are equal.
    //!
    //! \param[in] expected - Flags provided by the user that should be matched.
    //! \param[in] current - Flags currently evaluated for a given resource object.
    //! \param[in] selected - Flags that where so far the best matching if they where not equal to the expected ones.
    using ResourceFlagsCompareFn = auto(
        ice::ResourceFlags expected,
        ice::ResourceFlags current,
        ice::ResourceFlags selected
    ) noexcept -> ice::u32;

} // namespace ice
