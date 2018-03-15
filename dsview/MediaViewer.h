#pragma once
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>
#include <glm/ext.hpp>

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
        void toggleFullscreen();

        void calcMatrices(glm::mat4& upperScreenMat, glm::mat4& lowerScreenMat, float& upperScreenAlpha, float &lowerScreenAlpha);

        static void audioCallback(void *userdata, uint8_t *stream, int len);

        SDL_Window *win;

        // textures

        enum {
            SCREEN_TEX_TOP = 0,
            SCREEN_TEX_BOT = 1,
            NUM_TEXTURES = 2
        };

        GLuint screen_textures[NUM_TEXTURES];
        GLuint rect_vao;
        GLuint rect_vbo_vertices;
        GLuint rect_vbo_indices;

        // shader

        GLuint shader_program;

        struct {
            GLuint vpos;
            GLuint texcoord;
        } vertex_attrib_locations;

        struct {
            GLuint alpha;
            GLuint tex;
            GLuint MVP;
        } uniform_locations;

        // misc

        SDL_GLContext glcon;

        // animation stuff

        enum {
            CUR_STATE = 0, OLD_STATE = 1, NUM_STATES = 2
        };

        enum {
            DISP_MODE_NDS_TOP_ON_BOT,
            DISP_MODE_NDS_RIGHT,
            DISP_MODE_NDS_BOT_ON_TOP,
            DISP_MODE_NDS_LEFT,
            DISP_MODE_GBA_UPPER,
            DISP_MODE_GBA_LOWER,
        } screen_state[2];

        enum {
            DISP_MODE_R0,
            DISP_MODE_R90,
            DISP_MODE_R180,
            DISP_MODE_R270
        } rot_state[2];
        float anim_state_t;

        bool fullscreen;
		static bool mute;

		int win_h, win_w;

        Image& top;
        Image& bottom;

		boost::lockfree::spsc_queue<float>& audio_buffer;
        
        SDL_AudioDeviceID audioDeviceID;
};
