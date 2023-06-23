#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"
#include "IMGUI/imgui_internal.h"
#include <fstream>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class SpotifyTool : public BakkesMod::Plugin::BakkesModPlugin,
    public BakkesMod::Plugin::PluginSettingsWindow,
    public BakkesMod::Plugin::PluginWindow
{
private:
    std::string title, artist;
    uint8_t* data;
    bool isThumbnail = false;
    bool isWinRTInitialized = false;
    int snapping_grid_size_x = 100;
    int snapping_grid_size_y = 100;
    int screenSizeX = 1920;
    int screenSizeY = 1080;
    bool moveOverlay = false;
    bool snappingMode = false;
    bool keepRight = true;
    bool stoolEnabled = true;
    int next_keybind_index = 0;
    int previous_keybind_index = 0;
    int pause_keybind_index = 0;
    ImFont* myFont;
    bool isWindowOpen_ = false;
    bool isMinimized_ = false;
    bool show_color_picker = false;
    float colorText_r = 1.0f;
    float colorText_g = 0.0f;
    float colorText_b = 0.0f;
    float colorText_a = 1.0f;
    ImVec4 colorText = ImVec4(colorText_r, colorText_g, colorText_b, colorText_a);
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoFocusOnAppearing;

private:

    void DebugLog(std::string path, std::string info);
    virtual void onLoad();
    virtual void onUnload();
    void SetImGuiContext(uintptr_t ctx) override;
    void DragWidget(ImGuiWindow* window);
    void ShowColorPicker();
    void RenderSettings() override;
    void Render() override;
    std::string GetPluginName();
    std::string GetMenuName();
    std::string GetMenuTitle();
    bool ShouldBlockInput();
    bool IsActiveOverlay();
    void OnOpen();
    void OnClose();
    bool InitializeWinRT();
    void SyncSMTC();
    void SkipSongSMTC();
    void PreviousSongSMTC();
    void TogglePausePlaySongSMTC();
};