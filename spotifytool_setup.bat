set /p UserInputPath=Directory to BakkesMod folder? 
cd %UserInputPath%
if not exist "SpotifyTool" md "SpotifyTool"
cd SpotifyTool
if exist "stool_config.json" del "stool_config.json"
set code = ""
echo {"access_token":"","artist":"","base64":"ZmI2YzkzZTk0NjNlNDEwM2E0YTA2YWRmNGQzNzM3ODY6NzcwMDg0NTAzOTg0NDdiNWE0ZjY1Yzg1NDI0YzZhMjU=","code":"","duration":0,"picture":"","progress":0,"refresh_token":"","setup_statut":false,"song":""}> stool_config.json
if not exist "spotifytool_logo.png" curl -o "spotifytool_logo.png" https://cdn.discordapp.com/attachments/1018608568527749210/1018613685398274108/spotifytool_logo.png
start "" "https://accounts.spotify.com/fr/authorize?client_id=fb6c93e9463e4103a4a06adf4d373786&response_type=code&scope=user-modify-playback-state,user-read-currently-playing&redirect_uri=http://localhost:8888/auth/spotify/callback"