#include "pch.h"
#include "SpotifyTool.h"
#include <fstream>
#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include <string>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"
#include "IMGUI/imgui_internal.h"

#include "..\SMTCInterop\SMTCInterop.h"

/*
TO DO LIST:
 Fix CURL (DONE)
 Parse access token (DONE)
 Parse refresh token (DONE)
 Get song (DONE)
 Get picture (DONE)
 Picture in options
 Skip/Pause (DONE)
 Keybinds (DONE)
 LEARN TO FCKG CODE IN C++
*/

BAKKESMOD_PLUGIN(SpotifyTool, "Spotify Tool Plugin", "0.1.1", PERMISSION_ALL)
using namespace std;
using json = nlohmann::json;
shared_ptr<CVarManagerWrapper> _globalCvarManager;

std::string NEXT_KEYBIND = "S";
std::string PREVIOUS_KEYBIND = "P";
std::string PAUSE_KEYBIND = "A";

const std::string NEXT_HOTKEY = "next_hotkey";
const std::string PREVIOUS_HOTKEY = "previous_hotkey";
const std::string PAUSE_HOTKEY = "pause_hotkey";

void DebugLog(std::string info) {
	// Open the file in append mode
	fstream file("SpotifyTool_log.txt", ios::app);

	// Check if the file was opened successfully
	if (!file.is_open())
	{
		cout << "Error opening file!" << endl;
	}
	// Write the text
	string text = "\n" + info;
	file.write(text.c_str(), text.length());

	// Close the file
	file.close();
}

void SpotifyTool::onLoad()
{
	_globalCvarManager = cvarManager;
	gameWrapper->SetTimeout([this](GameWrapper* gameWrapper) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
		}, 1);
	cvarManager->registerCvar("stool_scale", "1", "Overlay scale", true, true, 0, true, 10, true);
	cvarManager->registerNotifier("Sync_spotify", [this](std::vector<std::string> args) {
		Sync_spotify();
		}, "", PERMISSION_ALL);
	Setup_spotify();
	Refresh_token();
	cvarManager->registerCvar(NEXT_HOTKEY, NEXT_KEYBIND, "Next song Hotkey", false)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {});
	cvarManager->registerNotifier(
		"Skip_song",
		[this](std::vector<std::string> args) { Skip_song(); },
		"Skip the current song",
		PERMISSION_ALL
	);
	cvarManager->registerCvar(PREVIOUS_HOTKEY, PREVIOUS_KEYBIND, "Previous song Hotkey", false)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {});
	cvarManager->registerNotifier(
		"Prev_song",
		[this](std::vector<std::string> args) { Prev_song(); },
		"Jump to the previous song",
		PERMISSION_ALL
	);
	cvarManager->registerCvar(PAUSE_HOTKEY, PAUSE_KEYBIND, "Pause/Resume song Hotkey", false)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {});
	cvarManager->registerNotifier(
		"Pause_song",
		[this](std::vector<std::string> args) { Pause_song(); },
		"Pause/Resume the current song",
		PERMISSION_ALL
	);
	gameWrapper->LoadToastTexture("spotifytool_logo", gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "spotifytool_logo.png");
	gameWrapper->Toast("SpotifyTool", "SpotifyTool is loaded", "spotifytool_logo", 5.0, ToastType_Warning);
	cvarManager->registerCvar("stool_enabled", "1", "Enable Spotify Tool", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		stoolEnabled = cvar.getBoolValue();
			});
	cvarManager->registerCvar("stool_color", "#FFFFFF", "color of overlay");

	SMTCManager::test();
}

void SpotifyTool::onUnload() {

	CVarWrapper nextHotkeyCVar = cvarManager->getCvar(NEXT_HOTKEY);
	CVarWrapper previousHotkeyCVar = cvarManager->getCvar(PREVIOUS_HOTKEY);
	CVarWrapper pauseHotkeyCVar = cvarManager->getCvar(PAUSE_HOTKEY);
	if (nextHotkeyCVar) {
		std::string bind_next = nextHotkeyCVar.getStringValue();
		cvarManager->removeBind(bind_next);
	}
	if (previousHotkeyCVar) {
		std::string bind_previous = previousHotkeyCVar.getStringValue();
		cvarManager->removeBind(bind_previous);
	}
	if (pauseHotkeyCVar) {
		std::string bind_pause = pauseHotkeyCVar.getStringValue();
		cvarManager->removeBind(bind_pause);
	}
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	data["song"] = "No Spotify Activity";
	std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	file << data;
	file.close();
}

// Name of the plugin to be shown on the f2 -> plugins list
std::string SpotifyTool::GetPluginName()
{
	return "SpotifyTool Beta";
}


#pragma region Rendering
void SpotifyTool::RenderSettings() {
	const char* keybinds[] = { "None","One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Zero", "F1", "F3", "F4", "F5", "F7", "F8", "F9", "F11", "F12",
		"NumPadOne", "NumPadTwo", "NumPadThree", "NumPadFour", "NumPadFive", "NumPadSix", "NumPadSeven", "NumPadEight", "NumPadNine", "NumPadZero",
		"XboxTypeS_LeftThumbStick", "XboxTypeS_RightThumbStick", "XboxTypeS_DPad_Up", "XboxTypeS_DPad_Left", "XboxTypeS_DPad_Right",  "XboxTypeS_DPad_Down",
		"XboxTypeS_LeftX", "XboxTypeS_LeftY", "XboxTypeS_RightX", "XboxTypeS_RightY", "XboxTypeS_X", "XboxTypeS_Y", "XboxTypeS_A", "XboxTypeS_A",
		"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };
	ImGui::GetIO().WantCaptureMouse = true;
	ImGui::GetIO().WantCaptureKeyboard = true;
	static int pause_keybind_index = 0;
	static int previous_keybind_index = 0;
	static int next_keybind_index = 0;
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
		ImGui::SetTooltip("Toggle SpotifyTool Plugin");
	}
	if (!enabled) {
		return;
	}

	CVarWrapper search_song = cvarManager->getCvar("stool_ssong");
	// Search song query
	static char query_song[128] = "";
	ImGui::InputText("", query_song, IM_ARRAYSIZE(query_song));
	ImGui::SameLine();
	if (ImGui::Button("Search")) {
		ImGui::SetTooltip("Search a song here");
		int amount = 1;
		Search_spotify(query_song,amount);
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
	ImGui::Checkbox("Play the search song", &skiptosong);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Force the widget to snap to right");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Sync your activity");
	}
	ImGui::Checkbox("Using non-premium Spotify", &stool_free);

	ImGui::Checkbox("Drag Mode", &moveOverlay);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Enable drag mode for the widget");
	}
	ImGui::Checkbox("Force snapping to right", &keepRight);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Force the widget to snap to right");
	}
	if (moveOverlay) {
		ImGui::Checkbox("Snapping mode", &snappingMode);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Toggle the snapping mode");
		}
		if (snappingMode) {
			ImGui::SliderInt("Snapping Grid Size X", &snapping_grid_size_x, 0, screenSizeX);
			ImGui::SliderInt("Snapping Grid Size Y", &snapping_grid_size_y, 0, screenSizeY);
		}
	}
	ImGui::SliderInt("Text color R", &text_color_r, 0, 255);
	ImGui::SliderInt("Text color G", &text_color_g, 0, 255);
	ImGui::SliderInt("Text color B", &text_color_b, 0, 255);
	cvarManager->removeBind(keybinds[next_keybind_index]);
	cvarManager->removeBind(keybinds[previous_keybind_index]);
	cvarManager->removeBind(keybinds[pause_keybind_index]);
	ImGui::Combo("Jump to next song", &next_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
	ImGui::Combo("Jump to previous song", &previous_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
	ImGui::Combo("Pause/Resume the song", &pause_keybind_index, keybinds, IM_ARRAYSIZE(keybinds));
	cvarManager->setBind(keybinds[next_keybind_index], "Skip_song");
	cvarManager->setBind(keybinds[previous_keybind_index], "Prev_song");
	cvarManager->setBind(keybinds[pause_keybind_index], "Pause_song");
	CVarWrapper skipEnableCVar = cvarManager->getCvar(NEXT_HOTKEY);
	CVarWrapper previousEnableCVar = cvarManager->getCvar(PREVIOUS_HOTKEY);
	CVarWrapper pauseEnableCVar = cvarManager->getCvar(PAUSE_HOTKEY);
	if (skipEnableCVar) {
		skipEnableCVar.setValue(keybinds[next_keybind_index]);
	}
	if (previousEnableCVar) {
		previousEnableCVar.setValue(keybinds[previous_keybind_index]);
	}
	if (pauseEnableCVar) {
		pauseEnableCVar.setValue(keybinds[pause_keybind_index]);
	}
}

void SpotifyTool::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
	auto gui = gameWrapper->GetGUIManager();

	// Font is customisable by adding a new one and/or changing name of another
	auto [res, font] = gui.LoadFont("SpotifyToolFont", "font.ttf", 40);

	if (res == 0) {
		cvarManager->log("Failed to load the font!");
	}
	else if (res == 1) {
		cvarManager->log("The font will be loaded");
	}
	else if (res == 2 && font) {
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
		// First ensure the font is actually loaded
		if (myFont) {
			ImGui::PushFont(myFont);
		}
		ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs;

		// uncomment if you don't want a background (flag found by just checking for the other window flags available
		//WindowFlags |= ImGuiWindowFlags_NoBackground;

		std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
		json data = json::parse(f);
		f.close();
		if (!ImGui::Begin(GetMenuTitle().c_str(), &isWindowOpen_, WindowFlags))
		{
			// Early out if the window is collapsed, as an optimization.
			ImGui::End();
			return;
		}
		if (myFont) {
			if (cover)
			{
				if (auto* ptr = cover->GetImguiPtr())
				{
					ImGui::Image(ptr, { 80, 80 });
				}
				else
				{
					ImGui::Text("Loading");
				}
			}
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImVec4 color(text_color_r / 255.0, text_color_g / 255.0, text_color_b / 255.0, 1.0);
			ImGui::TextColored(color, ("%s", data.value("song", "").c_str()));
			ImGui::TextColored(color, ("%s", data.value("artist", "").c_str()));
		}
		else
		{
			ImGui::Text("The custom font haven't been loaded yet");
		}
		ImGuiWindow* window = ImGui::FindWindowByName(GetMenuTitle().c_str());
		if (moveOverlay) {
			ImGui::GetIO().WantCaptureMouse = true;
			DragWidget(window);
		}
		else {
			ImGui::GetIO().WantCaptureMouse = false;
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
			song_duration = ((duration_ms - progress_ms) / 1000) + 3;
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
		if (token_denied > 3500)
		{
			Refresh_token();
			token_denied = 0;
		}
		if (counter > song_duration)
		{
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
	else
	{
		return;
	}

	if (myFont) {
		ImGui::PopFont();
	}

	ImGui::End();
}

void SpotifyTool::DragWidget(ImGuiWindow* window) {
	if (ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered()) {
		// doesn't do anything if any ImGui is hovered over
		return;
	}
	// drag cursor w/ arrows to N, E, S, W
	ImGui::SetMouseCursor(2);
	if (ImGui::IsMouseDown(0)) {
		// if holding left click, move
		// sets location to current mouse position
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
// Do ImGui rendering here

// Name of the menu that is used to toggle the window.
string SpotifyTool::GetMenuName()
{
	return "SpotifyTool";
}
string SpotifyTool::GetMenuTitle()
{
	return "Spotify Tool";
}
// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool SpotifyTool::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}
// Return true if window should be interactive
bool SpotifyTool::IsActiveOverlay()
{
	return false;
}
// Called when window is opened
void SpotifyTool::OnOpen()
{
	isWindowOpen_ = true;
}
// Called when window is closed
void SpotifyTool::OnClose()
{
	isWindowOpen_ = false;
}

#pragma region Spotify
void SpotifyTool::Setup_spotify() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	code_spotify = data.value("code", "");
	setup_statut = data.value("setup_statut", false);
	std::string auth_setup_base = "Basic ";
	std::string auth_setup = auth_setup_base + data.value("base64", "");
	if (setup_statut == false) {
		CurlRequest req;
		req.url = "https://accounts.spotify.com/api/token";
		req.verb = "POST";
		req.headers = {
			{"Authorization", auth_setup },
			{"Content-Type", "application/x-www-form-urlencoded"}
		};
		req.body = "redirect_uri=http%3A%2F%2Flocalhost%3A8888%2Fauth%2Fspotify%2Fcallback&grant_type=authorization_code&code=" + code_spotify;
		LOG("sending body request");
		HttpWrapper::SendCurlRequest(req, [this](int response_code, std::string result)
			{
				if (response_code == 200) {
					json token_complete = json::parse(result.begin(), result.end());
					std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
					json data = json::parse(f);
					f.close();
					refresh_token = token_complete["refresh_token"];
					access_token = token_complete["access_token"];
					data["access_token"] = access_token;
					data["refresh_token"] = refresh_token;
					std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
					file << data;
					file.close();
					LOG("Setup completed with success!");
				}
				else {
					LOG("Problem with setup... Error code {}, got {} as a result", response_code, result);
				}
			});
		std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
		json data = json::parse(f);
		f.close();
		setup_statut = true;
		data["setup_statut"] = setup_statut;
		std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
		file << data;
		file.close();
	}
	else {
		LOG("Setup already done");
	}
}

void SpotifyTool::Sync_spotify() {

	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_playing;
	req_playing.url = "https://api.spotify.com/v1/me/player/currently-playing";
	req_playing.verb = "GET";
	req_playing.headers = {

		{"Authorization", auth_bearer},
		{"Content-Type", "application/json"}
	};
	HttpWrapper::SendCurlRequest(req_playing, [this](int response_code, std::string result_playing)
		{

			currently_playing = result_playing;
			LOG("Request_result\n{}", response_code);
			if (response_code == 200) {
				std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				json data = json::parse(f);
				f.close();
				json playing_json = json::parse(currently_playing);
				song = playing_json["item"]["name"];
				LOG("Song\n{}", song);
				artist = playing_json["item"]["artists"][0]["name"];
				picture = playing_json["item"]["album"]["images"][0]["url"];
				duration = playing_json["item"]["duration_ms"];
				progress = playing_json["progress_ms"];
				cover = std::make_shared<ImageLinkWrapper>(picture, gameWrapper);
				if (cover)
				{
					if (auto* ptr = cover->GetImguiPtr())
					{
						ImGui::Image(ptr, { 80, 80 });
					}
					else {
						LOG("Image Loading...");
					}
				}
				data["song"] = song;
				data["artist"] = artist;
				data["picture"] = picture;
				data["duration"] = duration;
				data["progress"] = progress;
				std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				file << data;
				file.close();
				doOnce = true;
			}
			else {
				LOG("ERROR IN Sync_spotify with response code {}", response_code);
			}
		});
}

void SpotifyTool::Refresh_token() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	refresh_token = data.value("refresh_token", "");
	CurlRequest req_refresh;
	req_refresh.url = "https://accounts.spotify.com/api/token";
	req_refresh.verb = "POST";
	std::string auth_setup_base = "Basic ";
	std::string auth_setup = auth_setup_base + data.value("base64", "");
	req_refresh.headers = {

		{"Authorization", auth_setup },
		{"Content-Type", "application/x-www-form-urlencoded"}
	};
	req_refresh.body = "grant_type=refresh_token&refresh_token=" + refresh_token;
	HttpWrapper::SendCurlRequest(req_refresh, [this](int response_code, std::string result)
		{
			if (response_code == 200) {
				std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				json data = json::parse(f);
				f.close();
				json token_complete = json::parse(result.begin(), result.end());
				access_token = token_complete["access_token"];
				data["access_token"] = access_token;
				std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				file << data;
				file.close();
				LOG("Refresh was a success!");
			}
			else {
				LOG("ERROR IN Refresh_token with response code {}", response_code);
			}
		});
}

void SpotifyTool::Skip_song() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_skip;
	req_skip.url = "https://api.spotify.com/v1/me/player/next";
	req_skip.verb = "POST";
	req_skip.headers = {
		{"Authorization", auth_bearer},
		{"Content-Length", "0"},
		{"Content-Type", "application/json"}
	};
	HttpWrapper::SendCurlRequest(req_skip, [&](int response_code, std::string result_skip)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 204) {
				LOG("Song skipped");
				LOG("Song refreshed");
				counter = 0;
				Sync_spotify();
			}
			else {
				LOG("Request Problem in Skip_song {}, got {}, please contact the creator with this code", response_code, result_skip);
			}
		});
}

void SpotifyTool::Prev_song() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_prev;
	req_prev.url = "https://api.spotify.com/v1/me/player/previous";
	req_prev.verb = "POST";
	req_prev.headers = {
		{"Authorization", auth_bearer},
		{"Content-Length", "0"},
		{"Content-Type", "application/json"}
	};
	HttpWrapper::SendCurlRequest(req_prev, [this](int response_code, std::string result_skip)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 204) {
				LOG("Jump to previous song");
				LOG("Song refreshed");
				counter = 0;
				Sync_spotify();
			}
			else {
				LOG("Request Problem in Prev_song {}, got {}, please contact the creator with this code", response_code, result_skip);
			}
		});
}

void SpotifyTool::Pause_song() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_prev;
	if (paused) {
		req_prev.url = "https://api.spotify.com/v1/me/player/play";
		paused = false;
	}
	else {
		req_prev.url = "https://api.spotify.com/v1/me/player/pause";
		paused = true;
	}
	req_prev.verb = "PUT";
	req_prev.headers = {
		{"Authorization", auth_bearer},
		{"Content-Length", "0"},
		{"Content-Type", "application/json"}
	};

	static bool init = false;
	if (!init)
	{
		init = SMTCManager::Initialize();
	}

	if (init)
		SMTCManager::TogglePausePlay();

	/*
	HttpWrapper::SendCurlRequest(req_prev, [this](int response_code, std::string result_skip)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 204) {
				if (paused) {
					LOG("Song paused!");
				}
				else {
					LOG("Song resumed!");
				}
			}
			else {
				LOG("Request Problem in Pause_song {}, got {}, please contact the creator with this code", response_code, result_skip);
			}
		});
	*/
}
void SpotifyTool::Search_spotify(std::string query, int amount) {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_search;
	if (search_type)
	{
		req_search.url = "https://api.spotify.com/v1/search?q=" + query + "&type=track&limit=" + std::to_string(amount);
	}
	else
	{
		req_search.url = "https://api.spotify.com/v1/search?q=" + query + "&type=playlist&limit=" + std::to_string(amount);
	}
	req_search.verb = "GET";
	req_search.headers = {
		{"Authorization", auth_bearer},
		{"Content-Type", "application/json"}
	};
	HttpWrapper::SendCurlRequest(req_search, [this, amount](int response_code, std::string result_search)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 200) {
				std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				json data = json::parse(f);
				f.close();
				json complete_data = json::parse(result_search.begin(), result_search.end());
				std::string trackId = complete_data["tracks"]["items"][0]["uri"];
				
				/*
				for (int i = 0; i < amount; ++i)
				{
					std::string trackId = complete_data["tracks"]["items"][i]["uri"];
					uri_list.push_back(trackId);
				}
				data["searched"] = uri_list;
				*/

				data["searched"] = trackId;
				std::ofstream file(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
				file << data;
				file.close();
			}
			else {
				LOG("Request Problem in Search_spotify {}, got {}, please contact the creator with this code", response_code, result_search);
			}
		});
}
void SpotifyTool::Queue_song() {
	std::ifstream f(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + "stool_config.json");
	json data = json::parse(f);
	f.close();
	access_token = data.value("access_token", "");
	std::string track_uri = data.value("searched", "");
	auth = "Bearer ";
	auth_bearer = auth + access_token;
	CurlRequest req_queue;
	req_queue.url = "https://api.spotify.com/v1/me/player/queue?uri="+ track_uri;
	req_queue.verb = "POST";
	req_queue.headers = {
		{"Authorization", auth_bearer},
		{"Content-Length", "0"},
		{"Content-Type", "application/json"}
	};
	HttpWrapper::SendCurlRequest(req_queue, [this](int response_code, std::string result_queue)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 200 or response_code == 204) {
				LOG("Song queued");
			}
			else {
				LOG("Request Problem in Queue song {}, got {}, please contact the creator with this code", response_code, result_queue);
			}
		});
}

/*
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void writeToLine(string filename, string text, int lineNumber) {
  // Open the file in output mode
  ofstream file(filename, ios::out);

  // If the file couldn't be opened, print an error message and return
  if (!file.is_open()) {
	cerr << "Error: Could not open file '" << filename << "' for writing." << endl;
	return;
  }

  // Write the text to the specified line in the file
  for (int i = 1; i <= lineNumber; i++) {
	if (i == lineNumber) {
	  file << text << endl;
	} else {
	  file << endl;
	}
  }

  // Close the file
  file.close();
}
*/