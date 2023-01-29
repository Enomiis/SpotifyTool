import requests
import pyautogui
import pyperclip
import json
import os
import webbrowser
import time

def typewrite(text):
    pyperclip.copy(text)
    pyautogui.hotkey('ctrl', 'v')
    pyperclip.copy('')

pyautogui.FAILSAFE = False
bakkesmod_directory = input("Directory to BakkesMod folder? ")
web_browser = input("Which web browser are you using? ")
web_browser = web_browser.lower()
stool_directory = os.path.join(bakkesmod_directory, "SpotifyTool")
stool_config = os.path.join(stool_directory, "oauth_code.txt")
stool_logo = os.path.join(stool_directory, "stool_logo.png")
stool_log = os.path.join(stool_directory, "stool_log.log")
os.chdir(bakkesmod_directory)
if not os.path.exists(stool_directory):
    os.makedirs(stool_directory)
os.chdir(stool_directory)
link = "https://accounts.spotify.com/fr/authorize?client_id=fb6c93e9463e4103a4a06adf4d373786&response_type=code&scope=user-modify-playback-state,user-read-currently-playing&redirect_uri=http://localhost:8888/auth/spotify/callback"
webbrowser.open(link)
while not "localhost" in str(os.popen("tasklist /v /fi \"IMAGENAME eq " + str(web_browser) + ".exe\" /fo \"list\" | find \"Titre de la fenêtre:\"").read()):
    pass
time.sleep(5)
code = ""
pyautogui.keyDown("ctrl")
pyautogui.keyDown("shift")
pyautogui.keyDown("j")
pyautogui.keyUp('j')
pyautogui.keyUp('shift')
pyautogui.keyUp('ctrl')
time.sleep(2)
typewrite('document.title=pageData.reloadButton.reloadUrl.split("code=")[1]')
pyautogui.press("enter")
time.sleep(2)
url_containing_code = (str(os.popen("tasklist /v /fi \"IMAGENAME eq " + str(web_browser) + ".exe\" /fo \"list\" | find \"Titre de la fenêtre:\"").read()).split("\n")[0])
code += url_containing_code.split(": ")[-1]
typewrite('document.title=document.title.substring(255)')
pyautogui.press("enter")
time.sleep(2)
url_containing_code = (str(os.popen("tasklist /v /fi \"IMAGENAME eq " + str(web_browser) + ".exe\" /fo \"list\" | find \"Titre de la fenêtre:\"").read()).split("\n")[0])
index = (url_containing_code.split(": ")[-1]).find(" - ")
code += (url_containing_code.split(": ")[-1])[:-index+4]
pyautogui.keyDown("ctrl")
pyautogui.keyDown("shift")
pyautogui.keyDown("j")
pyautogui.keyUp('j')
pyautogui.keyUp('shift')
pyautogui.keyUp('ctrl')
pyautogui.keyDown('ctrl')
pyautogui.press('w')
pyautogui.keyUp('ctrl')
open(stool_config, 'w').write(code)
if not os.path.exists(stool_logo):
    url = "https://cdn.discordapp.com/attachments/1018608568527749210/1018613685398274108/spotifytool_logo.png"
    r = requests.get(url, allow_redirects=True)
    open(stool_logo, 'wb').write(r.content)
if not os.path.exists(stool_log):
    open(stool_log, 'w').write("")