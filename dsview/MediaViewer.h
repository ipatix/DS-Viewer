#pragma once
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <boost/lockfree/spsc_queue.hpp>

#include "Image.h"
#include "Xcept.h"

class MediaViewer
{
    public:
        MediaViewer(Image& _top, Image& _bottom, boost::lockfree::spsc_queue<float>& _audio_buffer);
        ~MediaViewer();
        bool UpdateVideo(bool blank);
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

		boost::lockfree::spsc_queue<float>& audio_buffer;
        
        SDL_AudioDeviceID audioDeviceID;
};
