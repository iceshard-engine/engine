/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <concepts>

namespace ice::os
{

    // TODO: Make it into a StringID, because it can be arbitrary...
    enum class HandleType : ice::u8
    {
        File,
        DynLib,

        // TODO: Remove
#if ISP_ANDROID
        JClass,
        JObject,
        JString,
#endif
    };

    template<HandleType HType>
    struct HandleDescriptor;

    template<HandleType HType>
    concept ValidHandleDescriptor = requires {
        std::is_same_v<typename HandleDescriptor<HType>::PlatformHandleType, void> == false;
        { HandleDescriptor<HType>::InvalidHandle } -> std::convertible_to<typename HandleDescriptor<HType>::PlatformHandleType>;
        { HandleDescriptor<HType>::is_valid(HandleDescriptor<HType>::InvalidHandle, nullptr) } -> std::convertible_to<bool>;
        { HandleDescriptor<HType>::close(HandleDescriptor<HType>::InvalidHandle, nullptr) } -> std::convertible_to<bool>;
    };

    template<HandleType HType>
    concept ManagedHandleDescriptor = ValidHandleDescriptor<HType> && requires {
        std::is_same_v<typename HandleDescriptor<HType>::HandleManagerType, void> == false;
    };

    template<HandleType HType, bool = false> requires (ValidHandleDescriptor<HType>)
    struct HandleInternal
    {
        using NativeType = typename HandleDescriptor<HType>::PlatformHandleType;
        using ManagerType = struct {};

        NativeType _handle;
    };

    template<HandleType HType> requires (ManagedHandleDescriptor<HType>)
    struct HandleInternal<HType, true>
    {
        using NativeType = typename HandleDescriptor<HType>::PlatformHandleType;
        using ManagerType = typename HandleDescriptor<HType>::HandleManagerType;

        NativeType _handle;
        ManagerType _manager;
    };

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    struct Handle;

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    struct Handle final
    {
        using NativeDescriptor = HandleDescriptor<HType>;
        using NativeInternal = HandleInternal<HType, ManagedHandleDescriptor<HType>>;
        using NativeType = typename NativeInternal::NativeType;

        Handle() noexcept;
        explicit Handle(NativeType value) noexcept requires(ManagedHandleDescriptor<HType> == false);
        explicit Handle(typename NativeInternal::ManagerType manager, NativeType value) noexcept requires(ManagedHandleDescriptor<HType> == true);
        Handle(Handle&& other) noexcept;
        Handle(Handle const& other) noexcept = delete;
        ~Handle() noexcept;

        auto operator=(Handle&& other) noexcept -> Handle&;
        auto operator=(Handle const& other) noexcept -> Handle& = delete;

        operator bool() const noexcept requires(ManagedHandleDescriptor<HType> == false)
        {
            return NativeDescriptor::is_valid(_internal._handle, nullptr);
        }

        operator bool() const noexcept requires(ManagedHandleDescriptor<HType> == true)
        {
            return NativeDescriptor::is_valid(_internal._handle, _internal._manager);
        }

        auto native() const noexcept -> NativeType { return _internal._handle; }
        bool close() noexcept;

    private:
        NativeInternal _internal;
    };

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle() noexcept
        : _internal{ NativeDescriptor::InvalidHandle }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle(NativeType value) noexcept requires(ManagedHandleDescriptor<HType> == false)
        : _internal{ value }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle(
        typename NativeInternal::ManagerType manager,
        NativeType value
    ) noexcept requires(ManagedHandleDescriptor<HType> == true)
        : _internal{ value, manager }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle(Handle&& other) noexcept
        : _internal{ ice::exchange(other._internal, { NativeDescriptor::InvalidHandle }) }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::~Handle() noexcept
    {
        close();
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    auto Handle<HType>::operator=(Handle&& other) noexcept -> Handle&
    {
        if (this != &other)
        {
            close();
            _internal = ice::exchange(other._internal, { NativeDescriptor::InvalidHandle });
        }
        return *this;
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    bool Handle<HType>::close() noexcept
    {
        if (*this)
        {
            if constexpr(ManagedHandleDescriptor<HType>)
            {
                auto const temp = ice::exchange(_internal, { NativeDescriptor::InvalidHandle });
                return NativeDescriptor::close(temp._handle, temp._manager);
            }
            else
            {
                return NativeDescriptor::close(ice::exchange(_internal._handle, NativeDescriptor::InvalidHandle), nullptr);
            }
        }
        return false;
    }

} // namespace ice::os
