#include "pch.h"
#include "SpotifyTool.h"
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <string>

/*

TO DO LIST:

 Fix CURL (DONE)
 Fix Copy/Paste user input
 Parse access token (DONE)
 Parse refresh token (DONE)
 Get song (DONE)
 Get picture
 Picture in options
 Skip/Pause
 Keybinds


 LEARN TO FCKG CODE IN C++

*/

BAKKESMOD_PLUGIN(SpotifyTool, "Spotify Tool Plugin", "0.1.0.0", PERMISSION_ALL)
using namespace std;
using json = nlohmann::json;
shared_ptr<CVarManagerWrapper> _globalCvarManager;



void SpotifyTool::onLoad()
{
	_globalCvarManager = cvarManager;
	cvarManager->registerCvar("stool_scale", "1", "Overlay scale", true, true, 0, true, 10, true);
	cvarManager->registerNotifier("Sync_spotify", [this](std::vector<std::string> args) {
		Sync_spotify();
		}, "", PERMISSION_ALL);
	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		Render(canvas);
		});
	Setup_spotify();
	Refresh_token();
	Sync_spotify();

	cvarManager->log("working bro");
	gameWrapper->LoadToastTexture("spotifytool_logo", gameWrapper->GetDataFolder() / "spotifytool_logo.png");
	gameWrapper->Toast("SpotifyTool", "SpotifyTool is loaded", "spotifytool_logo", 5.0, ToastType_Warning);
	cvarManager->registerCvar("stool_enabled", "1", "Enable Spotify Tool", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		stoolEnabled = cvar.getBoolValue();
			});

	cvarManager->registerCvar("stool_color", "#FFFFFF", "color of overlay");
	gameWrapper->RegisterDrawable(std::bind(&SpotifyTool::Render, this, std::placeholders::_1));

	cvarManager->registerCvar("stool_x_location", "0", "set x location of the overlay");
	cvarManager->registerCvar("stool_y_location", "0", "set y location of the overlay");
}

#pragma region File I / O
void SpotifyTool::WriteInFile(std::string _filename, std::string _value)
{
	std::ofstream stream(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + _filename, std::ios::out | std::ios::trunc);

	if (stream.is_open())
	{
		stream << _value;
		stream.close();
	}
	else
	{
		cvarManager->log("Can't write to file: " + _filename);
		cvarManager->log("Value to write was: " + _value);
	}
}
string SpotifyTool::LoadofFile(std::string _filename)
{
	string value;
	ifstream stream;
	stream.open(gameWrapper->GetBakkesModPath().string() + "\\SpotifyTool\\" + _filename);
	if (stream.is_open())
	{
		getline(stream, value);
		stream.close();
		cout << value;
	}
	return value;
}


void SpotifyTool::onUnload() {
	
	WriteInFile("song.txt", "No Spotify Activity");

}

void SpotifyTool::Render(CanvasWrapper canvas) {
	
	float stool_scale = cvarManager->getCvar("stool_scale").getFloatValue();
	if (!stool_scale) { return; }
	CVarWrapper textColorVar = cvarManager->getCvar("stool_color");
	if (!textColorVar) {
		return;
	}

	LinearColor textColor = textColorVar.getColorValue();
	canvas.SetColor(textColor);
	
	CVarWrapper xLocCvar = cvarManager->getCvar("stool_x_location");
	if (!xLocCvar) { return; }
	float xLoc = xLocCvar.getFloatValue();

	CVarWrapper yLocCvar = cvarManager->getCvar("stool_y_location");
	if (!yLocCvar) { return; }
	float yLoc = yLocCvar.getFloatValue();
	CVarWrapper enableCvar = cvarManager->getCvar("stool_enabled");
	bool enabled = enableCvar.getBoolValue();

	if (enabled) {
		canvas.SetPosition(Vector2{ int(xLoc), int(yLoc) });
		// Draw box here
		Vector2 drawLoc = {
			int(xLoc) * stool_scale,
			int(yLoc) * stool_scale };
		// Draw text
		Vector2 textPos = { float(drawLoc.X + 25), float(drawLoc.Y + 5) };

		// Set the position
		canvas.SetPosition(textPos);
		canvas.SetColor(textColor);

		time += ImGui::GetIO().DeltaTime;
		time2 += ImGui::GetIO().DeltaTime;
		if (time > 30)
		{
			Sync_spotify();
			time = 0;
		}
		if (time2 > 3500)
		{
			
			Refresh_token();
			time2 = 0;
		}
		song = LoadofFile("song.txt");
		canvas.DrawString(song, stool_scale, stool_scale);
	}
	else
	{
		return;
	}
}


#pragma region Spotify
void SpotifyTool::Setup_spotify() {
	code = LoadofFile("code.txt");
	LOG("Body_result{}", code);
	setup_status = LoadofFile("setup_status.txt");
	if (setup_status == "false") {
		CurlRequest req;
		req.url = "https://accounts.spotify.com/api/token";
		req.verb = "POST";
		req.headers = {

			{"Authorization", "Basic ZmI2YzkzZTk0NjNlNDEwM2E0YTA2YWRmNGQzNzM3ODY6NzcwMDg0NTAzOTg0NDdiNWE0ZjY1Yzg1NDI0YzZhMjU=" },
			{"Content-Type", "application/x-www-form-urlencoded"}
		};
		req.body = "redirect_uri=http%3A%2F%2Flocalhost%3A8888%2Fauth%2Fspotify%2Fcallback&grant_type=authorization_code&code=" + code;

		LOG("sending body request");
		HttpWrapper::SendCurlRequest(req, [this](int code, std::string result)
			{
				if (code == 200) {
					token = result;
					WriteInFile("return.txt", token);
					json token_complete = json::parse(token.begin(), token.end());
					refresh_token = token_complete["refresh_token"];
					access_token = token_complete["access_token"];
					WriteInFile("access_token.txt", access_token);
					WriteInFile("refresh_token.txt", refresh_token);

				}
				LOG("Body result:\n{}", result);
			});
		setup_status = "true";
		WriteInFile("setup_status.txt", setup_status);
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
			LOG("Body_Result\n{}", result_playing);
			if (response_code == 200) {
				json playing_json = json::parse(currently_playing);
				song = playing_json["item"]["name"];
				LOG("Song{}", song);
				WriteInFile("song.txt", "");
				WriteInFile("song.txt", song);
				artist = playing_json["item"]["artists"][0]["name"];
				/*picture = playing_json["access_token"];*/
				song_artist = song + " - " + artist;
				WriteInFile("song.txt", song_artist);
			}
		});
	
}

void SpotifyTool::Refresh_token() {
	refresh_token = LoadofFile("refresh_token.txt");
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
				WriteInFile("access_token.txt", access_token);
			}
		});
}



void SpotifyTool::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Name of the plugin to be shown on the f2 -> plugins list
std::string SpotifyTool::GetPluginName()
{
	return "SpotifyTool early alpha";
}

void SpotifyTool::RenderSettings() {
	ImGui::TextUnformatted("A Plugin for BM made to manage and display the currently playing song on Spotify. Huge thanks to the BakkesMod Programming Discord for carrying me to this <3");
	if (ImGui::Button("Sync Spotify")) {
		Sync_spotify();
	}
	
	
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
	DragWidget(xLocCvar, yLocCvar);
	
	CVarWrapper stool_scale = cvarManager->getCvar("stool_scale");
	if (!stool_scale) { return; }
	float scale = stool_scale.getFloatValue();
	if (ImGui::SliderFloat("Scale", &scale, 0.01, 10.0)) {
		stool_scale.setValue(scale);
	}
	
	CVarWrapper textColorVar = cvarManager->getCvar("stool_color");
	if (!textColorVar) { return; }
	// converts from 0-255 color to 0.0-1.0 color
	LinearColor textColor = textColorVar.getColorValue() / 255;
	if (ImGui::ColorEdit4("Text Color", &textColor.R)) {
		textColorVar.setValue(textColor * 255);
	}
	/*static char command[128];
	ImGui::InputText("Input callback", command, IM_ARRAYSIZE(command));
	*/
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
/*
// Name of the menu that is used to toggle the window.
std::string SpotifyTool::GetMenuName()
{
	return "SpotifyTool";
}
// Title to give the menu
std::string SpotifyTool::GetMenuTitle()
{
	return menuTitle_;
}
// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void SpotifyTool::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}
// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool SpotifyTool::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}
// Return true if window should be interactive
bool SpotifyTool::IsActiveOverlay()
{
	return true;
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
*/