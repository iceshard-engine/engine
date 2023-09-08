/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <concepts>

namespace ice::os
{

    enum class HandleType : ice::u8
    {
        File,
        DynLib,
    };

    template<HandleType HType>
    struct HandleDescriptor;

    template<HandleType HType>
    concept ValidHandleDescriptor = requires {
        std::is_same_v<typename HandleDescriptor<HType>::PlatformHandleType, void> == false;
        { HandleDescriptor<HType>::InvalidHandle } -> std::convertible_to<typename HandleDescriptor<HType>::PlatformHandleType>;
        { HandleDescriptor<HType>::is_valid(HandleDescriptor<HType>::InvalidHandle) } -> std::convertible_to<bool>;
        { HandleDescriptor<HType>::close(HandleDescriptor<HType>::InvalidHandle) } -> std::convertible_to<bool>;
    };

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    struct Handle;

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    struct Handle final
    {
        using NativeDescriptor = HandleDescriptor<HType>;
        using NativeType = typename NativeDescriptor::PlatformHandleType;

        Handle() noexcept;
        explicit Handle(NativeType value) noexcept;
        Handle(Handle&& other) noexcept;
        Handle(Handle const& other) noexcept = delete;
        ~Handle() noexcept;

        auto operator=(Handle&& other) noexcept -> Handle&;
        auto operator=(Handle const& other) noexcept -> Handle& = delete;

        operator bool() const noexcept { return NativeDescriptor::is_valid(_handle); }

        auto native() const noexcept -> NativeType { return _handle; }
        bool close() noexcept;

    private:
        NativeType _handle;
    };


    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle() noexcept
        : _handle{ NativeDescriptor::InvalidHandle }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle(NativeType value) noexcept
        : _handle{ value }
    {
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    Handle<HType>::Handle(Handle&& other) noexcept
        : _handle{ ice::exchange(other._handle, NativeDescriptor::InvalidHandle) }
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
            _handle = ice::exchange(other._handle, NativeDescriptor::InvalidHandle);
        }
        return *this;
    }

    template<HandleType HType> requires (ValidHandleDescriptor<HType>)
    bool Handle<HType>::close() noexcept
    {
        if (*this)
        {
            return NativeDescriptor::close(ice::exchange(_handle, NativeDescriptor::InvalidHandle));
        }
        return false;
    }

} // namespace ice::os
