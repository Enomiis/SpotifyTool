#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"
#include <fstream>
#include <iostream>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class SpotifyTool : public BakkesMod::Plugin::BakkesModPlugin,
    public BakkesMod::Plugin::PluginSettingsWindow
    {
    private:
        string code_spotify, refresh_token, access_token, token, picture, artist,song_artist, auth_bearer,auth,playing,formated_playing;
        string song_file = "C:\\Users\\User\\AppData\\Roaming\\bakkesmod\\bakkesmod\\SpotifyTool\\song.txt";
        bool stoolEnabled = true;
        bool moveOverlay = false;
        string setup_status;
        string song;
        ifstream file, song_json;
        string response;
        string currently_playing;
        float timer,timer_reset;
        bool song_sync = true;
        bool inDragMode = false;
        float time, time2;
        ImFont* myFont;

    private:
        virtual void onLoad();
        virtual void onUnload();
        string GetPluginName();
        void SetImGuiContext(uintptr_t ctx) override;
        void WriteInFile(std::string _filename, std::string _value);
        string LoadofFile(std::string _filename);
        void DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar);
        void RenderSettings() override;
        void Sync_spotify();
        void Setup_spotify();
        void Refresh_token();
        string GetMenuTitle();
        bool IsActiveOverlay();
    };