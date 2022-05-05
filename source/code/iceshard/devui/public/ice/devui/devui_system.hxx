#pragma once
#include <ice/stringid.hxx>
#include <ice/engine_devui.hxx>
#include <ice/world/world_trait_archive.hxx>

namespace ice::devui
{

    class DevUIWidget;

    class DevUISystem : public ice::EngineDevUI
    {
    public:
        virtual ~DevUISystem() noexcept = default;

        virtual void register_trait(
            ice::WorldTraitArchive& archive
        ) noexcept = 0;
    };

} // namespace ice::devui
