#pragma once
#include <ice/unique_ptr.hxx>

namespace ice
{

    class EngineRunner;

    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        virtual auto create_runner() noexcept -> ice::UniquePtr<EngineRunner> = 0;
    };

} // namespace ice
