#include <cassert>
#include <algorithm>
#include <iostream>

#include "MediaViewer.h"
#include "Config.h"
#include "Xcept.h"

bool MediaViewer::mute = false;

MediaViewer::MediaViewer(Image& _top, Image& _bottom, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : fullscreen(false), top(_top), bottom(_bottom), audio_buffer(_audio_buffer)
{
    // Video
    assert(top.Height() == bottom.Height());
    assert(top.Width() == bottom.Width());
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        throw Xcept("SDL_Init Error: %s", SDL_GetError());
    if (!(win = SDL_CreateWindow("dsview V0.1", 100, 100, VISIBLE_X, VISIBLE_Y * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)))
        throw Xcept("SDL_CreateWindow Error: %s", SDL_GetError());
    if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED)))
        throw Xcept("SDL_CreateRenderer Error: %s", SDL_GetError());
    if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2") == SDL_FALSE)
        throw Xcept("SDL_SetHint Error: %s", SDL_GetError());
    if (!(tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VISIBLE_X + 2, VISIBLE_Y * 2 + 2)))
        throw Xcept("SDL_CreateTexture Error: %s", SDL_GetError());
    SDL_GetWindowSize(win, &win_w, &win_h);
    UpdateVideo(true);
    // Audio
    SDL_AudioSpec spec;
    spec.freq = AUDIO_SAMPLERATE;
    spec.format = AUDIO_F32;
    spec.channels = 2;
    spec.samples = AUDIO_BUF_SIZE / 4;
    spec.callback = audioCallback;
    spec.userdata = &audio_buffer;
    if (!(audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0)))
        throw Xcept("SDL_OpenAudioDevice Error: %s", SDL_GetError());
    SDL_PauseAudioDevice(audioDeviceID, 0);
}

MediaViewer::~MediaViewer()
{
    SDL_CloseAudioDevice(audioDeviceID);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

bool MediaViewer::UpdateVideo(bool blank)
{
    SDL_Event sev;
    while (SDL_PollEvent(&sev))
    {
        switch (sev.type)
        {
            case SDL_WINDOWEVENT:
                switch (sev.window.event)
                {
                    case SDL_WINDOWEVENT_RESTORED:
                        SDL_RestoreWindow(win);
                        SDL_SetWindowSize(win, win_w, win_h);
                        SDL_RestoreWindow(win);
                        //SDL_MaximizeWindow(win);
                        //SDL_GetWindowSize(win, &win_w, &win_h);
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        win_w = sev.window.data1;
                        win_h = sev.window.data2;
                        break;
                    case SDL_WINDOWEVENT_CLOSE:
                        return false;
                }
                break;
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
				if (sev.key.state == SDL_PRESSED && sev.key.keysym.sym == SDLK_f)
					toggleFullscreen();
				else if (sev.key.state == SDL_PRESSED && sev.key.keysym.sym == SDLK_q)
					return false;
				else if (sev.key.state == SDL_PRESSED && sev.key.keysym.sym == SDLK_m)
					mute = !mute;
                break;
        }
    }

    SDL_RenderClear(ren);
    if (blank)
        clearTexture();
    else
        imageTexture();
    SDL_Rect src, dest;
    src.w = int(VISIBLE_X);
    src.h = int(VISIBLE_Y) * 2;
    src.x = 1;
    src.y = 1;
    //float scaling_factor = 1.f;
	float ds_ratio = float(VISIBLE_X) / float(VISIBLE_Y * 2);
	float win_ratio = float(win_w) / float(win_h);
	if (win_ratio > ds_ratio)
	{
		dest.w = int(VISIBLE_X) * win_h / (int(VISIBLE_Y) * 2);
		dest.h = win_h;
		dest.x = win_w / 2 - int(dest.w / 2);
		dest.y = 0;
	}
	else
	{
		dest.w = win_w;
		dest.h = int(VISIBLE_Y * 2) * win_w / (int(VISIBLE_X));
		dest.x = 0;
		dest.y = win_h / 2 - int(dest.h / 2);
	}
    SDL_RenderCopy(ren, tex, &src, &dest);
    //SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);

    return true;
}

void MediaViewer::clearTexture()
{
    void *pixels;
    int pitch;
    {
        uint32_t f;
        int a, w, h;
        SDL_QueryTexture(tex, &f, &a, &w, &h);
    }
    SDL_LockTexture(tex, NULL, &pixels, &pitch);
    uint32_t *upixels = (uint32_t *)pixels;
    size_t offset = size_t(size_t(pitch) / sizeof(uint32_t)) * (VISIBLE_Y * 2 + 2);
    std::fill(upixels, upixels + offset, uint32_t(0xFF00FF));
    SDL_UnlockTexture(tex);
}

void MediaViewer::imageTexture()
{
    void *pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, &pixels, &pitch);
    uint8_t *upixels = (uint8_t *)pixels;
	for (size_t i = 0; i < (size_t(pitch) / sizeof(uint32_t)); i++)
	{
		if (i < VISIBLE_X + 2) {
			*upixels++ = 0; *upixels++ = 0; *upixels++ = 0; *upixels++ = 0;
		} 
		else {
			upixels += 4;
		}
	}
    for (size_t i = START_Y; i < START_Y + VISIBLE_Y; i++)
    {
		*upixels++ = 0; *upixels++ = 0;	*upixels++ = 0;	*upixels++ = 0;
        for (size_t j = START_X; j < START_X + VISIBLE_X; j++)
        {
            *upixels++ = 0xFF;
            *upixels++ = top(i, j).b;
            *upixels++ = top(i, j).g;
            *upixels++ = top(i, j).r;
        }
		*upixels++ = 0; *upixels++ = 0;	*upixels++ = 0;	*upixels++ = 0;
        upixels += (size_t(pitch) / sizeof(uint32_t)) - (VISIBLE_X + 2);
    }
    for (size_t i = START_Y; i < START_Y + VISIBLE_Y; i++)
    {
		*upixels++ = 0; *upixels++ = 0;	*upixels++ = 0;	*upixels++ = 0;
        for (size_t j = START_X; j < START_X + VISIBLE_X; j++)
        {
            *upixels++ = 0xFF;
            *upixels++ = bottom(i, j).b;
            *upixels++ = bottom(i, j).g;
            *upixels++ = bottom(i, j).r;
        }
		*upixels++ = 0; *upixels++ = 0;	*upixels++ = 0;	*upixels++ = 0;
        upixels += (size_t(pitch) / sizeof(uint32_t)) - (VISIBLE_X + 2);
    }
	for (size_t i = 0; i < (size_t(pitch) / sizeof(uint32_t)); i++)
	{
		if (i < VISIBLE_X + 2) {
			*upixels++ = 0; *upixels++ = 0; *upixels++ = 0; *upixels++ = 0;
		}
		else {
			upixels += 4;
		}
	}
    SDL_UnlockTexture(tex);
}

void MediaViewer::toggleFullscreen()
{
    fullscreen = !fullscreen;
    if (SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
        throw Xcept("SDL_SetWindowFullscreen: %s", SDL_GetError());
}

void MediaViewer::audioCallback(void *userdata, uint8_t *stream, int len)
{
    float *buffer = (float *)stream;
    size_t frames = size_t(len) / sizeof(float) / 2;
    boost::lockfree::spsc_queue<float> *buf = (boost::lockfree::spsc_queue<float> *)userdata;

    size_t took = buf->pop(buffer, frames * 2);
	std::fill(buffer + took, buffer + (frames * 2), 0.f);
	if (MediaViewer::IsMuted())
		for (size_t i = 0; i < frames * 2; i++)
			buffer[i] = 0.f;
}
