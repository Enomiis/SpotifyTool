#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"
#include <fstream>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class SpotifyTool : public BakkesMod::Plugin::BakkesModPlugin,
    public BakkesMod::Plugin::PluginSettingsWindow
    {
    private:
        std::string code_spotify, refresh_token, access_token, token, picture, artist,song_artist, auth_bearer,auth,playing,formated_playing;
        std::string song_file = "C:\\Users\\User\\AppData\\Roaming\\bakkesmod\\bakkesmod\\SpotifyTool\\song.txt";
        bool stoolEnabled = true;
        bool moveOverlay = false;
        std::string setup_status;
        std::string song;
        std::ifstream file, song_json;
        std::string response;
        std::string currently_playing;
        float timer,timer_reset;
        bool song_sync = true;
        bool inDragMode = false;
        float time, time2;
        ImFont* myFont;
        bool isWindowOpen_ = false;
        bool isMinimized_ = false;
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoFocusOnAppearing;
        
    private:
        
        virtual void onLoad();
        virtual void onUnload();
        void SetImGuiContext(uintptr_t ctx) override;
        void WriteInFile(std::string _filename, std::string _value);
        std::string LoadofFile(std::string _filename);
        void DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar);
        void RenderSettings() override;
        void Render();
        void Sync_spotify();
        void Setup_spotify();
        void Refresh_token();
        std::string GetPluginName();
        std::string GetMenuName();
        std::string GetMenuTitle();
        bool ShouldBlockInput();
        bool IsActiveOverlay();
        void OnOpen();
        void OnClose();
    
    };