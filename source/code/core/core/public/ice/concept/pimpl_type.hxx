/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/concept/enum_flags.hxx>

namespace ice::concepts
{

    enum class PimplFlags : ice::u8
    {
        None = 0x0,
        Default = None,

        NoMoveSemantics = 0x01,
        NoCopySemantics = 0x02,
        NoMoveCopySemantics = NoMoveSemantics | NoCopySemantics,
    };

    class PimplType
    {
    protected:
        template<typename T>
        struct Internal;

    public:
        template<typename T>
        PimplType(Internal<T>* data) noexcept;

        // TODO: Move this somewhere else!
        PimplType(PimplType&& other) noexcept = delete;
        auto operator=(PimplType&& other) noexcept -> PimplType& = delete;
        PimplType(PimplType const& other) noexcept = delete;
        auto operator=(PimplType const& other) noexcept -> PimplType& = delete;

    protected:
        template<typename Self>
        auto internal(this Self& self) noexcept -> Internal<Self>&;

    private:
        void* _internal;
    };


    template<typename Self>
    inline PimplType::PimplType(Internal<Self>* data) noexcept
        : _internal{ data }
    {
    }

    template<typename Self>
    auto PimplType::internal(this Self& self) noexcept -> Internal<Self>&
    {
        return *reinterpret_cast<Internal<Self>*>(self._internal);
    }

} // namespace ice::concepts
