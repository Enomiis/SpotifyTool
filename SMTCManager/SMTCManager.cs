using System;
using System.IO;
using Windows.Media.Control;
using System.Drawing;
using Windows.Media;
using Windows.Perception.Spatial.Preview;

namespace SMTCManager
{
    public class SMTCManager
    {
        private GlobalSystemMediaTransportControlsSessionManager manager = null;
        private GlobalSystemMediaTransportControlsSession currSession = null;
        private bool initialized = false, logEnabled = true;

        private string mediaType = null, artist = null, title = null, subtitle = null;
        bool hasThumbnail = false;

        public bool Initialized { get => this.initialized; }
        public bool LogEnabled { get => logEnabled; set => logEnabled = value; }
        public string MediaType { get => this.mediaType != null ? this.mediaType: ""; }
        public string Artist { get => this.artist != null ? this.artist : ""; }
        public string Title { get => this.title != null ? this.title : "";  }
        public string Subtitle { get => this.subtitle != null ? this.subtitle : "";  }
        public bool HasThumbnail { get => this.hasThumbnail;  }
        public bool HasSession { get => this.currSession != null;  }

        private const string LOGFILE_PATH = @".\smtc-log.txt";
        private const string THUMBNAIL_PATH = @".\smtc-thumbnail.png";

        public SMTCManager()
        {
            Log("SMTCController - Starting...");
        }

        public bool Init()
        {
            var task = GlobalSystemMediaTransportControlsSessionManager.RequestAsync().AsTask();
            task.Wait();
            this.manager = task.Result;
            
            if (this.manager != null)
            {
                Log("GlobalSMTCSessionManager object Retrieved!");
                this.initialized = true;

                this.manager.CurrentSessionChanged += OnCurrentSessionChanged;
                RefreshCurrentSession();
            }
            else { this.initialized = false; }

            return this.initialized;
        }
        public void Quit()
        {
            if (!this.initialized) return;
            
            this.manager.CurrentSessionChanged -= OnCurrentSessionChanged;
        }

        public void SkipToNext()
        {
            if (this.currSession == null) return;

            Log("Trying to skip to next media...");
            this.currSession.TrySkipNextAsync().AsTask().Wait();
        }
        public void SkipToPrevious()
        {
            if (this.currSession == null) return;

            Log("Trying to skip to previous media...");
            this.currSession.TrySkipPreviousAsync().AsTask().Wait();
        }
        public void Play()
        {
            if (this.currSession == null) return;

            Log("Trying to play media playback...");
            this.currSession.TryPlayAsync().AsTask().Wait();
        }
        public void Pause()
        {
            if (this.currSession == null) return;

            Log("Trying to pause media playback...");
            this.currSession.TryPauseAsync().AsTask().Wait();
        }
        public void TogglePausePlay()
        {
            if (this.currSession == null) return;

            Log("Trying to toggle media playback...");
            this.currSession.TryTogglePlayPauseAsync().AsTask().Wait();
        }

        private void OnCurrentSessionChanged(GlobalSystemMediaTransportControlsSessionManager manager, CurrentSessionChangedEventArgs args)
        {
            Log("Current Session Changed!");
            RefreshCurrentSession();
        }
        private void OnMediaPropChanged(GlobalSystemMediaTransportControlsSession sesion, MediaPropertiesChangedEventArgs args)
        {
            Log("Media Properties changed!");
            RefreshMediaProp();
        }

        private void RefreshCurrentSession()
        {
            Log("Refreshing current session...");
            if (this.currSession != null)
            {
                this.currSession.MediaPropertiesChanged -= OnMediaPropChanged;
            }

            this.currSession = this.manager.GetCurrentSession();
            if (this.currSession != null)
            {
                this.currSession.MediaPropertiesChanged += OnMediaPropChanged;
                RefreshMediaProp();
            }
        }
        private void RefreshMediaProp()
        {
            string log_string = "Refreshing media properties information...\n";
            var task = this.currSession.TryGetMediaPropertiesAsync().AsTask();
            task.Wait();
            var mediaProp = task.Result;

            switch (mediaProp.PlaybackType.Value)
            {
                case MediaPlaybackType.Music:
                    this.mediaType = "MUSIC";
                    break;
                case MediaPlaybackType.Video:
                    this.mediaType = "VIDEO";
                    break;
                case MediaPlaybackType.Image:
                    this.mediaType = "IMAGE";
                    break;
                default:
                    this.mediaType = "UNKNOWN";
                    break;
            }
            log_string += $"\tSession Media Type: {this.mediaType}\n";
            
            this.artist = mediaProp.Artist;
            this.title = mediaProp.Title;
            this.subtitle = mediaProp.Subtitle;

            log_string += this.Artist;
            log_string += this.Title;
            log_string += this.Subtitle;

            if (mediaProp.Thumbnail != null)
            {
                log_string += $"Thumbnail available. Saving thumbnail to '{THUMBNAIL_PATH}'";
                var thumbnailTask = mediaProp.Thumbnail.OpenReadAsync().AsTask();
                thumbnailTask.Wait();

                Image.FromStream(thumbnailTask.Result.AsStream())
                    .Save(THUMBNAIL_PATH);

                this.hasThumbnail = true;
            }
            else
            {
                log_string += "No thumbnail available.";
                this.hasThumbnail = false;
            }

            Log(log_string);
        }

        public void Log(string text)
        {
            if (this.logEnabled)
                File.AppendAllText(LOGFILE_PATH, System.DateTime.Now.ToString("[dd/MM/yy HH:mm] ") + text + "\r\n");
        }
    }
}
