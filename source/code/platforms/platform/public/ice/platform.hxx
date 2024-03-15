/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/expected.hxx>
#include <ice/shard.hxx>
#include <ice/span.hxx>

namespace ice::platform
{

    //! \brief Flags used to select specific platform features.
    //!
    //! \note Some API's may be restricted to specific platforms.
    //! \note Some features might result in small performance costs, depending on the API implementation.
    enum class FeatureFlags : ice::u32
    {
        None = 0x0,

        //! \brief Platform layer with the most basic features, allowing to query system and input events.
        Core = 0x0001,

        //! \brief Represents the render surface layer. This allows to access the native drawable surface area.
        RenderSurface = 0x0002,

        //! \brief Represents various device reported values and states. For example.: battery, temperature.
        //! \note Some properties maybe only be available on specific platforms.
        Vitals = 0x0004,

        //! \brief Represents various storage locations provided by the OS.
        //!   These paths should be used to initialize resources providers or write data to.
        //! \note For portability reasons it's suggested to always use paths availabe from this OS module.
        StoragePaths = 0x0008,

        //! \brief Access to command line arguments / execution parameters passed to the app.
        ExecutionParams = 0x8000,

        //! \note Used to properly handle the binary 'not (~)' operation.
        //! \see ice::FlagType and ice::FlagTypeAll concepts for details.
        All = Core | RenderSurface | Vitals | StoragePaths | ExecutionParams,
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
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params = {}
    ) noexcept -> ice::Result;

    //! \brief Tries to initialize all selected feature API's.
    //!
    //! \param flags [in] Feature flags selected for initialization.
    //! \param alloc [in] The allocator to be used for all internal allocation requests.
    auto initialize_with_allocator(
        ice::Allocator& alloc,
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params = {}
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
        if (result == ice::S_Success)
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
    //! \note After a call to this function using any other API other than
    //!   \fn ice::platform::available_features or \fn ice::platform::initialize is not allowed.
    void shutdown() noexcept;


    //! \brief Error returned when the platform API was already initialized.
    static constexpr ice::ErrorCode E_PlatformAlreadyInitialized{ "E.0200:App:Platform Already Initialized" };

    //! \brief Error returned when the platform API is not available on the current device and/or platform.
    static constexpr ice::ErrorCode E_PlatformFeatureNotAvailable{ "E.0201:App:Platform Feature Not Available" };

    //! \brief Error returned when accessing Android platform API's from an JNI library
    //!   that is not a NativeActivity Application.
    //! \note This error is only returned on Android systems.s
    static constexpr ice::ErrorCode E_PlatformAndroidNotANativeActivity{ "E.0202:App:The current Android JNI instsance is not a NativeActivity!" };

} // namespace ice::platform
