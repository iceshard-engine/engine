#include <ice/color.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/string_utils.hxx>
#include "imgui_style_palette.hxx"

#include <imgui/imgui.h>
#undef assert

namespace ice::devui::styles
{

    enum Color : ice::u32
    {
        Color_AccentPrimary = 0,
        Color_AccentSecondary,
        Color_Surface,
        Color_ElevatedSurface,
        Color_TextPrimary,
        Color_TextSecondary,
        Color_Border,
        Color_Shadow,
        Color_DisabledFill,
        Color_Loading,
        Color_Error,
        Color_ErrorBackground,
        Color_Success,
        Color_SuccessBackground,
        Color_SelectedTint,
        Color_FocusRing,
    };

    static constexpr ice::Color<ice::f32> Colors_DarkMode[]{
        ice::Color<ice::f32>{ 0.4980f, 0.7804f, 1.0000f, 1.f },
        ice::Color<ice::f32>{ 0.3765f, 0.8392f, 0.7843f, 1.f },
        ice::Color<ice::f32>{ 0.0275f, 0.0784f, 0.1569f, 1.f },
        ice::Color<ice::f32>{ 0.0549f, 0.1412f, 0.2000f, 1.f },
        ice::Color<ice::f32>{ 0.9020f, 0.9647f, 1.0000f, 1.f },
        ice::Color<ice::f32>{ 0.6235f, 0.7294f, 0.8039f, 1.f },
        ice::Color<ice::f32>{ 0.1176f, 0.2275f, 0.2902f, 1.f },
        ice::Color<ice::f32>{ 0.0000f, 0.0000f, 0.0000f, 0.48f },
        ice::Color<ice::f32>{ 0.0275f, 0.1255f, 0.1647f, 1.f },
        ice::Color<ice::f32>{ 0.0431f, 0.1647f, 0.2235f, 1.f },
        ice::Color<ice::f32>{ 1.0000f, 0.4824f, 0.4980f, 1.f },
        ice::Color<ice::f32>{ 0.2471f, 0.1176f, 0.1255f, 1.f },
        ice::Color<ice::f32>{ 0.4745f, 0.8941f, 0.6039f, 1.f },
        ice::Color<ice::f32>{ 0.0549f, 0.1686f, 0.1137f, 1.f },
        ice::Color<ice::f32>{ 0.0314f, 0.2078f, 0.2824f, 1.f },
        ice::Color<ice::f32>{ 0.1647f, 0.6196f, 1.0000f, 1.f },
    };

    static constexpr ice::Color<ice::f32> Colors_LightMode[]{
        ice::Color<ice::f32>{ 0.1176f, 0.4353f, 0.7216f, 1.f },
        ice::Color<ice::f32>{ 0.0549f, 0.6039f, 0.5804f, 1.f },
        ice::Color<ice::f32>{ 1.0000f, 1.0000f, 1.0000f, 1.f },
        ice::Color<ice::f32>{ 0.9647f, 0.9765f, 0.9843f, 1.f },
        ice::Color<ice::f32>{ 0.0431f, 0.1059f, 0.1686f, 1.f },
        ice::Color<ice::f32>{ 0.3216f, 0.4078f, 0.4588f, 1.f },
        ice::Color<ice::f32>{ 0.8824f, 0.9098f, 0.9333f, 1.f },
        ice::Color<ice::f32>{ 0.0078f, 0.1765f, 0.3216f, 0.08f },
        ice::Color<ice::f32>{ 0.9412f, 0.9569f, 0.9686f, 1.f },
        ice::Color<ice::f32>{ 0.9137f, 0.9373f, 0.9529f, 1.f },
        ice::Color<ice::f32>{ 0.8392f, 0.2706f, 0.2902f, 1.f },
        ice::Color<ice::f32>{ 1.0000f, 0.9529f, 0.9529f, 1.f },
        ice::Color<ice::f32>{ 0.1176f, 0.6078f, 0.2902f, 1.f },
        ice::Color<ice::f32>{ 0.9529f, 0.9843f, 0.9608f, 1.f },
        ice::Color<ice::f32>{ 0.8471f, 0.9333f, 1.0000f, 1.f },
        ice::Color<ice::f32>{ 0.7451f, 0.9020f, 1.0000f, 1.f },
    };

    static constexpr std::tuple<ImGuiCol, Color> ColorMappings[]{
        { ImGuiCol_Text, Color_TextPrimary },
        { ImGuiCol_WindowBg, Color_Surface },
        { ImGuiCol_ChildBg, Color_ElevatedSurface },
        { ImGuiCol_ChildBg, Color_ElevatedSurface },
        { ImGuiCol_Border, Color_Border },
        { ImGuiCol_BorderShadow, Color_Shadow },
        { ImGuiCol_FrameBg, Color_DisabledFill },
        { ImGuiCol_FrameBgHovered, Color_AccentPrimary },
        { ImGuiCol_FrameBgActive, Color_AccentSecondary },
        { ImGuiCol_TitleBg, Color_Surface },
        { ImGuiCol_TitleBgActive, Color_AccentPrimary },
        { ImGuiCol_TitleBgCollapsed, Color_Surface },
        { ImGuiCol_MenuBarBg, Color_Surface },
        { ImGuiCol_ScrollbarBg, Color_Surface },
        { ImGuiCol_ScrollbarGrab, Color_Border },
        { ImGuiCol_ScrollbarGrabHovered, Color_AccentPrimary },
        { ImGuiCol_ScrollbarGrabActive, Color_AccentSecondary },
        { ImGuiCol_CheckMark, Color_AccentPrimary },
        { ImGuiCol_SliderGrab, Color_AccentPrimary },
        { ImGuiCol_SliderGrabActive, Color_AccentSecondary },
        { ImGuiCol_Button, Color_AccentPrimary },
        { ImGuiCol_ButtonHovered, Color_AccentSecondary },
        { ImGuiCol_ButtonActive, Color_AccentSecondary },
        { ImGuiCol_Header, Color_AccentPrimary },
        { ImGuiCol_HeaderHovered, Color_AccentSecondary },
        { ImGuiCol_HeaderActive, Color_AccentSecondary },
        { ImGuiCol_Separator, Color_Border },
        { ImGuiCol_SeparatorHovered, Color_AccentPrimary },
        { ImGuiCol_SeparatorActive, Color_AccentSecondary },
        { ImGuiCol_ResizeGrip, Color_Border },
        { ImGuiCol_ResizeGripHovered, Color_AccentPrimary },
        { ImGuiCol_ResizeGripActive, Color_AccentSecondary },
        { ImGuiCol_Tab, Color_AccentPrimary },
        { ImGuiCol_TabHovered, Color_AccentSecondary },
        { ImGuiCol_TabActive, Color_AccentSecondary },
        { ImGuiCol_TabUnfocused, Color_AccentPrimary },
        { ImGuiCol_TabUnfocusedActive, Color_AccentSecondary },
        { ImGuiCol_DockingPreview, Color_AccentPrimary },
        { ImGuiCol_DockingEmptyBg, Color_Surface },
        { ImGuiCol_PlotLines, Color_AccentPrimary },
        { ImGuiCol_PlotLinesHovered, Color_AccentSecondary },
        { ImGuiCol_PlotHistogram, Color_AccentPrimary },
        { ImGuiCol_PlotHistogramHovered, Color_AccentSecondary },
        { ImGuiCol_TableHeaderBg, Color_Surface },
        { ImGuiCol_TableBorderStrong, Color_Border },
        { ImGuiCol_TableBorderLight, Color_Border },
        { ImGuiCol_TableRowBg, Color_Surface },
        { ImGuiCol_TableRowBgAlt, Color_ElevatedSurface },
        { ImGuiCol_TextSelectedBg, Color_SelectedTint },
        { ImGuiCol_DragDropTarget, Color_AccentPrimary },
        { ImGuiCol_NavHighlight, Color_FocusRing },
        { ImGuiCol_NavWindowingHighlight, Color_FocusRing },
        { ImGuiCol_NavWindowingDimBg, Color_Shadow },
        { ImGuiCol_ModalWindowDimBg, Color_Shadow },
    };

    static constexpr std::tuple<ImGuiCol, ice::Color<ice::u8>> ColorMappingsU8[]{
        { ImGuiCol_Text,                ice::Color<ice::u8>{ 210,220,235,255 } },
        { ImGuiCol_WindowBg,            ice::Color<ice::u8>{ 15,20,28,255 } },
        { ImGuiCol_ChildBg,             ice::Color<ice::u8>{ 18,26,36,255 } },
        { ImGuiCol_PopupBg,             ice::Color<ice::u8>{ 18,26,36,255 } },
        { ImGuiCol_Border,              ice::Color<ice::u8>{ 40,60,75,255 } },
        { ImGuiCol_BorderShadow,        ice::Color<ice::u8>{ 10,15,20,255 } },
        { ImGuiCol_FrameBg,             ice::Color<ice::u8>{ 25,38,52,255 } },
        { ImGuiCol_FrameBgHovered,      ice::Color<ice::u8>{ 35,65,70,255 } },
        { ImGuiCol_FrameBgActive,       ice::Color<ice::u8>{ 45,95,100,255 } },
        { ImGuiCol_TitleBg,             ice::Color<ice::u8>{ 20,30,45,255 } },
        { ImGuiCol_TitleBgActive,       ice::Color<ice::u8>{ 30,50,65,255 } },
        { ImGuiCol_TitleBgCollapsed,    ice::Color<ice::u8>{ 15,22,30,200 } },
        { ImGuiCol_MenuBarBg,           ice::Color<ice::u8>{ 22,32,45,255 } },
        { ImGuiCol_ScrollbarBg,         ice::Color<ice::u8>{ 15,20,28,180 } },
        { ImGuiCol_ScrollbarGrab,       ice::Color<ice::u8>{ 45,95,100,255 } },
        { ImGuiCol_ScrollbarGrabHovered,ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_ScrollbarGrabActive, ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_CheckMark,           ice::Color<ice::u8>{ 160,210,215,255 } },
        { ImGuiCol_SliderGrab,          ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_SliderGrabActive,    ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_Button,              ice::Color<ice::u8>{ 30,50,65,255 } },
        { ImGuiCol_ButtonHovered,       ice::Color<ice::u8>{ 45,95,100,255 } },
        { ImGuiCol_ButtonActive,        ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_Header,              ice::Color<ice::u8>{ 30,50,65,255 } },
        { ImGuiCol_HeaderHovered,       ice::Color<ice::u8>{ 45,95,100,255 } },
        { ImGuiCol_HeaderActive,        ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_Separator,           ice::Color<ice::u8>{ 40,60,75,255 } },
        { ImGuiCol_SeparatorHovered,    ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_SeparatorActive,     ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_ResizeGrip,          ice::Color<ice::u8>{ 45,95,100,180 } },
        { ImGuiCol_ResizeGripHovered,   ice::Color<ice::u8>{ 65,125,130,220 } },
        { ImGuiCol_ResizeGripActive,    ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_Tab,                 ice::Color<ice::u8>{ 25,38,52,255 } },
        { ImGuiCol_TabHovered,          ice::Color<ice::u8>{ 45,95,100,255 } },
        { ImGuiCol_TabActive,           ice::Color<ice::u8>{ 65,125,130,255 } },
        { ImGuiCol_TabUnfocused,        ice::Color<ice::u8>{ 25,38,52,200 } },
        { ImGuiCol_TabUnfocusedActive,  ice::Color<ice::u8>{ 45,95,100,220 } },
        { ImGuiCol_DockingPreview,      ice::Color<ice::u8>{ 85,155,160,180 } },
        { ImGuiCol_DockingEmptyBg,      ice::Color<ice::u8>{ 10,15,20,255 } },
        { ImGuiCol_PlotLines,           ice::Color<ice::u8>{ 120,175,180,255 } },
        { ImGuiCol_PlotLinesHovered,    ice::Color<ice::u8>{ 160,210,215,255 } },
        { ImGuiCol_PlotHistogram,       ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_PlotHistogramHovered,ice::Color<ice::u8>{ 160,210,215,255 } },
        { ImGuiCol_TableHeaderBg,       ice::Color<ice::u8>{ 30,45,65,255 } },
        { ImGuiCol_TableBorderStrong,   ice::Color<ice::u8>{ 40,60,75,255 } },
        { ImGuiCol_TableBorderLight,    ice::Color<ice::u8>{ 25,38,52,255 } },
        { ImGuiCol_TableRowBg,          ice::Color<ice::u8>{ 20,30,45,255 } },
        { ImGuiCol_TableRowBgAlt,       ice::Color<ice::u8>{ 25,38,52,255 } },
        { ImGuiCol_TextSelectedBg,      ice::Color<ice::u8>{ 65,125,130,180 } },
        { ImGuiCol_DragDropTarget,      ice::Color<ice::u8>{ 160,210,215,255 } },
        { ImGuiCol_NavHighlight,        ice::Color<ice::u8>{ 85,155,160,255 } },
        { ImGuiCol_NavWindowingHighlight,ice::Color<ice::u8>{ 160,210,215,200 } },
        { ImGuiCol_NavWindowingDimBg,   ice::Color<ice::u8>{ 10,15,20,200 } },
        { ImGuiCol_ModalWindowDimBg,    ice::Color<ice::u8>{ 10,15,20,220 } },
    };


    static constexpr std::tuple<ImGuiCol, ice::Color<ice::u8>> ColorMappingsU8_Light[]{
        { ImGuiCol_Text,                ice::Color<ice::u8>{ 20,30,40,255 } },
        { ImGuiCol_WindowBg,            ice::Color<ice::u8>{ 245,248,250,255 } },
        { ImGuiCol_ChildBg,             ice::Color<ice::u8>{ 252,254,255,255 } },
        { ImGuiCol_PopupBg,             ice::Color<ice::u8>{ 252,254,255,255 } },
        { ImGuiCol_Border,              ice::Color<ice::u8>{ 160,180,190,255 } },
        { ImGuiCol_BorderShadow,        ice::Color<ice::u8>{ 220,230,235,255 } },
        { ImGuiCol_FrameBg,             ice::Color<ice::u8>{ 230,238,242,255 } },
        { ImGuiCol_FrameBgHovered,      ice::Color<ice::u8>{ 150,190,195,255 } },
        { ImGuiCol_FrameBgActive,       ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_TitleBg,             ice::Color<ice::u8>{ 225,235,240,255 } },
        { ImGuiCol_TitleBgActive,       ice::Color<ice::u8>{ 150,190,195,255 } },
        { ImGuiCol_TitleBgCollapsed,    ice::Color<ice::u8>{ 235,242,245,200 } },
        { ImGuiCol_MenuBarBg,           ice::Color<ice::u8>{ 235,242,245,255 } },
        { ImGuiCol_ScrollbarBg,         ice::Color<ice::u8>{ 245,248,250,200 } },
        { ImGuiCol_ScrollbarGrab,       ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_ScrollbarGrabHovered,ice::Color<ice::u8>{ 70,120,125,255 } },
        { ImGuiCol_ScrollbarGrabActive, ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_CheckMark,           ice::Color<ice::u8>{ 20,110,115,255 } },
        { ImGuiCol_SliderGrab,          ice::Color<ice::u8>{ 40,120,125,255 } },
        { ImGuiCol_SliderGrabActive,    ice::Color<ice::u8>{ 20,90,95,255 } },
        { ImGuiCol_Button,              ice::Color<ice::u8>{ 190,220,225,255 } },
        { ImGuiCol_ButtonHovered,       ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_ButtonActive,        ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_Header,              ice::Color<ice::u8>{ 190,220,225,255 } },
        { ImGuiCol_HeaderHovered,       ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_HeaderActive,        ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_Separator,           ice::Color<ice::u8>{ 160,180,190,255 } },
        { ImGuiCol_SeparatorHovered,    ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_SeparatorActive,     ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_ResizeGrip,          ice::Color<ice::u8>{ 100,150,155,180 } },
        { ImGuiCol_ResizeGripHovered,   ice::Color<ice::u8>{ 70,120,125,220 } },
        { ImGuiCol_ResizeGripActive,    ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_Tab,                 ice::Color<ice::u8>{ 230,238,242,255 } },
        { ImGuiCol_TabHovered,          ice::Color<ice::u8>{ 100,150,155,255 } },
        { ImGuiCol_TabActive,           ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_TabUnfocused,        ice::Color<ice::u8>{ 230,238,242,200 } },
        { ImGuiCol_TabUnfocusedActive,  ice::Color<ice::u8>{ 100,150,155,220 } },
        { ImGuiCol_DockingPreview,      ice::Color<ice::u8>{ 40,90,95,180 } },
        { ImGuiCol_DockingEmptyBg,      ice::Color<ice::u8>{ 250,252,254,255 } },
        { ImGuiCol_PlotLines,           ice::Color<ice::u8>{ 40,120,125,255 } },
        { ImGuiCol_PlotLinesHovered,    ice::Color<ice::u8>{ 20,90,95,255 } },
        { ImGuiCol_PlotHistogram,       ice::Color<ice::u8>{ 40,120,125,255 } },
        { ImGuiCol_PlotHistogramHovered,ice::Color<ice::u8>{ 20,90,95,255 } },
        { ImGuiCol_TableHeaderBg,       ice::Color<ice::u8>{ 225,235,240,255 } },
        { ImGuiCol_TableBorderStrong,   ice::Color<ice::u8>{ 160,180,190,255 } },
        { ImGuiCol_TableBorderLight,    ice::Color<ice::u8>{ 230,238,242,255 } },
        { ImGuiCol_TableRowBg,          ice::Color<ice::u8>{ 250,252,254,255 } },
        { ImGuiCol_TableRowBgAlt,       ice::Color<ice::u8>{ 240,246,248,255 } },
        { ImGuiCol_TextSelectedBg,      ice::Color<ice::u8>{ 100,150,155,180 } },
        { ImGuiCol_DragDropTarget,      ice::Color<ice::u8>{ 20,90,95,255 } },
        { ImGuiCol_NavHighlight,        ice::Color<ice::u8>{ 40,90,95,255 } },
        { ImGuiCol_NavWindowingHighlight,ice::Color<ice::u8>{ 20,90,95,200 } },
        { ImGuiCol_NavWindowingDimBg,   ice::Color<ice::u8>{ 245,248,250,200 } },
        { ImGuiCol_ModalWindowDimBg,    ice::Color<ice::u8>{ 245,248,250,220 } },
    };


    static constexpr std::tuple<ImGuiCol, ice::Color<ice::u8>> ColorMappingsU8_Dark[]{
        { ImGuiCol_Text,                ice::Color<ice::u8>{ 215,225,240,255 } },
        { ImGuiCol_WindowBg,            ice::Color<ice::u8>{ 12,18,28,255 } },
        { ImGuiCol_ChildBg,             ice::Color<ice::u8>{ 16,24,36,255 } },
        { ImGuiCol_PopupBg,             ice::Color<ice::u8>{ 16,24,36,255 } },
        { ImGuiCol_Border,              ice::Color<ice::u8>{ 40,60,85,255 } },
        { ImGuiCol_BorderShadow,        ice::Color<ice::u8>{ 8,12,18,255 } },
        { ImGuiCol_FrameBg,             ice::Color<ice::u8>{ 22,34,50,255 } },
        { ImGuiCol_FrameBgHovered,      ice::Color<ice::u8>{ 40,70,105,255 } },
        { ImGuiCol_FrameBgActive,       ice::Color<ice::u8>{ 60,100,145,255 } },
        { ImGuiCol_TitleBg,             ice::Color<ice::u8>{ 18,28,42,255 } },
        { ImGuiCol_TitleBgActive,       ice::Color<ice::u8>{ 28,44,65,255 } },
        { ImGuiCol_TitleBgCollapsed,    ice::Color<ice::u8>{ 12,18,28,200 } },
        { ImGuiCol_MenuBarBg,           ice::Color<ice::u8>{ 20,30,46,255 } },
        { ImGuiCol_ScrollbarBg,         ice::Color<ice::u8>{ 12,18,28,180 } },
        { ImGuiCol_ScrollbarGrab,       ice::Color<ice::u8>{ 60,100,145,255 } },
        { ImGuiCol_ScrollbarGrabHovered,ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_ScrollbarGrabActive, ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_CheckMark,           ice::Color<ice::u8>{ 185,220,250,255 } },
        { ImGuiCol_SliderGrab,          ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_SliderGrabActive,    ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_Button,              ice::Color<ice::u8>{ 28,44,65,255 } },
        { ImGuiCol_ButtonHovered,       ice::Color<ice::u8>{ 60,100,145,255 } },
        { ImGuiCol_ButtonActive,        ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_Header,              ice::Color<ice::u8>{ 28,44,65,255 } },
        { ImGuiCol_HeaderHovered,       ice::Color<ice::u8>{ 60,100,145,255 } },
        { ImGuiCol_HeaderActive,        ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_Separator,           ice::Color<ice::u8>{ 40,60,85,255 } },
        { ImGuiCol_SeparatorHovered,    ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_SeparatorActive,     ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_ResizeGrip,          ice::Color<ice::u8>{ 60,100,145,180 } },
        { ImGuiCol_ResizeGripHovered,   ice::Color<ice::u8>{ 85,135,185,220 } },
        { ImGuiCol_ResizeGripActive,    ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_Tab,                 ice::Color<ice::u8>{ 22,34,50,255 } },
        { ImGuiCol_TabHovered,          ice::Color<ice::u8>{ 60,100,145,255 } },
        { ImGuiCol_TabActive,           ice::Color<ice::u8>{ 85,135,185,255 } },
        { ImGuiCol_TabUnfocused,        ice::Color<ice::u8>{ 22,34,50,200 } },
        { ImGuiCol_TabUnfocusedActive,  ice::Color<ice::u8>{ 60,100,145,220 } },
        { ImGuiCol_DockingPreview,      ice::Color<ice::u8>{ 110,165,215,180 } },
        { ImGuiCol_DockingEmptyBg,      ice::Color<ice::u8>{ 8,12,18,255 } },
        { ImGuiCol_PlotLines,           ice::Color<ice::u8>{ 135,185,225,255 } },
        { ImGuiCol_PlotLinesHovered,    ice::Color<ice::u8>{ 185,220,250,255 } },
        { ImGuiCol_PlotHistogram,       ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_PlotHistogramHovered,ice::Color<ice::u8>{ 185,220,250,255 } },
        { ImGuiCol_TableHeaderBg,       ice::Color<ice::u8>{ 28,44,65,255 } },
        { ImGuiCol_TableBorderStrong,   ice::Color<ice::u8>{ 40,60,85,255 } },
        { ImGuiCol_TableBorderLight,    ice::Color<ice::u8>{ 22,34,50,255 } },
        { ImGuiCol_TableRowBg,          ice::Color<ice::u8>{ 18,28,42,255 } },
        { ImGuiCol_TableRowBgAlt,       ice::Color<ice::u8>{ 22,34,50,255 } },
        { ImGuiCol_TextSelectedBg,      ice::Color<ice::u8>{ 85,135,185,180 } },
        { ImGuiCol_DragDropTarget,      ice::Color<ice::u8>{ 185,220,250,255 } },
        { ImGuiCol_NavHighlight,        ice::Color<ice::u8>{ 110,165,215,255 } },
        { ImGuiCol_NavWindowingHighlight,ice::Color<ice::u8>{ 185,220,250,200 } },
        { ImGuiCol_NavWindowingDimBg,   ice::Color<ice::u8>{ 8,12,18,200 } },
        { ImGuiCol_ModalWindowDimBg,    ice::Color<ice::u8>{ 8,12,18,220 } },
    };

    template<ice::u32 Size>
    inline auto apply_color_theme(std::tuple<ImGuiCol, ice::Color<ice::u8>> const(&Colors)[Size]) noexcept
    {
        for (ice::u32 i = 0; i < ice::count(Colors); ++i)
        {
            auto const& [imgui_col, style_col] = Colors[i];
            ImGui::PushStyleColor(imgui_col, ImGui::ToColor(style_col));
        }
    }

    void apply_color_theme(Theme theme) noexcept
    {
        switch (theme)
        {
        case Theme::Dark:
            apply_color_theme(ColorMappingsU8_Dark);
            break;
        case Theme::Light:
            apply_color_theme(ColorMappingsU8_Light);
            break;
        default:
            apply_color_theme(ColorMappingsU8_Dark);
            break;
        }
    }

    void pop_color_theme() noexcept
    {
        ImGui::PopStyleColor(ice::count(ColorMappingsU8_Dark));
    }

    auto apply_stylesheet() noexcept -> ice::u32
    {
        int vars = 0;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.6f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.0f, 8.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 32.0f, 32.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.0f, 0.5f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 8.0f, 4.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 4.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 4.0f, 4.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 21.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 4.0f, 2.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 8.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 8.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0.0f, 0.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 1.0f); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, { 0.5f, 0.5f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, { 4.0f, 0.0f }); ++vars;
        ImGui::PushStyleVar(ImGuiStyleVar_DockingSeparatorSize, 4.0f); ++vars;
        return vars;
    }

    void pop_stylesheet() noexcept
    {
        ImGui::PopStyleVar(30);
    }

} // namespace ice::style


namespace ice::devui
{

    ImGuiStylePalette::ImGuiStylePalette(ice::Allocator& alloc) noexcept
        : DevUIWidget{ DevUIWidgetInfo{.category = "Help", .name = "Style Palette"} }
    {
    }

    void ImGuiStylePalette::build_content() noexcept
    {
        ImGui::TextUnformatted("Style Palette");
        ImGui::Separator();
        ImGui::BeginTable("StylePaletteTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable);
        ImGui::TableSetupColumn("Dark Mode", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Light Mode", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::BeginChild("StylePaletteChildDark", { 0, 500 }, true, ImGuiWindowFlags_HorizontalScrollbar);
        float const box_size = 24.0f;
        float const box_spacing = 4.0f;
        float const text_height = ImGui::GetTextLineHeightWithSpacing();
        float const line_height = box_size + box_spacing * 2.0f;
        float const text_offset_y = (line_height - text_height) * 0.5f;
        for (ice::u32 i = 0; i < ice::count(ice::devui::styles::Colors_DarkMode); ++i)
        {
            ice::Color<ice::f32> const& color_value = ice::devui::styles::Colors_DarkMode[i];
            ImGui::PushID(i);
            ImGui::ColorButton("##Color", { color_value.r, color_value.g, color_value.b, color_value.a }, ImGuiColorEditFlags_NoTooltip, { box_size, box_size });
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset_y);
            ImGui::TextT("Color {}: R={:.2} G={:.2} B={:.2} A={:.2}", i, color_value.r, color_value.g, color_value.b, color_value.a);
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("StylePaletteChildLight", { 0, 500 }, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (ice::u32 i = 0; i < ice::count(ice::devui::styles::Colors_LightMode); ++i)
        {
            ice::Color<ice::f32> const& color_value = ice::devui::styles::Colors_LightMode[i];
            ImGui::PushID(i + 128);
            ImGui::ColorButton("##Color", { color_value.r, color_value.g, color_value.b, color_value.a }, ImGuiColorEditFlags_NoTooltip, { box_size, box_size });
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset_y);
            ImGui::TextT("Color {}: R={:.2} G={:.2} B={:.2} A={:.2}", i, color_value.r, color_value.g, color_value.b, color_value.a);
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::EndTable();

        ImGui::Separator();

        ImGui::TextUnformatted("This is a preview of the current style applied to the UI.");
        ImGui::Separator();

        ice::devui::styles::apply_stylesheet();
        ice::devui::styles::apply_color_theme(ice::devui::styles::ColorMappingsU8_Light);
        {
            ImGui::PushID("Dark");
            bool open = true;
            ImGui::ShowDemoWindow(&open);
            ImGui::PopID();
        }
        ice::devui::styles::pop_color_theme();
        ice::devui::styles::pop_stylesheet();
    }

} // namespace ice::devui
