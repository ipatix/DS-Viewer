#pragma once
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "Image.h"
#include "Xcept.h"
#include "TRingbuffer.h"

class MediaViewer
{
    public:
        MediaViewer(Image& _top, Image& _bottom);
        ~MediaViewer();
        bool UpdateVideo(bool blank);
        TRingbuffer<float>& GetAudioBuffer();
		static bool IsMuted() { return mute; }
    private:
        void clearTexture();
        void imageTexture();
        void toggleFullscreen();

        static void audioCallback(void *userdata, uint8_t *stream, int len);

        SDL_Window *win;
        SDL_Renderer *ren;
        SDL_Texture *tex;

        bool fullscreen;
		static bool mute;

		int win_h, win_w;

        Image& top;
        Image& bottom;
        
        SDL_AudioDeviceID audioDeviceID;
        TRingbuffer<float> audiobuf;
};
