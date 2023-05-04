#include "pch.h"
#include "SpotifyTool.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include "nlohmann/json.hpp"
#include <string>
#include <regex>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"

BAKKESMOD_PLUGIN(SpotifyTool, "Music Manager Plugin", "0.1.1", PERMISSION_ALL)
using namespace std;
using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using json = nlohmann::json;
shared_ptr < CVarManagerWrapper > _globalCvarManager;

string NEXT_KEYBIND = "None";
string PREVIOUS_KEYBIND = "None";
string PAUSE_KEYBIND = "None";

const string NEXT_HOTKEY = "stool_next_hotkey";
const string PREVIOUS_HOTKEY = "stool_previous_hotkey";
const string PAUSE_HOTKEY = "stool_pause_hotkey";

const string NEXT_KEYBIND_INDEX = "stool_next_keybind_index";
const string PREVIOUS_KEYBIND_INDEX = "stool_previous_keybind_index";
const string PAUSE_KEYBIND_INDEX = "stool_pause_keybind_index";

const string COLOR_TEXT_R = "stool_colorText_r";
const string COLOR_TEXT_G = "stool_colorText_g";
const string COLOR_TEXT_B = "stool_colorText_b";
const string COLOR_TEXT_A = "stool_colorText_a";

static string ROOT_DIRECTORY = "";
static string LOG_FILE_PATH = "";
static string LOGO_FILE_PATH = "";

void SpotifyTool::DebugLog(string path, string info) {
    fstream file(path, ios::app);

    time_t t = time(nullptr);
    tm tm = *localtime(&t);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H-%M-%S", &tm);
    string timeString(timeStr);

    if (!file.is_open()) {
        return;
    }

    string text = "\n[" + timeString + "] " + info;
    file.write(text.c_str(), text.length());

    file.close();
}

void SpotifyTool::onLoad() {
    _globalCvarManager = cvarManager;
    ROOT_DIRECTORY = gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\";
    LOG_FILE_PATH = ROOT_DIRECTORY + "stool_log.log";
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Initializing WinRT communication...");
    isWinRTInitialized = InitializeWinRT();
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Setting up the config and the directories...");
    LOGO_FILE_PATH = ROOT_DIRECTORY + "stool_logo.png";
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Updating game resolution...");
    screenSizeX = gameWrapper->GetScreenSize().X;
    screenSizeY = gameWrapper->GetScreenSize().Y;
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering togglemenu command...");
    gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
        cvarManager->executeCommand("togglemenu " + GetMenuName());
        }, 1);
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering WinRT SMTC synchronization notifier...");
    cvarManager->registerNotifier("SyncSMTC", [this](vector < string > args) {
        SyncSMTC();
        }, "", PERMISSION_ALL);
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the plugin enabled variable...");
    cvarManager->registerCvar("stool_enabled", "1", "Enable Music Manager", true, true, 0, true, 1)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        stoolEnabled = cvar.getBoolValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the keybind index for the Next song hotkey...");
    cvarManager->registerCvar(NEXT_KEYBIND_INDEX, "0", "Index for the next hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        next_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the keybind index for the Previous song hotkey...");
    cvarManager->registerCvar(PREVIOUS_KEYBIND_INDEX, "0", "Index for the previous hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        previous_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the keybind index for the Pause song hotkey...");
    cvarManager->registerCvar(PAUSE_KEYBIND_INDEX, "0", "Index for the pause hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        pause_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the R-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_R, "1.0f", "R-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_r = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the G-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_G, "0.0f", "G-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_g = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the B-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_B, "0.0f", "B-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_b = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the A-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_A, "1.0f", "A-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_a = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Updating the color vector...");
    colorText = ImVec4(colorText_r, colorText_g, colorText_b, colorText_a);
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the hotkey to skip the current song...");
    cvarManager->registerCvar(NEXT_HOTKEY, NEXT_KEYBIND, "Next song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering the command notifier to skip the current song...");
    cvarManager->registerNotifier(
        "SkipSong",
        [this](vector < string > args) {
            SkipSongSMTC();
        },
        "Skip the current song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the hotkey to return to the previous song...");
    cvarManager->registerCvar(PREVIOUS_HOTKEY, PREVIOUS_KEYBIND, "Previous song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering the command notifier to return to the previous song...");
    cvarManager->registerNotifier(
        "PrevSong",
        [this](vector < string > args) {
            PreviousSongSMTC();
        },
        "Jump to the previous song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering/Fetching the hotkey to pause the current song...");
    cvarManager->registerCvar(PAUSE_HOTKEY, PAUSE_KEYBIND, "Pause/Resume song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Registering the command notifier to pause the current song...");
    cvarManager->registerNotifier(
        "PlayPauseSong",
        [this](vector < string > args) {
            TogglePausePlaySongSMTC();
        },
        "Pause/Resume the current song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Loading the logo to display as a texture...");
    gameWrapper->LoadToastTexture("stool_logo", LOGO_FILE_PATH);
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Loading the toast to display...");
    gameWrapper->Toast("MusicManager", "MusicManager loaded!", "stool_logo", 5.0, ToastType_Warning);
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onLoad() -> Finished initializing.");
}

void SpotifyTool::onUnload() {
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onUnload() -> Unloading the saved keybinds...");
    CVarWrapper nextHotkeyCVar = cvarManager->getCvar(NEXT_HOTKEY);
    CVarWrapper previousHotkeyCVar = cvarManager->getCvar(PREVIOUS_HOTKEY);
    CVarWrapper pauseHotkeyCVar = cvarManager->getCvar(PAUSE_HOTKEY);
    if (nextHotkeyCVar) {
        string bind_next = nextHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_next);
        DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onUnload() -> Successfully removed the skip the current song hotkey bind!");
    }
    if (previousHotkeyCVar) {
        string bind_previous = previousHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_previous);
        DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onUnload() -> Successfully removed the go to the previous song hotkey bind!");
    }
    if (pauseHotkeyCVar) {
        string bind_pause = pauseHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_pause);
        DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onUnload() -> Successfully removed the pause the current song hotkey bind!");
    }
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::onUnload() -> Unloading completed.");
}

string SpotifyTool::GetPluginName() {
    return "MusicManager Beta";
}

#pragma region Rendering
void SpotifyTool::RenderSettings() {
    const char* keybinds[] = {
        "None",
        "One",
        "Two",
        "Three",
        "Four",
        "Five",
        "Six",
        "Seven",
        "Eight",
        "Nine",
        "Zero",
        "F1",
        "F3",
        "F4",
        "F5",
        "F7",
        "F8",
        "F9",
        "F11",
        "F12",
        "NumPadOne",
        "NumPadTwo",
        "NumPadThree",
        "NumPadFour",
        "NumPadFive",
        "NumPadSix",
        "NumPadSeven",
        "NumPadEight",
        "NumPadNine",
        "NumPadZero",
        "XboxTypeS_LeftThumbStick",
        "XboxTypeS_RightThumbStick",
        "XboxTypeS_DPad_Up",
        "XboxTypeS_DPad_Left",
        "XboxTypeS_DPad_Right",
        "XboxTypeS_DPad_Down",
        "XboxTypeS_LeftX",
        "XboxTypeS_LeftY",
        "XboxTypeS_RightX",
        "XboxTypeS_RightY",
        "XboxTypeS_X",
        "XboxTypeS_Y",
        "XboxTypeS_A",
        "XboxTypeS_A",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z"
    };
    ImGui::GetIO().WantCaptureMouse = true;
    ImGui::GetIO().WantCaptureKeyboard = true;
    ImGui::TextUnformatted("A Plugin for BM made to manage and display the currently playing song now on every support! (Beta version) Made by Maxime Madani, Thomas Dufeu & Enomis");
    CVarWrapper enableCvar = cvarManager->getCvar("stool_enabled");
    bool enabled = false;
    if (enableCvar) {
        enabled = enableCvar.getBoolValue();
    }
    if (ImGui::Checkbox("Enable plugin", &enabled)) {
        enableCvar.setValue(enabled);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box to enable the MusicManager plugin");
    }
    if (!enabled) {
        return;
    }

    if (ImGui::Button("Sync Music")) {
        SyncSMTC();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Synchronize your music activity through SMTC");
    }
    ImGui::Checkbox("Drag Mode", &moveOverlay);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box to drag around the widget");
    }
    ImGui::Checkbox("Force snapping to right", &keepRight);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box to force the widget to snap to right");
    }
    if (moveOverlay) {
        ImGui::Checkbox("Snapping mode", &snappingMode);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Check this box to enable the snapping positioning");
        }
        if (snappingMode) {
            ImGui::SliderInt("Snapping Grid Size X", &snapping_grid_size_x, 0, screenSizeX);
            ImGui::SliderInt("Snapping Grid Size Y", &snapping_grid_size_y, 0, screenSizeY);
        }
    }
    if (ImGui::Button("Change the color of the text")) {
        show_color_picker = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click here to change the color of the text of the widget");
    }
    CVarWrapper next_index = cvarManager->getCvar(NEXT_KEYBIND_INDEX);
    CVarWrapper previous_index = cvarManager->getCvar(PREVIOUS_KEYBIND_INDEX);
    CVarWrapper pause_index = cvarManager->getCvar(PAUSE_KEYBIND_INDEX);
    if (next_index) {
        next_keybind_index = next_index.getIntValue();
        cvarManager->removeBind(keybinds[next_keybind_index]);
        ImGui::Combo("Jump to next song", &next_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
        cvarManager->setBind(keybinds[next_keybind_index], "Skip_song");
        CVarWrapper skipEnableCVar = cvarManager->getCvar(NEXT_HOTKEY);
        if (skipEnableCVar) {
            skipEnableCVar.setValue(keybinds[next_keybind_index]);
        }
        next_index.setValue(next_keybind_index);
    }
    if (previous_index) {
        previous_keybind_index = previous_index.getIntValue();
        cvarManager->removeBind(keybinds[previous_keybind_index]);
        ImGui::Combo("Jump to previous song", &previous_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
        cvarManager->setBind(keybinds[previous_keybind_index], "Prev_song");
        CVarWrapper previousEnableCVar = cvarManager->getCvar(PREVIOUS_HOTKEY);
        if (previousEnableCVar) {
            previousEnableCVar.setValue(keybinds[previous_keybind_index]);
        }
        previous_index.setValue(previous_keybind_index);
    }
    if (pause_index) {
        pause_keybind_index = pause_index.getIntValue();
        cvarManager->removeBind(keybinds[pause_keybind_index]);
        ImGui::Combo("Pause/Resume the song", &pause_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
        cvarManager->setBind(keybinds[pause_keybind_index], "Pause_song");
        CVarWrapper pauseEnableCVar = cvarManager->getCvar(PAUSE_HOTKEY);
        if (pauseEnableCVar) {
            pauseEnableCVar.setValue(keybinds[pause_keybind_index]);
        }
        pause_index.setValue(pause_keybind_index);
    }
}

void SpotifyTool::ShowColorPicker() {
    CVarWrapper color_r = cvarManager->getCvar(COLOR_TEXT_R);
    CVarWrapper color_g = cvarManager->getCvar(COLOR_TEXT_G);
    CVarWrapper color_b = cvarManager->getCvar(COLOR_TEXT_B);
    CVarWrapper color_a = cvarManager->getCvar(COLOR_TEXT_A);
    if (color_r) {
        colorText_r = color_r.getFloatValue();
    }
    else {
        colorText_r = 1.0f;
    }
    if (color_g) {
        colorText_g = color_g.getFloatValue();
    }
    else {
        colorText_g = 0.0f;
    }
    if (color_b) {
        colorText_b = color_b.getFloatValue();
    }
    else {
        colorText_b = 0.0f;
    }
    if (color_a) {
        colorText_a = color_a.getFloatValue();
    }
    else {
        colorText_a = 1.0f;
    }
    colorText = ImVec4(colorText_r, colorText_g, colorText_b, colorText_a);
    ImGui::Begin("Color Picker", &show_color_picker);
    ImGui::ColorEdit4("Color", (float*)&colorText);
    if (color_r) {
        color_r.setValue(colorText.x);
    }
    if (color_g) {
        color_g.setValue(colorText.y);
    }
    if (color_b) {
        color_b.setValue(colorText.z);
    }
    if (color_a) {
        color_a.setValue(colorText.w);
    }
    ImGui::End();
}

void SpotifyTool::SetImGuiContext(uintptr_t ctx) {
    DebugLog(LOG_FILE_PATH, "INFO: MusicManager::SetImGuiContext(uintptr_t ctx) -> Trying to load the font through ImGUI...");
    ImGui::SetCurrentContext(reinterpret_cast <ImGuiContext*> (ctx));
    auto gui = gameWrapper->GetGUIManager();

    auto [res, font] = gui.LoadFont("SpotifyFont", "font.ttf", 40);

    if (res == 0) {
        DebugLog(LOG_FILE_PATH, "ERROR: MusicManager::SetImGuiContext(uintptr_t ctx) -> Failed to load the font!");
    }
    else if (res == 1) {
        DebugLog(LOG_FILE_PATH, "INFO: MusicManager::SetImGuiContext(uintptr_t ctx) -> The font was found and will be loaded.");
    }
    else if (res == 2 && font) {
        DebugLog(LOG_FILE_PATH, "INFO: MusicManager::SetImGuiContext(uintptr_t ctx) -> The font is now selected and set as active font.");
        myFont = font;
    }
}

void SpotifyTool::Render() {
    if (!myFont) {
        auto gui = gameWrapper->GetGUIManager();
        myFont = gui.GetFont("SpotifyFont");
    }
    CVarWrapper enableCvar = cvarManager->getCvar("stool_enabled");
    if (!enableCvar) {
        if (myFont) {
            ImGui::PopFont();
        }
        return;
    }
    bool enabled = enableCvar.getBoolValue();
    if (enabled) {
        if (myFont) {
            ImGui::PushFont(myFont);
        }
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs;
        if (!ImGui::Begin(GetMenuTitle().c_str(), &isWindowOpen_, WindowFlags)) {
            ImGui::End();
            return;
        }
        if (myFont) {
            if (isThumbnail) {
                ImGui::Image(data, {
                    80,
                    80
                    });
            }
            ImGui::SameLine();
            ImGui::BeginGroup();
            CVarWrapper color_r = cvarManager->getCvar(COLOR_TEXT_R);
            CVarWrapper color_g = cvarManager->getCvar(COLOR_TEXT_G);
            CVarWrapper color_b = cvarManager->getCvar(COLOR_TEXT_B);
            CVarWrapper color_a = cvarManager->getCvar(COLOR_TEXT_A);
            if (color_r) {
                colorText_r = color_r.getFloatValue();
            }
            else {
                colorText_r = 1.0f;
            }
            if (color_g) {
                colorText_g = color_g.getFloatValue();
            }
            else {
                colorText_g = 0.0f;
            }
            if (color_b) {
                colorText_b = color_b.getFloatValue();
            }
            else {
                colorText_b = 0.0f;
            }
            if (color_a) {
                colorText_a = color_a.getFloatValue();
            }
            else {
                colorText_a = 1.0f;
            }
            colorText = ImVec4(colorText_r, colorText_g, colorText_b, colorText_a);
            ImGui::TextColored(colorText, ("%s", title.c_str()));
            ImGui::TextColored(colorText, ("%s", artist.c_str()));
        }
        else {
            ImGui::Text("The custom font haven't been loaded yet");
        }
        ImGuiWindow* window = ImGui::FindWindowByName(GetMenuTitle().c_str());
        if (moveOverlay) {
            ImGui::GetIO().WantCaptureMouse = true;
            DragWidget(window);
        }
        if (window->Pos.x > screenSizeX - window->Size.x || keepRight) {
            if (window->Pos.x != screenSizeX - window->Size.x) {
                window->Pos.x = screenSizeX - window->Size.x;
            }
        }
        if (window->Pos.y > screenSizeY - window->Size.y) {
            window->Pos.y = screenSizeY - window->Size.y;
        }
    }
    else {
        return;
    }

    if (myFont) {
        ImGui::PopFont();
    }

    ImGui::End();
    if (show_color_picker) {
        ShowColorPicker();
    }
}

void SpotifyTool::DragWidget(ImGuiWindow* window) {
    if (ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered()) {
        return;
    }
    ImGui::SetMouseCursor(2);
    if (ImGui::IsMouseDown(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (snappingMode) {
            if ((mousePos.x - (mousePos.x / snapping_grid_size_x) * snapping_grid_size_x == 0) && (mousePos.x >= 0 && mousePos.x <= screenSizeX - window->Size.x)) {
                window->Pos.x = snapping_grid_size_x * floor(mousePos.x / snapping_grid_size_x);
            }
            if (snapping_grid_size_x * ceil(mousePos.x / snapping_grid_size_x) > screenSizeX - window->Size.x) {
                window->Pos.x = screenSizeX - window->Size.x;
            }
            if (mousePos.y - (mousePos.y / snapping_grid_size_y) * snapping_grid_size_y == 0 && (mousePos.y >= 0 && mousePos.y <= screenSizeY - window->Size.y)) {
                window->Pos.y = snapping_grid_size_y * floor(mousePos.y / snapping_grid_size_y);
            }
            if (snapping_grid_size_y * ceil(mousePos.y / snapping_grid_size_y) > screenSizeY - window->Size.y) {
                window->Pos.y = screenSizeY - window->Size.y;
            }
        }
        else {
            window->Pos.x = clamp(mousePos.x, (float)0.0, screenSizeX - window->Size.x);
            window->Pos.y = clamp(mousePos.y, (float)0.0, screenSizeY - window->Size.y);
        }
    }
}

string SpotifyTool::GetMenuName() {
    return "MusicManager";
}
string SpotifyTool::GetMenuTitle() {
    return "Music Manager";
}
bool SpotifyTool::ShouldBlockInput() {
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}
bool SpotifyTool::IsActiveOverlay() {
    return false;
}
void SpotifyTool::OnOpen() {
    isWindowOpen_ = true;
}
void SpotifyTool::OnClose() {
    isWindowOpen_ = false;
    moveOverlay = false;
    ImGui::GetIO().WantCaptureMouse = false;
    ImGui::GetIO().WantCaptureKeyboard = false;
}

#pragma region MusicManager
bool SpotifyTool::InitializeWinRT() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::InitializeWinRT() -> Trying to init_apartment...");
    try {
        winrt::init_apartment();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::InitializeWinRT() -> OK!");
        return true;
    }
    catch (winrt::hresult_error const& ex) {
        wstring error_wstr(ex.message().c_str());
        string error(error_wstr.begin(), error_wstr.end());
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::InitializeWinRT() -> " + error);
        return false;
    }
}

void SpotifyTool::SyncSMTC() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SyncSMTC() -> Checking if WinRT is enabled...");
    if (!isWinRTInitialized) {
        return;
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SyncSMTC() -> OK!");
    GlobalSystemMediaTransportControlsSession currentSession = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();
    GlobalSystemMediaTransportControlsSessionMediaProperties properties = currentSession.TryGetMediaPropertiesAsync().get();

    wstring trackName_wstr(properties.Title().c_str());
    string trackName(trackName_wstr.begin(), trackName_wstr.end());
    title = trackName;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SyncSMTC() -> Title: " + title);

    wstring artistName_wstr(properties.Artist().c_str());
    string artistName(artistName_wstr.begin(), artistName_wstr.end());
    artist = artistName;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SyncSMTC() -> Artist: " + artist);

    IRandomAccessStreamReference thumbnail = properties.Thumbnail();
    if (thumbnail) {
        isThumbnail = true;
        IRandomAccessStreamWithContentType data_stream = thumbnail.OpenReadAsync().get();
        IBuffer buffer = Buffer(data_stream.Size());
        buffer = data_stream.ReadAsync(buffer, data_stream.Size(), InputStreamOptions::None).get();
        //data = buffer.data();
    }
    else {
        isThumbnail = false;
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SyncSMTC() -> IsThumbnail: " + to_string(isThumbnail));
}

void SpotifyTool::SkipSongSMTC() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SkipSongSMTC() -> Checking if WinRT is available...");
    if (!isWinRTInitialized) {
        return;
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SkipSongSMTC() -> Trying to skip the song...");
    GlobalSystemMediaTransportControlsSession currentSession = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();
    if (!currentSession.TrySkipNextAsync().get()) {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::SkipSongSMTC() -> Cannot skip to next song!");
    }
    else {
        SyncSMTC();
    }
}

void SpotifyTool::PreviousSongSMTC() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::PreviousSongSMTC() -> Checking if WinRT is available...");
    if (!isWinRTInitialized) {
        return;
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::PreviousSongSMTC() -> Trying to go to the previous song...");
    GlobalSystemMediaTransportControlsSession currentSession = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();
    if (!currentSession.TrySkipPreviousAsync().get()) {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::PreviousSongSMTC() -> Cannot go to the previous song!");
    }
    else {
        SyncSMTC();
    }
}

void SpotifyTool::TogglePausePlaySongSMTC() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::TogglePausePlaySongSMTC() -> Checking if WinRT is available...");
    if (!isWinRTInitialized) {
        return;
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::TogglePausePlaySongSMTC() -> Trying to toggle the music...");
    GlobalSystemMediaTransportControlsSession currentSession = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetCurrentSession();
    if (!currentSession.TryTogglePlayPauseAsync().get()) {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::TogglePausePlaySongSMTC() -> Cannot toggle play/pause!");
    }
    else {
        SyncSMTC();
    }
}