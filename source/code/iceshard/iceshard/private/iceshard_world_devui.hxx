#pragma once
#include "iceshard_world.hxx"

#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_module.hxx>
#include <ice/devui_widget.hxx>

namespace ice
{

    class IceshardWorld::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(
            ice::Allocator& alloc,
            ice::IceshardWorld& world
        ) noexcept;

        void build_content() noexcept override;

        ice::Shard world_operation;
    private:
        ice::Allocator& _allocator;
        ice::IceshardWorld& _world;
    };

} // namespace ice
