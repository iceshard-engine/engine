#pragma once
#include <ice/engine_data_storage.hxx>

namespace ice
{

    struct EngineFrameData
    {
        virtual ~EngineFrameData() noexcept = default;

        // virtual auto frame() noexcept -> ice::DataStorage& = 0;
        virtual auto runtime() noexcept -> ice::DataStorage& = 0;

        // virtual auto frame() const noexcept -> ice::DataStorage const& = 0;
        virtual auto runtime() const noexcept -> ice::DataStorage const& = 0;
        virtual auto persistent() const noexcept -> ice::DataStorage const& = 0;
    };

} // namespace ice
