#include "pch.h"

#include "SpotifyTool.h"

#include <fstream>

#include <iostream>

#include <ctime>

#include "nlohmann/json.hpp"

#include <string>

#include <regex>

#include "bakkesmod/wrappers/GuiManagerWrapper.h"

#include "IMGUI/imgui_internal.h"

/*
TO DO LIST:
 Take C++ courses maybe ;)
*/

BAKKESMOD_PLUGIN(SpotifyTool, "Spotify Tool Plugin", "0.1.1", PERMISSION_ALL)
using namespace std;
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
static string JSON_DATA = "";

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
    string temp_oauth_file = ROOT_DIRECTORY + "oauth_code.txt";
    LOG_FILE_PATH = ROOT_DIRECTORY + "stool_log.log";
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Setting up the config and the directories...");
    LOGO_FILE_PATH = ROOT_DIRECTORY + "stool_logo.png";
    if (come_from_deInit) {
        JSON_DATA = buffer_json_data;
        come_from_deInit = false;
    }
    else {
        JSON_DATA = R"({"access_token":"","artist":"","base64":"ZmI2YzkzZTk0NjNlNDEwM2E0YTA2YWRmNGQzNzM3ODY6NzcwMDg0NTAzOTg0NDdiNWE0ZjY1Yzg1NDI0YzZhMjU=","code":"","duration":0,"picture":"","progress":0,"refresh_token":"","setup_statut":false,"song":"","searched":""})";
    }
    json data = json::parse(JSON_DATA);
    ifstream f(ROOT_DIRECTORY + "oauth_code.txt");
    if (f.is_open()) {
        string code((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        f.close();
        data["code"] = code;
        remove(temp_oauth_file.c_str());
    }
    JSON_DATA = data.dump();
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Updating game resolution...");
    screenSizeX = gameWrapper->GetScreenSize().X;
    screenSizeY = gameWrapper->GetScreenSize().Y;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering togglemenu command...");
    gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
        cvarManager->executeCommand("togglemenu " + GetMenuName());
        }, 1);
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering Spotify synchronization notifier...");
    cvarManager->registerNotifier("Sync_spotify", [this](vector < string > args) {
        Sync_spotify();
        }, "", PERMISSION_ALL);
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Setting up Spotify access through the Setup_spotify() function...");
    Setup_spotify();
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Refreshing the token access through the Refresh_token() function...");
    Refresh_token();
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the plugin enabled variable...");
    cvarManager->registerCvar("stool_enabled", "1", "Enable Spotify Tool", true, true, 0, true, 1)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        stoolEnabled = cvar.getBoolValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the keybind index for the Next song hotkey...");
    cvarManager->registerCvar(NEXT_KEYBIND_INDEX, "0", "Index for the next hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        next_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the keybind index for the Previous song hotkey...");
    cvarManager->registerCvar(PREVIOUS_KEYBIND_INDEX, "0", "Index for the previous hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        previous_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the keybind index for the Pause song hotkey...");
    cvarManager->registerCvar(PAUSE_KEYBIND_INDEX, "0", "Index for the pause hotkey in the keybind list", true, true, 0, true, 69, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        pause_keybind_index = cvar.getIntValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the R-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_R, "1.0f", "R-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_r = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the G-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_G, "0.0f", "G-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_g = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the B-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_B, "0.0f", "B-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_b = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the A-channel of the color for the text inside the widget...");
    cvarManager->registerCvar(COLOR_TEXT_A, "1.0f", "A-channel value of the color for the text inside the widget", true, true, 0.0f, true, 1.0f, true)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        colorText_a = cvar.getFloatValue();
            });
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Updating the color vector...");
    colorText = ImVec4(colorText_r, colorText_g, colorText_b, colorText_a);
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the hotkey to skip the current song...");
    cvarManager->registerCvar(NEXT_HOTKEY, NEXT_KEYBIND, "Next song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering the command notifier to skip the current song...");
    cvarManager->registerNotifier(
        "Skip_song",
        [this](vector < string > args) {
            Skip_song();
        },
        "Skip the current song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the hotkey to return to the previous song...");
    cvarManager->registerCvar(PREVIOUS_HOTKEY, PREVIOUS_KEYBIND, "Previous song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering the command notifier to return to the previous song...");
    cvarManager->registerNotifier(
        "Prev_song",
        [this](vector < string > args) {
            Prev_song();
        },
        "Jump to the previous song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering/Fetching the hotkey to pause the current song...");
    cvarManager->registerCvar(PAUSE_HOTKEY, PAUSE_KEYBIND, "Pause/Resume song Hotkey", false)
        .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {});
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Registering the command notifier to pause the current song...");
    cvarManager->registerNotifier(
        "Pause_song",
        [this](vector < string > args) {
            Pause_song();
        },
        "Pause/Resume the current song",
            PERMISSION_ALL
            );
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Loading the logo to display as a texture...");
    gameWrapper->LoadToastTexture("stool_logo", LOGO_FILE_PATH);
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Loading the toast to display...");
    gameWrapper->Toast("SpotifyTool", "SpotifyTool loaded!", "stool_logo", 5.0, ToastType_Warning);
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onLoad() -> Finished initializing.");
}

void SpotifyTool::onUnload() {
    come_from_deInit = true;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Unloading the saved keybinds...");
    CVarWrapper nextHotkeyCVar = cvarManager->getCvar(NEXT_HOTKEY);
    CVarWrapper previousHotkeyCVar = cvarManager->getCvar(PREVIOUS_HOTKEY);
    CVarWrapper pauseHotkeyCVar = cvarManager->getCvar(PAUSE_HOTKEY);
    if (nextHotkeyCVar) {
        string bind_next = nextHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_next);
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Successfully removed the skip the current song hotkey bind!");
    }
    if (previousHotkeyCVar) {
        string bind_previous = previousHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_previous);
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Successfully removed the go to the previous song hotkey bind!");
    }
    if (pauseHotkeyCVar) {
        string bind_pause = pauseHotkeyCVar.getStringValue();
        cvarManager->removeBind(bind_pause);
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Successfully removed the pause the current song hotkey bind!");
    }
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Clearing the song's datas...");
    json data = json::parse(JSON_DATA);
    data["song"] = "No Song";
    JSON_DATA = data.dump();
    buffer_json_data = JSON_DATA;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::onUnload() -> Unloading completed.");
}

string SpotifyTool::GetPluginName() {
    return "SpotifyTool Beta";
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
    ImGui::TextUnformatted("A Plugin for BM made to manage and display the currently playing song on Spotify (Beta version). Huge thanks to the BakkesMod Programming Discord for carrying me to this <3");
    CVarWrapper enableCvar = cvarManager->getCvar("stool_enabled");
    bool enabled = false;
    if (enableCvar) {
        enabled = enableCvar.getBoolValue();
    }
    if (ImGui::Checkbox("Enable plugin", &enabled)) {
        enableCvar.setValue(enabled);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box to enable the SpotifyTool plugin");
    }
    if (!enabled) {
        return;
    }

    CVarWrapper search_song = cvarManager->getCvar("stool_ssong");
    static char query_song[128] = "";
    ImGui::InputText("", query_song, IM_ARRAYSIZE(query_song));
    ImGui::SameLine();
    if (ImGui::Button("Search")) {
        ImGui::SetTooltip("Search a song here");
        int amount = 1;
        Search_spotify(query_song, amount);
        Queue_song();
        if (skiptosong) {
            gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
                Skip_song();
                }, 1);
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Sync Spotify")) {
        Sync_spotify();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Synchronize your Spotify activity");
    }
    ImGui::Checkbox("Play the search song", &skiptosong);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box to skip to the next song in your searched playlist");
    }
    ImGui::Checkbox("Using non-premium Spotify", &stool_free);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Check this box if you have Spotify Free with the 40-seconds ad break");
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
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SetImGuiContext(uintptr_t ctx) -> Trying to load the font through ImGUI...");
    ImGui::SetCurrentContext(reinterpret_cast <ImGuiContext*> (ctx));
    auto gui = gameWrapper->GetGUIManager();

    auto [res, font] = gui.LoadFont("SpotifyToolFont", "font.ttf", 40);

    if (res == 0) {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::SetImGuiContext(uintptr_t ctx) -> Failed to load the font!");
    }
    else if (res == 1) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SetImGuiContext(uintptr_t ctx) -> The font was found and will be loaded.");
    }
    else if (res == 2 && font) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::SetImGuiContext(uintptr_t ctx) -> The font is now selected and set as active font.");
        myFont = font;
    }
}

void SpotifyTool::Render() {
    if (!myFont) {
        auto gui = gameWrapper->GetGUIManager();
        myFont = gui.GetFont("SpotifyToolFont");
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
        json data = json::parse(JSON_DATA);
        if (!ImGui::Begin(GetMenuTitle().c_str(), &isWindowOpen_, WindowFlags)) {
            ImGui::End();
            return;
        }
        if (myFont) {
            if (cover) {
                if (auto* ptr = cover->GetImguiPtr()) {
                    ImGui::Image(ptr, {
                        80,
                        80
                        });
                }
                else {
                    ImGui::Text("Loading");
                }
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
            ImGui::TextColored(colorText, ("%s", data.value("song", "").c_str()));
            ImGui::TextColored(colorText, ("%s", data.value("artist", "").c_str()));
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
        if (doOnce) {
            duration_ms = data.value("duration", 0);
            progress_ms = data.value("progress", 0);
            song_duration = ((duration_ms - progress_ms) / 1000.0f) + 3.0f;
            doOnce = false;
            counter = 0;
        }
        if (!paused) {
            counter += ImGui::GetIO().DeltaTime;
        }
        token_denied += ImGui::GetIO().DeltaTime;
        if (skipped) {
            skip_delay += ImGui::GetIO().DeltaTime;
        }
        if (token_denied > 3500) {
            Refresh_token();
            token_denied = 0;
        }
        if (counter > song_duration) {
            Sync_spotify();
            if (stool_free) {
                gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
                    cvarManager->executeCommand("Sync_spotify");
                    }, 40);
            }
            song = data.value("song", "");
            doOnce = true;
            counter = 0;
            skipped = false;
        }
        if (skip_delay > 1) {
            Sync_spotify();
            skipped = false;
            skip_delay = 0;
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
    return "SpotifyTool";
}
string SpotifyTool::GetMenuTitle() {
    return "Spotify Tool";
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

#pragma region Spotify
void SpotifyTool::Setup_spotify() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> Fetching the code and setup status from the config...");
    json data = json::parse(JSON_DATA);
    code_spotify = data.value("code", "");
    setup_statut = data.value("setup_statut", false);
    string auth_setup = "Basic " + data.value("base64", "");
    if (setup_statut == false) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> Preparing the CURL request to the Spotify servers...");
        CurlRequest req;
        req.url = "https://accounts.spotify.com/api/token";
        req.verb = "POST";
        req.headers = {
            {
                "Authorization", auth_setup
            },
            {
                "Content-Type", "application/x-www-form-urlencoded"
            }
        };
        req.body = "redirect_uri=http%3A%2F%2Flocalhost%3A8888%2Fauth%2Fspotify%2Fcallback&grant_type=authorization_code&code=" + code_spotify;
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> Sending CURL request, expecting 200 as a response...");
        HttpWrapper::SendCurlRequest(req, [this](int response_code, string result) {
            DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as a response code.");
        if (response_code == 200) {
            DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify::SendCurlRequest -> Code 200 recieved, fetching the refresh and access token to validate the session...");
            json token_complete = json::parse(result.begin(), result.end());
            json data = json::parse(JSON_DATA);
            refresh_token = token_complete["refresh_token"];
            access_token = token_complete["access_token"];
            data["access_token"] = access_token;
            data["refresh_token"] = refresh_token;
            JSON_DATA = data.dump();
            DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify::SendCurlRequest -> Fetching was successfull, new tokens have been saved!");
        }
        else {
            DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Setup_spotify::SendCurlRequest -> The setup encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result + " as a result.");
            LOG("ERROR: SpotifyTool::Setup_spotify::SendCurlRequest -> The setup encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result);
        }
            });
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> Updating the setup status in the config...");
        json data = json::parse(JSON_DATA);
        setup_statut = true;
        data["setup_statut"] = setup_statut;
        JSON_DATA = data.dump();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> Update complete, the setup was successfull!");
    }
    else {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Setup_spotify() -> The setup was alredy done, skipping...");
    }
}

void SpotifyTool::Sync_spotify() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify() -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify() -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_playing;
    req_playing.url = "https://api.spotify.com/v1/me/player/currently-playing";
    req_playing.verb = "GET";
    req_playing.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify() -> Sending CURL request, expecting 200 as a response...");
    HttpWrapper::SendCurlRequest(req_playing, [this](int response_code, string result_playing) {

        currently_playing = result_playing;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 200) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> Code 200 successfully recieved, parsing the datas from the Spotify servers...");
        json data = json::parse(JSON_DATA);
        json playing_json = json::parse(currently_playing);
        song = playing_json["item"]["name"];
        artist = playing_json["item"]["artists"][0]["name"];
        picture = playing_json["item"]["album"]["images"][0]["url"];
        duration = playing_json["item"]["duration_ms"];
        progress = playing_json["progress_ms"];
        regex rx("(\\d)(?=(\\d{3})+$)");
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> Successfully retrieved song's data:\n\tSong: " + song + "\n\tArtist: " + artist + "\n\tURL of the album cover: " + picture + "\n\tDuration (in ms): " + regex_replace(to_string(duration), rx, "$1 ") + "\n\tCurrent song progress (in ms): " + regex_replace(to_string(progress), rx, "$1 "));
        cover = make_shared < ImageLinkWrapper >(picture, gameWrapper);
        if (cover) {
            if (auto* ptr = cover->GetImguiPtr()) {
                DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> Image was successfully loaded in!");
                ImGui::Image(ptr, {
                    80,
                    80
                    });
            }
            else {
                DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> Loading the image from the URL...");
            }
        }
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Sync_spotify::SendCurlRequest -> Writing datas in the config to save them...");
        data["song"] = song;
        data["artist"] = artist;
        data["picture"] = picture;
        data["duration"] = duration;
        data["progress"] = progress;
        JSON_DATA = data.dump();
        doOnce = true;
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Sync_spotify::SendCurlRequest -> The synchronization encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_playing + " as a result.");
        LOG("ERROR: SpotifyTool::Sync_spotify::SendCurlRequest -> The synchronization encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_playing);
    }
        });
}

void SpotifyTool::Refresh_token() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token() -> Fetching the refresh token from the config...");
    json data = json::parse(JSON_DATA);
    refresh_token = data.value("refresh_token", "");
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token() -> Successfully fetched the refresh token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_refresh;
    req_refresh.url = "https://accounts.spotify.com/api/token";
    req_refresh.verb = "POST";
    string auth_setup = "Basic " + data.value("base64", "");
    req_refresh.headers = {
        {
            "Authorization", auth_setup
        },
        {
            "Content-Type", "application/x-www-form-urlencoded"
        }
    };
    req_refresh.body = "grant_type=refresh_token&refresh_token=" + refresh_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token() -> Sending CURL request, expecting 200 as a response...");
    HttpWrapper::SendCurlRequest(req_refresh, [this](int response_code, string result) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 200) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token::SendCurlRequest -> Code 200 successfully recieved, parsing the token data from the Spotify servers...");
        json data = json::parse(JSON_DATA);
        json token_complete = json::parse(result.begin(), result.end());
        access_token = token_complete["access_token"];
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token::SendCurlRequest -> Successfully parsed the data from the Spotify servers, saving the new token in the config...");
        data["access_token"] = access_token;
        JSON_DATA = data.dump();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Refresh_token::SendCurlRequest -> Successfully saved the config, the token is refreshed.");
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Refresh_token::SendCurlRequest -> The token refresh encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result + " as a result.");
        LOG("ERROR: SpotifyTool::Refresh_token::SendCurlRequest -> The token refresh encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result);
    }
        });
}

void SpotifyTool::Skip_song() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song() -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song() -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_skip;
    req_skip.url = "https://api.spotify.com/v1/me/player/next";
    req_skip.verb = "POST";
    req_skip.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Length", "0"
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song() -> Sending CURL request, expecting 204 as a response...");
    HttpWrapper::SendCurlRequest(req_skip, [&](int response_code, string result_skip) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 204) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song::SendCurlRequest -> Code 204 successfully recieved, song has been skipped successfully!");
        counter = 0;
        Sync_spotify();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Skip_song::SendCurlRequest -> The new song is now synced.");
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Skip_song::SendCurlRequest -> The skip song procedure encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_skip + " as a result.");
        LOG("ERROR: SpotifyTool::Skip_song::SendCurlRequest -> The skip song procedure encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_skip);
    }
        });
}

void SpotifyTool::Prev_song() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song() -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song() -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_prev;
    req_prev.url = "https://api.spotify.com/v1/me/player/previous";
    req_prev.verb = "POST";
    req_prev.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Length", "0"
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song() -> Sending CURL request, expecting 204 as a response...");
    HttpWrapper::SendCurlRequest(req_prev, [this](int response_code, string result_prev) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 204) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song::SendCurlRequest -> Code 204 successfully recieved, previous song has been loaded successfully!");
        counter = 0;
        Sync_spotify();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Prev_song::SendCurlRequest -> The new song is now synced.");
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Prev_song::SendCurlRequest -> The go to previous song procedure encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_prev + " as a result.");
        LOG("ERROR: SpotifyTool::Prev_song::SendCurlRequest -> The go to previous song procedure encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_prev);
    }
        });
}

void SpotifyTool::Pause_song() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song() -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song() -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_pause;
    if (paused) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song() -> The media is paused, a resume command will be send through CURL.");
        req_pause.url = "https://api.spotify.com/v1/me/player/play";
        paused = false;
    }
    else {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song() -> The media is playing, a pause command will be send through CURL.");
        req_pause.url = "https://api.spotify.com/v1/me/player/pause";
        paused = true;
    }
    req_pause.verb = "PUT";
    req_pause.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Length", "0"
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song() -> Sending CURL request, expecting 204 as a response...");
    HttpWrapper::SendCurlRequest(req_pause, [this](int response_code, string result_pause) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 204) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song::SendCurlRequest -> Code 204 successfully recieved, executing the control command...");
        if (paused) {
            DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song::SendCurlRequest -> Successfully paused the song.");
        }
        else {
            DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Pause_song::SendCurlRequest -> Successfully resumed the song.");
        }
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Pause_song::SendCurlRequest -> The pause song procedure encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_pause + " as a result.");
        LOG("ERROR: SpotifyTool::Pause_song::SendCurlRequest -> The pause song procedure encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_pause);
    }
        });
}

void SpotifyTool::Search_spotify(string query, int amount) {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify(string query = " + query + ", int amount = " + to_string(amount) + ") -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify(string query = " + query + ", int amount = " + to_string(amount) + ") -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_search;
    if (search_type) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify(string query = " + query + ", int amount = " + to_string(amount) + ") -> The search type will fetch tracks for the query.");
        req_search.url = "https://api.spotify.com/v1/search?q=" + query + "&type=track&limit=" + to_string(amount);
    }
    else {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify(string query = " + query + ", int amount = " + to_string(amount) + ") -> The search type will fetch playlists for the query.");
        req_search.url = "https://api.spotify.com/v1/search?q=" + query + "&type=playlist&limit=" + to_string(amount);
    }
    req_search.verb = "GET";
    req_search.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify(string query = " + query + ", int amount = " + to_string(amount) + ") -> Sending CURL request, expecting 200 as a response...");
    HttpWrapper::SendCurlRequest(req_search, [this, amount](int response_code, string result_search) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 200) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify::SendCurlRequest -> Code 200 successfully recieved, parsing the result's datas from the Spotify servers...");
        json data = json::parse(JSON_DATA);
        json complete_data = json::parse(result_search.begin(), result_search.end());
        string trackId = complete_data["tracks"]["items"][0]["uri"];
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify::SendCurlRequest -> Successfully parsed the datas from the Spotify servers, saving the search ID in the config...");
        data["searched"] = trackId;
        JSON_DATA = data.dump();
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Search_spotify::SendCurlRequest -> Successfully saved the config, the search ID has been memorized.");
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Search_spotify::SendCurlRequest -> The search song procedure encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_search + " as a result.");
        LOG("ERROR: SpotifyTool::Search_spotify::SendCurlRequest -> The search song procedure encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_search);
    }
        });
}
void SpotifyTool::Queue_song() {
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Queue_song() -> Fetching the access token from the config...");
    json data = json::parse(JSON_DATA);
    access_token = data.value("access_token", "");
    string track_uri = data.value("searched", "");
    auth_bearer = "Bearer " + access_token;
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Queue_song() -> Successfully fetched the access token, preparing the CURL request to the Spotify servers...");
    CurlRequest req_queue;
    req_queue.url = "https://api.spotify.com/v1/me/player/queue?uri=" + track_uri;
    req_queue.verb = "POST";
    req_queue.headers = {
        {
            "Authorization", auth_bearer
        },
        {
            "Content-Length", "0"
        },
        {
            "Content-Type", "application/json"
        }
    };
    DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Queue_song() -> Sending CURL request, expecting 200 or 204 as a response...");
    HttpWrapper::SendCurlRequest(req_queue, [this](int response_code, string result_queue) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Queue_song::SendCurlRequest -> CURL request gave " + to_string(response_code) + " as response code.");
    if (response_code == 200 or response_code == 204) {
        DebugLog(LOG_FILE_PATH, "INFO: SpotifyTool::Queue_song::SendCurlRequest -> Code " + to_string(response_code) + " successfully recieved, song is now queued!");
    }
    else {
        DebugLog(LOG_FILE_PATH, "ERROR: SpotifyTool::Queue_song::SendCurlRequest -> The queuing song procedure encountered a problem! Error code " + to_string(response_code) + ", CURL request returned " + result_queue + " as a result.");
        LOG("ERROR: SpotifyTool::Queue_song::SendCurlRequest -> The queuing song procedure encountered a problem! Error code {}, CURL request returned {} as a result.", to_string(response_code), result_queue);
    }
        });
}