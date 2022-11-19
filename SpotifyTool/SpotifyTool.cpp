#include "pch.h"
#include "SpotifyTool.h"
#include <fstream>
#include <iostream>
#include <string>
#include "nlohmann/json.hpp"
#include <string>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"
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

BAKKESMOD_PLUGIN(SpotifyTool, "Spotify Tool Plugin", "0.1.0.0", PERMISSION_ALL)
using namespace std;
using json = nlohmann::json;
shared_ptr<CVarManagerWrapper> _globalCvarManager;



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
	Sync_spotify();
	cvarManager->registerNotifier("Skip_song", [this](std::vector<std::string> args) {
		Skip_song();
		}, "", PERMISSION_ALL);
	cvarManager->log("working bro");
	gameWrapper->LoadToastTexture("spotifytool_logo", gameWrapper->GetDataFolder() / "spotifytool_logo.png");
	gameWrapper->Toast("SpotifyTool", "SpotifyTool is loaded", "spotifytool_logo", 5.0, ToastType_Warning);
	cvarManager->registerCvar("stool_enabled", "1", "Enable Spotify Tool", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		stoolEnabled = cvar.getBoolValue();
			});

	cvarManager->registerCvar("stool_color", "#FFFFFF", "color of overlay");
}

#pragma region Data manipulation
struct MySetting
{
	std::string code;
	std::string access_token;
	std::string refresh_token;
	std::string setup_statut;
	std::string song;
	std::string artist;
	std::string picture;
	std::string duration;
	std::string progress;
	std::string base64;

	static MySetting Load(const std::filesystem::path& load_path);
	void Save(const std::filesystem::path& save_path);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MySetting, foo, bar, baars)
MySetting MySetting::Load(const std::filesystem::path& load_path)
{
    return json::parse(std::ifstream(load_path));
}

void MySetting::Save(const std::filesystem::path& save_path)
{
    std::ofstream(save_path) << json{*this};
}

void SpotifyTool::onUnload() {

	WriteInFile("song.txt", "No Spotify Activity");

}

#pragma region Spotify
void SpotifyTool::Setup_spotify() {
	code_spotify = LoadofFile("code");
	LOG("Body_result{}", code_spotify);
	setup_statut = LoadofFile("setup_statut");
	base64 = LoadofFile("base64");
	if (setup_statut == "false") {
		CurlRequest req;
		req.url = "https://accounts.spotify.com/api/token";
		req.verb = "POST";
		req.headers = {

			{"Authorization", "Basic "+base64},
			{"Content-Type", "application/x-www-form-urlencoded"}
		};
		req.body = "redirect_uri=http%3A%2F%2Flocalhost%3A8888%2Fauth%2Fspotify%2Fcallback&grant_type=authorization_code&code=" + code_spotify;

		LOG("sending body request");
		HttpWrapper::SendCurlRequest(req, [this](int response_code, std::string result)
			{
				if (response_code == 200) {
					token = result;
					WriteInFile("return.txt", token);
					json token_complete = json::parse(token.begin(), token.end());
					refresh_token = token_complete["refresh_token"];
					access_token = token_complete["access_token"];
					WriteInFile("access_token", access_token);
					WriteInFile("refresh_token", refresh_token);

				}
			});
		setup_statut = "true";
		WriteInFile("setup_statut", setup_statut);
	}
}

void SpotifyTool::Sync_spotify() {

	access_token = LoadofFile("access_token.txt");
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
			WriteInFile("song_request.json", currently_playing);
			LOG("Request_result\n{}", response_code);
			if (response_code == 200) {
				json playing_json = json::parse(currently_playing);
				song = playing_json["item"]["name"];
				LOG("Song{}", song);
				WriteInFile("song", song);
				artist = playing_json["item"]["artists"][0]["name"];
				WriteInFile("artist", artist);
				picture = playing_json["item"]["album"]["images"][0]["url"];
				WriteInFile("picture", picture);
				duration = playing_json["item"]["duration_ms"];
				WriteInFile("duration", std::to_string(duration));
				progress = playing_json["progress_ms"];
				WriteInFile("progress", std::to_string(progress));
				cover = std::make_shared<ImageLinkWrapper>(LoadofFile("picture"), gameWrapper);
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
			}
		});
}

void SpotifyTool::Refresh_token() {
	refresh_token = LoadofFile("refresh_token");
	CurlRequest req_refresh;
	req_refresh.url = "https://accounts.spotify.com/api/token";
	req_refresh.verb = "POST";
	req_refresh.headers = {

		{"Authorization", "Basic ZmI2YzkzZTk0NjNlNDEwM2E0YTA2YWRmNGQzNzM3ODY6NzcwMDg0NTAzOTg0NDdiNWE0ZjY1Yzg1NDI0YzZhMjU=" },
		{"Content-Type", "application/x-www-form-urlencoded"}
	};
	req_refresh.body = "grant_type=refresh_token&refresh_token=" + refresh_token;
	HttpWrapper::SendCurlRequest(req_refresh, [this](int response_code, std::string result)
		{
			if (response_code == 200) {
				token = result;
				json token_complete = json::parse(token.begin(), token.end());
				access_token = token_complete["access_token"];
				WriteInFile("access_token", access_token);
			}
		});
}

void SpotifyTool::Skip_song() {
	access_token = LoadofFile("access_token");
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

	HttpWrapper::SendCurlRequest(req_skip, [this](int response_code, std::string result_skip)
		{
			LOG("Request_result\n{}", response_code);
			if (response_code == 204) {
				LOG("Song skipped");
				LOG("Song refreshed");
				skipped = true;
				doOnce = true;
			}
			else {
				LOG("Request Problem in Skip_song {}, please contact the creator with this code", response_code);
			}
		});
	
}

// Name of the plugin to be shown on the f2 -> plugins list
std::string SpotifyTool::GetPluginName()
{
	return "SpotifyTool early beta";
}


#pragma region Rendering
void SpotifyTool::RenderSettings() {
	ImGui::TextUnformatted("A Plugin for BM made to manage and display the currently playing song on Spotify. Huge thanks to the BakkesMod Programming Discord for carrying me to this <3");
	if (ImGui::Button("Sync Spotify")) {
		Sync_spotify();
	}
	ImGui::Checkbox("Drag Mode", &moveOverlay);

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Sync your activity");
	}

	CVarWrapper enableCvar = cvarManager->getCvar("stool_enabled");

	if (!enableCvar) {
		return;
	}

	bool enabled = enableCvar.getBoolValue();

	if (ImGui::Checkbox("Enable plugin", &enabled)) {
		enableCvar.setValue(enabled);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Toggle SpotifyTool Plugin");
	}


	CVarWrapper xLocCvar = cvarManager->getCvar("stool_x_location");
	if (!xLocCvar) { return; }
	float xLoc = xLocCvar.getFloatValue();
	if (ImGui::SliderFloat("Text X Location", &xLoc, 0.0, 1920)) {
		xLocCvar.setValue(xLoc);
	}
	CVarWrapper yLocCvar = cvarManager->getCvar("stool_y_location");
	if (!yLocCvar) { return; }
	float yLoc = yLocCvar.getFloatValue();
	if (ImGui::SliderFloat("Text Y Location", &yLoc, 0.0, 1080)) {
		yLocCvar.setValue(yLoc);
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
			| ImGuiWindowFlags_NoFocusOnAppearing;

		if (!moveOverlay)
		{
			WindowFlags |= ImGuiWindowFlags_NoInputs;
		}

		// uncomment if you don't want a background (flag found by just checking for the other window flags available
		//WindowFlags |= ImGuiWindowFlags_NoBackground;


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
			ImGui::Text("%s",song.c_str());
			ImGui::Text("%s", artist.c_str());
		}
		else
		{
			ImGui::Text("The custom font haven't been loaded yet");
		}
	
		if (doOnce) {
			duration_ms = std::stoi(LoadofFile("duration"));
			progress_ms = std::stoi(LoadofFile("progress"));
			song_duration = ((duration_ms - progress_ms) / 1000) + 4;
			doOnce = false;
		}
		counter += ImGui::GetIO().DeltaTime;
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
			song = LoadofFile("song");
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

void SpotifyTool::DragWidget(CVarWrapper xLocCvar, CVarWrapper yLocCvar) {
	ImGui::Checkbox("Drag Mode", &inDragMode);

	if (inDragMode) {
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
			xLocCvar.setValue(mousePos.x);
			yLocCvar.setValue(mousePos.y);
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


// Changing color is work in progress
/*
	CVarWrapper textColorVar = cvarManager->getCvar("stool_color");
	if (!textColorVar) { return; }
	// converts from 0-255 color to 0.0-1.0 color
	LinearColor textColor = textColorVar.getColorValue() / 255;
	if (ImGui::ColorEdit4("Text Color", &textColor.R)) {
		textColorVar.setValue(textColor * 255);
	}
*/