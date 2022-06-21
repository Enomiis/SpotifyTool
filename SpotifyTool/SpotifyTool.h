#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

struct Song_name {
    std::string song = "Not Sync";
};

class SpotifyTool : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
    virtual void onLoad();
    virtual void onUnload();

    void RenderSettings() override;
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;
    void WriteInFile(std::string _filename, std::string _value);
    void DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar);
    void Render(CanvasWrapper canvas);
    void Sync_spotify();
    std::shared_ptr<ImageWrapper> background_v1;
    
};