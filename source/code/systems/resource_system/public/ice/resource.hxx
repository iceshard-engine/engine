#pragma once
#include <ice/base.hxx>
#include <ice/data.hxx>

namespace ice
{

    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        virtual auto name() const noexcept -> ice::String = 0;

        virtual auto location() const noexcept -> ice::URI const& = 0;

        virtual auto data() noexcept -> ice::Data = 0;

        virtual auto metadata() noexcept -> ice::Data = 0;
    };


} // namespace ice
