/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/result_codes.hxx>
#include <ice/mem_allocator.hxx>

namespace ice::platform
{

    //! \brief Flags used to select specific platform features.
    //!
    //! \note Depending on the specified flags some API's may be inaccessible, however specyfing always all features can also lead to unnecessary runtime costs.
    enum class FeatureFlags : ice::u32
    {
        None = 0x0,

        //! \brief Platform layer with the most basic features, allowing to query system and input events, query a render surface.
        Core = 0x01,

        //! \brief Represents the render surface layer. This allows to access the native drawable surface area.
        RenderSurface = 0x02,

        //! \brief Represents various values and states that can be accessed, including things like: battery, thermals, utilization.
        //!
        //! \note Not all properties are accessible no all platforms. More on this can be read in the '<todo>' section.
        //! \todo This flag is for now only used as example of a missing platform feature.
        Vitals = 0x04,

        All = Core,
    };

    //! \brief API type to platform FeatureFlags value mapping. Allows for easier API queries.
    template<typename FeatureType>
    static constexpr ice::platform::FeatureFlags Constant_FeatureFlags = FeatureFlags::None;


    //! \return All features available on the current platform and/or device.
    auto available_features() noexcept -> ice::platform::FeatureFlags;

    //! \brief Tries to initialize all selected feature API's.
    //!
    //! \param flags [in] Feature flags selected for initialization.
    auto initialize(
        ice::platform::FeatureFlags flags
    ) noexcept -> ice::Result;

    //! \brief Tries to initialize all selected feature API's.
    //!
    //! \param flags [in] Feature flags selected for initialization.
    //! \param alloc [in] The allocator to be used for all internal allocation requests.
    auto initialize_with_allocator(
        ice::platform::FeatureFlags flags,
        ice::Allocator& alloc
    ) noexcept -> ice::Result;

    //! \brief Queries the platform for the specific feature API pointer.
    //!
    //! \param flag [in] A single flag value representing the desired feature.
    //! \param out_api_ptr [in/out] A void pointer where the API address will be stored.
    //!
    //! \return 'Res::Success' if the platform was initialized and the API is available.
    auto query_api(ice::platform::FeatureFlags flag, void*& out_api_ptr) noexcept -> ice::Result;

    //! \brief Queries the platform for the specific feature API pointer.
    //! \note The feature flags will be fetched from the provided pointer API type.
    //!
    //! \param out_feature [in/out] A Feature type pointer where the API address will be stored.
    //!
    //! \return 'Res::Success' if the platform was initialized and the API is available.
    template<typename Feature> requires(Constant_FeatureFlags<Feature> != FeatureFlags::None)
    auto query_api(Feature*& out_feature) noexcept -> ice::Result
    {
        void* feature_ptr;
        ice::Result result = query_api(Constant_FeatureFlags<Feature>, feature_ptr);
        if (result == ice::Res::Success)
        {
            out_feature = reinterpret_cast<Feature*>(feature_ptr);
        }
        return result;
    }

    //! \brief Queries the platform for the specific feature API pointers.
    //!
    //! \param flag [in] A set of flags representing the desired features.
    //! \param out_api_ptrs [in/out] A void pointer array where the API addresses will be stored.
    //!
    //! \note This API might not be available later.
    //! \return 'Res::Success' if the platform was initialized and the requested API's are available.
    auto query_apis(ice::platform::FeatureFlags flags, void** out_api_ptrs) noexcept -> ice::Result;

    //! \brief Deletes all internal objects and invalidates all API pointers.
    //!
    //! \note After a call to this function using any other API other than \fn ice::platform::available_features or \fn ice::platform::initialize is undefined.
    void shutdown() noexcept;


    //! \brief Error returned when the platform API was already initialized.
    static constexpr ice::ResultCode E_PlatformAlreadyInitialized = ice::ResCode::create(ice::ResultSeverity::Error, "Platform Already Initialized");

    //! \brief Error returned when the platform API is not available on the current device and/or platform.
    static constexpr ice::ResultCode E_PlatformFeatureNotAvailable = ice::ResCode::create(ice::ResultSeverity::Error, "Platform Feature Not Available");

} // namespace ice::platform
