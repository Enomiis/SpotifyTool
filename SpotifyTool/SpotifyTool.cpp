#include "pch.h"
#include "SpotifyTool.h"
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
/*

TO DO LIST:

 Fix CURL
 Fix Copy/Paste user input
 Parse access token
 Parse refresh token
 Get song
 Get picture
 Picture in options
 Skip/Pause
 Keybinds


 LEARN TO FCKG CODE IN C++

*/
BAKKESMOD_PLUGIN(SpotifyTool, "Spotify Tool Plugin", "0.1.0.0", PERMISSION_ALL)
using namespace std;
string get_token, get_token2, pos, command_token, command_end, command;
string song_file = "C:\\Users\\User\\AppData\\Roaming\\bakkesmod\\bakkesmod\\SpotifyTool\\song.txt";
shared_ptr<CVarManagerWrapper> _globalCvarManager;
bool stoolEnabled = true;
string song;
fstream file;
string response;

void SpotifyTool::onLoad()
{
	_globalCvarManager = cvarManager;
	cvarManager->registerCvar("stool_scale", "1", "Overlay scale", true, true, 0, true, 10);
	cvarManager->registerNotifier("Sync_spotify", [this](std::vector<std::string> args) {
		Sync_spotify();
		}, "", PERMISSION_ALL);
	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		Render(canvas);
		});
	get_token = "https://google.com/callback?code=AQD4jiy230t5Us2gu5R-3dtuUHblXf-7fcuPR0p77lY7HVpbqYFXP1mTql8pECYxEE6hHRS5bLXCaasIYkqAFJORoCgQMAQfHKUcU8_WSs08ucV7soBKUHrWt5GD3IRxBanpTuW355H5qbk0ghN3gFz-iJR9CsxHHFRyIHJkx1sZnDDdSAkSEnb6IunpCeMRnOGlY-tsjUj33fwK4yAYGiBcyZSwSm5lodSmn8N-sKl88K6ZXyjkjw";
	pos = get_token.find("code =");
	get_token2 = get_token.substr(26, 246);
	CurlRequest req;
	req.url = "https://accounts.spotify.com/api/token";
	req.verb = "GET";
	req.headers = {

		{"Authorization", "Basic ZmI2YzkzZTk0NjNlNDEwM2E0YTA2YWRmNGQzNzM3ODY6NzcwMDg0NTAzOTg0NDdiNWE0ZjY1Yzg1NDI0YzZhMjU =" }

	};
	req.body = "-d grant_type=authorization_code&code=AQDUcREXNjjXmOQyYJ9m4UWa701f-3a-6HI-S_lfcGzF9TgCQccPGIX8xcIp5tCKFxGSOKFnBejdIMUJxolaZbkpvkqZ1L63j5VCAh-sDIUBb6Lq3SFzXm_xzf1oZQG2Cv9KaTWRMjAFoZSCeRtwWj9cv4yIDftvAT77yP5v9UaSXUSIwXM90EZAqidXqjqDEnTIMoRTFc8xDz1YPi7n1tErOYMd2Hsh30bI1FaQAWZYreimmYH1vA&redirect_uri=http%3A%2F%2Flocalhost:8888%2Fauth%2Fspotify%2Fcallback";
	
	LOG("sending body request");
	HttpWrapper::SendCurlRequest(req, [this](int code, std::string result)
		{
			LOG("Body result{}", result);
		});
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
void SpotifyTool::onUnload() {
	cvarManager->log("I was too stool for this world B'(");
	WriteInFile("Song.txt", "No Spotify Activty");
}
// in a .cpp file 
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
			int(xLoc)*stool_scale,
			int(yLoc)*stool_scale };
		Vector2 sizeBox = {
			175*float(stool_scale),
			30*float(stool_scale)
		};
		canvas.SetPosition(drawLoc);

		// Set background color
		canvas.SetColor(LinearColor{ 0, 0, 0, 150 });
		canvas.FillBox(sizeBox);
		// Draw text
		Vector2 textPos = { float(drawLoc.X + 25), float(drawLoc.Y + 5) };
	
		// Set the position
		canvas.SetPosition(textPos);
		canvas.SetColor(textColor);
		file.open(song_file, ios::in);
		if (file.is_open()) {
			getline(file, song);
		}
		file.close();
		canvas.DrawString(song, stool_scale, stool_scale);
	}
	else
	{
		return;
	}
}
void SpotifyTool::Sync_spotify() {
	
	
	cvarManager->log("Sync done");
	file.open(song_file, ios::in);
	if (file.is_open()) {
		getline(file, song);
	}
	file.close();
}
void SpotifyTool::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Name of the plugin to be shown on the f2 -> plugins list
std::string SpotifyTool::GetPluginName()
{
	return "SpotifyTool";
}

bool inDragMode = false;

void SpotifyTool::RenderSettings() {
	ImGui::TextUnformatted("A Plugin for BM made to manage and display the currently playing song on Spotify");
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
	static char command[128];
	ImGui::InputText("Input callback", command, IM_ARRAYSIZE(command));
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
/*
void SpotifyTool::SpotifyRequest(std::string method, std::string endpoint, std::string body, std::function<void(int status, std::string data)> cb) {
	CurlRequest req;
	req.url = "https://api.spotify.com/v1" + endpoint;
	req.verb = method;
	req.body = body;
	req.headers = {
		{"Content-Type", "application/json"},
		{"Content-Length", std::to_string(body.length())},
		{"Authorization", accessToken}
	};

	try {
		HttpWrapper::SendCurlRequest(req, [this, method, endpoint, body, cb](int code, std::string res) {
			if (res == "") return cb(code, res);

			json stuff = json::parse(res);
			if (code == 401 && stuff["error"]["message"] == "The access token expired") {
				this->GetAccessToken();
				this->SpotifyRequest(method, endpoint, body, cb);
			}
			else {
				cb(code, res);
			}
			});
	}
	catch (...) {
		this->Log("CRASH IN WEB THREAD OR SOMETHING");
	}
}

this->SpotifyRequest("https://api.spotify.com/v1", "/me/player/pause", "", [this](int code, string res) {
	this->Log(res);
	});
*/