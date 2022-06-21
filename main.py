from SwSpotify import spotify
import time
while True:
    track = spotify.song()
    artists = spotify.artist()
    to_write = str(f"{track} by {artists}")

    file = open(r"C:\Users\User\AppData\Roaming\bakkesmod\bakkesmod\SpotifyTool\Song.txt", "w") 
    file.write(to_write) 
    file.close()
    time.sleep(2)
