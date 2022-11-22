#pragma once
#pragma comment( lib, "pluginsdk.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"
#include <fstream>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class SpotifyTool : public BakkesMod::Plugin::BakkesModPlugin,
    public BakkesMod::Plugin::PluginSettingsWindow,
    public BakkesMod::Plugin::PluginWindow
{
private:
    std::string code_spotify, refresh_token, access_token, token, picture, artist, auth_bearer, auth, song, currently_playing;
    bool setup_statut = false;
    bool stoolEnabled = true;
    bool moveOverlay = false;
    int duration_ms, duration, progress, progress_ms;
    bool song_sync = true;
    bool inDragMode = false;
    bool doOnce = true;
    bool skipped = true;
    float counter, token_denied, song_duration, skip_delay;
    ImFont* myFont;
    bool isWindowOpen_ = false;
    bool isMinimized_ = false;
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoFocusOnAppearing;
    class ImageLinkWrapper
    {
        std::shared_ptr<ImageWrapper> m_image = nullptr;
        std::string m_url;

    public:
        explicit ImageLinkWrapper(std::string url, std::shared_ptr<GameWrapper> gw)
            : m_url(std::move(url))
        {
            const auto tmp_name = std::to_string(std::hash<std::string>{}(m_url));
            const auto path = gw->GetDataFolder() / "imageCache" / tmp_name;
            if (exists(path))
            {
                LOG("using image cache");
                LoadImageData(path);
            }
            else
            {
                create_directories(path.parent_path());
                DownloadImage(path);
            }

        }

        [[nodiscard]] void* GetImguiPtr() const
        {
            return m_image != nullptr ? m_image->GetImGuiTex() : nullptr;
        }

    private:
        void DownloadImage(const std::filesystem::path& cache_path)
        {
            const CurlRequest req{ m_url, "GET" };
            HttpWrapper::SendCurlRequest(req, cache_path.wstring(), [this](int i, const std::wstring path)
                {
                    LOG("File downloaded: {}", i);
                    if (i == 200)
                    {
                        LoadImageData(path);
                    }
                });
        }

        void LoadImageData(const std::filesystem::path& path)
        {
            LOG("Loading image data");
            m_image = std::make_shared<ImageWrapper>(path, false, true);
        }
    };
    std::shared_ptr<ImageLinkWrapper> cover;

private:

    virtual void onLoad();
    virtual void onUnload();
    void SetImGuiContext(uintptr_t ctx) override;
    void DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar);
    void RenderSettings() override;
    void Render() override;
    void Sync_spotify();
    void Setup_spotify();
    void Refresh_token();
    void Skip_song();
    std::string GetPluginName();
    std::string GetMenuName();
    std::string GetMenuTitle();
    bool ShouldBlockInput();
    bool IsActiveOverlay();
    void OnOpen();
    void OnClose();
};