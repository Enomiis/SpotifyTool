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
        std::string code, refresh_token, access_token, token, picture, artist,song_artist, auth_bearer,auth,playing,formated_playing;
        std::string song_file = "C:\\Users\\User\\AppData\\Roaming\\bakkesmod\\bakkesmod\\SpotifyTool\\song.txt";
        bool stoolEnabled = true;
        std::string setup_status;
        std::string song;
        std::ifstream file, song_json;
        std::string response;
        std::string currently_playing;
        float timer,timer_reset;
        bool song_sync = true;
        float time;

    private:
        virtual void onLoad();
        virtual void onUnload();
        void RenderSettings() override;
        std::string GetPluginName() override;
        void SetImGuiContext(uintptr_t ctx) override;
        void WriteInFile(std::string _filename, std::string _value);
        std::string LoadofFile(std::string _filename);
        void DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar);
        void Render(CanvasWrapper canvas);
        void Sync_spotify();
        std::shared_ptr<ImageWrapper> background_v1;
    };