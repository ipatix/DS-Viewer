#include <cassert>
#include <algorithm>
#include <iostream>

#include "MediaViewer.h"
#include "Config.h"
#include "Xcept.h"
#include "shaders.h"

bool MediaViewer::mute = false;

MediaViewer::MediaViewer(Image& _top, Image& _bottom, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : fullscreen(false), top(_top), bottom(_bottom), audio_buffer(_audio_buffer)
{
    // transformations
    for (int i = 0; i < NUM_STATES; i++) {
        rot_state[i] = DISP_MODE_R0;
        screen_state[i] = DISP_MODE_NDS_TOP_ON_BOT;
    }
    anim_state_t = 0.0f;

    // Video
    assert(top.Height() == bottom.Height());
    assert(top.Width() == bottom.Width());
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        throw Xcept("SDL_Init Error: %s", SDL_GetError());
    win = SDL_CreateWindow("dsview V0.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NDS_W, NDS_HH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!win)
        throw Xcept("SDL_CreateWindow Error: %s", SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetSwapInterval(1);
    glcon = SDL_GL_CreateContext(win);
    if (!glcon)
        throw Xcept("SDL_GL_CreateContext Error: %s", SDL_GetError());
    if (SDL_GL_MakeCurrent(win, glcon))
        throw Xcept("SDL_GL_MakeCurrent Error: %s", SDL_GetError());
    glewInit();

    // misc opengl

    GL_ERR_CHK();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // textures
    GL_ERR_CHK();

    float borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glGenTextures(NUM_TEXTURES, screen_textures);
    glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_TOP]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, NDS_W, NDS_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_BOT]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, NDS_W, NDS_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // shaders
    GL_ERR_CHK();

    GLint status;
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertex_shader)
        throw Xcept("glCreateShader ERROR");
    static const GLchar *vs_sources[1] = { Shaders::default_vertex_shader };
    static const GLint vs_lengths[1] = { static_cast<GLint>(strlen(Shaders::default_vertex_shader)) };
    glShaderSource(vertex_shader, 1, vs_sources, vs_lengths);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogSize;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &infoLogSize);
        std::vector<GLchar> infoLog(static_cast<size_t>(infoLogSize));
        glGetShaderInfoLog(vertex_shader, infoLogSize, nullptr, infoLog.data());
        throw Xcept("Shader Compiler Error:\n\n%s", infoLog.data());
    }
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragment_shader)
        throw Xcept("glCreateShader ERROR");
    static const GLchar *fs_sources[1] = { Shaders::default_fragment_shader };
    static const GLint fs_lengths[1] = { static_cast<GLint>(strlen(Shaders::default_fragment_shader)) };
    glShaderSource(fragment_shader, 1, fs_sources, fs_lengths);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogSize;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &infoLogSize);
        std::vector<GLchar> infoLog(static_cast<size_t>(infoLogSize));
        glGetShaderInfoLog(fragment_shader, infoLogSize, nullptr, infoLog.data());
        throw Xcept("Shader Compiler Error:\n\n%s", infoLog.data());
    }
    shader_program = glCreateProgram();
    if (!shader_program)
        throw Xcept("glCreateProgram ERROR");
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint infoLogSize;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &infoLogSize);
        std::vector<GLchar> infoLog(static_cast<size_t>(infoLogSize));
        glGetProgramInfoLog(shader_program, infoLogSize, nullptr, infoLog.data());
        throw Xcept("Shader Linking Error: %s", infoLog.data());
    }
    glDetachShader(shader_program, vertex_shader);
    glDetachShader(shader_program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    uniform_locations.alpha = glGetUniformLocation(shader_program, "alpha");
    uniform_locations.tex = glGetUniformLocation(shader_program, "tex");
    uniform_locations.MVP = glGetUniformLocation(shader_program, "MVP");
    vertex_attrib_locations.vpos = glGetAttribLocation(shader_program, "vpos");
    vertex_attrib_locations.texcoord = glGetAttribLocation(shader_program, "texcoord");

    // VBOs
    GL_ERR_CHK();

    glGenVertexArrays(1, &rect_vao);
    glGenBuffers(1, &rect_vbo_vertices);
    glGenBuffers(1, &rect_vbo_indices);
    glBindVertexArray(rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, rect_vbo_vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect_vbo_indices);

    // texture coords are flipped upside down because textures are indexed from top to bottom
    static const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // lower left vertex
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // lower right vertex
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // upper right vertex
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // upper left vertex
    };
    static const unsigned short indices[] = {
        0, 1, 2, 0, 2, 3
    };

    glVertexAttribPointer(vertex_attrib_locations.vpos, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(vertex_attrib_locations.vpos);
    glVertexAttribPointer(vertex_attrib_locations.texcoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)12);
    glEnableVertexAttribArray(vertex_attrib_locations.texcoord);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // other stuff
    GL_ERR_CHK();

    SDL_GetWindowSize(win, &win_w, &win_h);
    glViewport(0, 0, win_w, win_h);
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
    glDeleteBuffers(1, &rect_vbo_indices);
    glDeleteBuffers(1, &rect_vbo_vertices);
    glDeleteVertexArrays(1, &rect_vao);
    glDeleteProgram(shader_program);

    glDeleteTextures(NUM_TEXTURES, screen_textures);

    SDL_GL_DeleteContext(glcon);
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
                glViewport(0, 0, win_w, win_h);
                break;
            case SDL_WINDOWEVENT_CLOSE:
                return false;
            }
            break;
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
            if (sev.key.state != SDL_PRESSED)
                break;
            if (sev.key.keysym.sym == SDLK_f) {
                toggleFullscreen();
            }
            else if (sev.key.keysym.sym == SDLK_q) {
                return false;
            }
            else if (sev.key.keysym.sym == SDLK_m) {
                mute = !mute;
            }
            else if (sev.key.keysym.sym == SDLK_w) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (screen_state[CUR_STATE]) {
                case DISP_MODE_NDS_TOP_ON_BOT:
                case DISP_MODE_NDS_RIGHT:
                case DISP_MODE_NDS_LEFT:
                    screen_state[CUR_STATE] = DISP_MODE_GBA_UPPER;
                    break;
                case DISP_MODE_NDS_BOT_ON_TOP:
                    screen_state[CUR_STATE] = DISP_MODE_GBA_LOWER;
                    break;
                case DISP_MODE_GBA_UPPER:
                    break;
                case DISP_MODE_GBA_LOWER:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_TOP_ON_BOT;
                    break;
                }
            }
            else if (sev.key.keysym.sym == SDLK_s) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (screen_state[CUR_STATE]) {
                case DISP_MODE_NDS_TOP_ON_BOT:
                case DISP_MODE_NDS_RIGHT:
                case DISP_MODE_NDS_LEFT:
                    screen_state[CUR_STATE] = DISP_MODE_GBA_LOWER;
                    break;
                case DISP_MODE_NDS_BOT_ON_TOP:
                    screen_state[CUR_STATE] = DISP_MODE_GBA_UPPER;
                    break;
                case DISP_MODE_GBA_UPPER:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_TOP_ON_BOT;
                    break;
                case DISP_MODE_GBA_LOWER:
                    break;
                }
            }
            else if (sev.key.keysym.sym == SDLK_a) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (rot_state[CUR_STATE]) {
                case DISP_MODE_R0:
                    rot_state[CUR_STATE] = DISP_MODE_R270;
                    break;
                case DISP_MODE_R90:
                    rot_state[CUR_STATE] = DISP_MODE_R0;
                    break;
                case DISP_MODE_R180:
                    rot_state[CUR_STATE] = DISP_MODE_R90;
                    break;
                case DISP_MODE_R270:
                    rot_state[CUR_STATE] = DISP_MODE_R180;
                    break;
                }
            }
            else if (sev.key.keysym.sym == SDLK_d) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (rot_state[CUR_STATE]) {
                case DISP_MODE_R0:
                    rot_state[CUR_STATE] = DISP_MODE_R90;
                    break;
                case DISP_MODE_R90:
                    rot_state[CUR_STATE] = DISP_MODE_R180;
                    break;
                case DISP_MODE_R180:
                    rot_state[CUR_STATE] = DISP_MODE_R270;
                    break;
                case DISP_MODE_R270:
                    rot_state[CUR_STATE] = DISP_MODE_R0;
                    break;
                }
            }
            else if (sev.key.keysym.sym == SDLK_g) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (screen_state[CUR_STATE]) {
                case DISP_MODE_NDS_TOP_ON_BOT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_LEFT;
                    break;
                case DISP_MODE_NDS_RIGHT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_TOP_ON_BOT;
                    break;
                case DISP_MODE_NDS_BOT_ON_TOP:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_RIGHT;
                    break;
                case DISP_MODE_NDS_LEFT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_BOT_ON_TOP;
                    break;
                case DISP_MODE_GBA_UPPER:
                case DISP_MODE_GBA_LOWER:
                    break;
                }
            }
            else if (sev.key.keysym.sym == SDLK_h) {
                if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE])
                    break;
                switch (screen_state[CUR_STATE]) {
                case DISP_MODE_NDS_TOP_ON_BOT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_RIGHT;
                    break;
                case DISP_MODE_NDS_RIGHT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_BOT_ON_TOP;
                    break;
                case DISP_MODE_NDS_BOT_ON_TOP:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_LEFT;
                    break;
                case DISP_MODE_NDS_LEFT:
                    screen_state[CUR_STATE] = DISP_MODE_NDS_TOP_ON_BOT;
                    break;
                case DISP_MODE_GBA_UPPER:
                case DISP_MODE_GBA_LOWER:
                    break;
                }
            }
            break;
        }
    }
    GL_ERR_CHK();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!blank) {
        // upload new texture data
        glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_TOP]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_W, NDS_H, GL_RGB, GL_UNSIGNED_BYTE, top.getData());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_BOT]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_W, NDS_H, GL_RGB, GL_UNSIGNED_BYTE, bottom.getData());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    GL_ERR_CHK();

    float upperScreenAlpha, lowerScreenAlpha;
    glm::mat4 upperScreenMat, lowerScreenMat;

    calcMatrices(upperScreenMat, lowerScreenMat, upperScreenAlpha, lowerScreenAlpha);

    glUseProgram(shader_program);
    glBindVertexArray(rect_vao);
    // draw upper screen
    glUniform1f(uniform_locations.alpha, upperScreenAlpha);
    glUniformMatrix4fv(uniform_locations.MVP, 1, GL_FALSE, glm::value_ptr(upperScreenMat));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_TOP]);
    glUniform1i(uniform_locations.tex, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    // draw lower screen
    glUniform1f(uniform_locations.alpha, lowerScreenAlpha);
    glUniformMatrix4fv(uniform_locations.MVP, 1, GL_FALSE, glm::value_ptr(lowerScreenMat));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_textures[SCREEN_TEX_BOT]);
    glUniform1i(uniform_locations.tex, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);


    glUseProgram(0);

    // TODO render things
    
    SDL_GL_SwapWindow(win);

    return true;
}

void MediaViewer::toggleFullscreen()
{
    fullscreen = !fullscreen;
    if (SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
        throw Xcept("SDL_SetWindowFullscreen: %s", SDL_GetError());
}

void MediaViewer::calcMatrices(glm::mat4& upperScreenMat, glm::mat4& lowerScreenMat, float& upperScreenAlpha, float& lowerScreenAlpha) {
    struct {
        float rot_state_angle;
        float rot_state_width;
        float rot_state_height;

        glm::vec3 top_screen_offset;
        glm::vec3 bot_screen_offset;

        float top_screen_alpha;
        float bot_screen_alpha;
    } states[2];

    auto tweenfunc = [](float t) {
        return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    };

    // calc screen state

    for (int i = 0; i < NUM_STATES; i++) {
        switch (screen_state[i]) {
        case DISP_MODE_NDS_TOP_ON_BOT:
            states[i].top_screen_offset = glm::vec3(0.0f, 1.0f, 0.0f);
            states[i].bot_screen_offset = glm::vec3(0.0f, -1.0f, 0.0f);
            states[i].top_screen_alpha = 1.0f;
            states[i].bot_screen_alpha = 1.0f;
            break;
        case DISP_MODE_NDS_RIGHT:
            states[i].top_screen_offset = glm::vec3(1.0f, 0.0f, 0.0f);
            states[i].bot_screen_offset = glm::vec3(-1.0f, 0.0f, 0.0f);
            states[i].top_screen_alpha = 1.0f;
            states[i].bot_screen_alpha = 1.0f;
            break;
        case DISP_MODE_NDS_BOT_ON_TOP:
            states[i].top_screen_offset = glm::vec3(0.0f, -1.0f, 0.0f);
            states[i].bot_screen_offset = glm::vec3(0.0f, 1.0f, 0.0f);
            states[i].top_screen_alpha = 1.0f;
            states[i].bot_screen_alpha = 1.0f;
            break;
        case DISP_MODE_NDS_LEFT:
            states[i].top_screen_offset = glm::vec3(-1.0f, 0.0f, 0.0f);
            states[i].bot_screen_offset = glm::vec3(1.0f, 0.0f, 0.0f);
            states[i].top_screen_alpha = 1.0f;
            states[i].bot_screen_alpha = 1.0f;
            break;
        case DISP_MODE_GBA_UPPER:
            states[i].top_screen_offset = glm::vec3(0.0f, 0.0f, -0.5f);
            states[i].bot_screen_offset = glm::vec3(0.0f, 0.0f, 0.5f);
            states[i].top_screen_alpha = 1.0f;
            states[i].bot_screen_alpha = 0.0f;
            break;
        case DISP_MODE_GBA_LOWER:
            states[i].top_screen_offset = glm::vec3(0.0f, 0.0f, 0.5f);
            states[i].bot_screen_offset = glm::vec3(0.0f, 0.0f, -0.5f);
            states[i].top_screen_alpha = 0.0f;
            states[i].bot_screen_alpha = 1.0f;
            break;
        }
    }

    // calc rotation

    for (int i = 0; i < NUM_STATES; i++) {
        switch (rot_state[i]) {
        case DISP_MODE_R0:
            if (i == CUR_STATE && rot_state[OLD_STATE] == DISP_MODE_R270)
                states[i].rot_state_angle = 2.0f * M_PI;
            else
                states[i].rot_state_angle = 0.0;

            switch (screen_state[i]) {
            case DISP_MODE_NDS_TOP_ON_BOT:
            case DISP_MODE_NDS_BOT_ON_TOP:
                states[i].rot_state_width = float(NDS_W) / float(NDS_H);
                states[i].rot_state_height = 2.0f;
                break;
            case DISP_MODE_NDS_RIGHT:
            case DISP_MODE_NDS_LEFT:
                states[i].rot_state_width = 2.0f * float(NDS_W) / float(NDS_H);
                states[i].rot_state_height = 1.0f;
                break;
            case DISP_MODE_GBA_UPPER:
            case DISP_MODE_GBA_LOWER:
                states[i].rot_state_width = float(GBA_W) / float(NDS_H);
                states[i].rot_state_height = float(GBA_H) / float(NDS_H);
                break;
            }
            break;
        case DISP_MODE_R90:
            states[i].rot_state_angle = 0.5f * M_PI;

            switch (screen_state[i]) {
            case DISP_MODE_NDS_TOP_ON_BOT:
            case DISP_MODE_NDS_BOT_ON_TOP:
                states[i].rot_state_width = 2.0f;
                states[i].rot_state_height = float(NDS_W) / float(NDS_H);
                break;
            case DISP_MODE_NDS_RIGHT:
            case DISP_MODE_NDS_LEFT:
                states[i].rot_state_width = 1.0f;
                states[i].rot_state_height = 2.0f * float(NDS_W) / float(NDS_H);
                break;
            case DISP_MODE_GBA_UPPER:
            case DISP_MODE_GBA_LOWER:
                states[i].rot_state_width = float(GBA_H) / float(NDS_H);
                states[i].rot_state_height = float(GBA_W) / float(NDS_H);
                break;
            }
            break;
        case DISP_MODE_R180:
            states[i].rot_state_angle = M_PI;

            switch (screen_state[i]) {
            case DISP_MODE_NDS_TOP_ON_BOT:
            case DISP_MODE_NDS_BOT_ON_TOP:
                states[i].rot_state_width = float(NDS_W) / float(NDS_H);
                states[i].rot_state_height = 2.0f;
                break;
            case DISP_MODE_NDS_RIGHT:
            case DISP_MODE_NDS_LEFT:
                states[i].rot_state_width = 2.0f * float(NDS_W) / float(NDS_H);
                states[i].rot_state_height = 1.0f;
                break;
            case DISP_MODE_GBA_UPPER:
            case DISP_MODE_GBA_LOWER:
                states[i].rot_state_width = float(GBA_W) / float(NDS_H);
                states[i].rot_state_height = float(GBA_H) / float(NDS_H);
                break;
            }
            break;
        case DISP_MODE_R270:
            if (i == CUR_STATE && rot_state[OLD_STATE] == DISP_MODE_R0)
                states[i].rot_state_angle = -0.5f * M_PI;
            else
                states[i].rot_state_angle = 1.5f * M_PI;

            switch (screen_state[i]) {
            case DISP_MODE_NDS_TOP_ON_BOT:
            case DISP_MODE_NDS_BOT_ON_TOP:
                states[i].rot_state_width = 2.0f;
                states[i].rot_state_height = float(NDS_W) / float(NDS_H);
                break;
            case DISP_MODE_NDS_RIGHT:
            case DISP_MODE_NDS_LEFT:
                states[i].rot_state_width = 1.0f;
                states[i].rot_state_height = 2.0f * float(NDS_W) / float(NDS_H);
                break;
            case DISP_MODE_GBA_UPPER:
            case DISP_MODE_GBA_LOWER:
                states[i].rot_state_width = float(GBA_H) / float(NDS_H);
                states[i].rot_state_height = float(GBA_W) / float(NDS_H);
                break;
            }
            break;
        }
    }

    float t = tweenfunc(anim_state_t);

    float rot_state_angle = glm::mix(states[OLD_STATE].rot_state_angle, states[CUR_STATE].rot_state_angle, t);
    float rot_state_width = glm::mix(states[OLD_STATE].rot_state_width, states[CUR_STATE].rot_state_width, t);
    float rot_state_height = glm::mix(states[OLD_STATE].rot_state_height, states[CUR_STATE].rot_state_height, t);
    glm::vec3 top_screen_offset = glm::mix(states[OLD_STATE].top_screen_offset, states[CUR_STATE].top_screen_offset, t);
    glm::vec3 bot_screen_offset = glm::mix(states[OLD_STATE].bot_screen_offset, states[CUR_STATE].bot_screen_offset, t);
    upperScreenAlpha = glm::mix(states[OLD_STATE].top_screen_alpha, states[CUR_STATE].top_screen_alpha, t);
    lowerScreenAlpha = glm::mix(states[OLD_STATE].bot_screen_alpha, states[CUR_STATE].bot_screen_alpha, t);

    // transformations are in reverse order
    float windowAspectRatio = float(win_w) / float(win_h);
    float consoleAspectRatio = float(rot_state_width) / float(rot_state_height);
    if (windowAspectRatio > consoleAspectRatio) {
        //upperScreenMat = glm::scale(upperScreenMat, glm::vec3())
        upperScreenMat = glm::scale(upperScreenMat, glm::vec3(float(win_h) / float(win_w), 1.0f, 1.0f));
        upperScreenMat = glm::scale(upperScreenMat, glm::vec3(1.0f / rot_state_height, 1.0f / rot_state_height, 1.0f));
    } else {
        upperScreenMat = glm::scale(upperScreenMat, glm::vec3(1.0f, float(win_w) / float(win_h), 1.0f));
        upperScreenMat = glm::scale(upperScreenMat, glm::vec3(1.0f / rot_state_width, 1.0f / rot_state_width, 1.0f));
    }
    upperScreenMat = glm::rotate(upperScreenMat, rot_state_angle, glm::vec3(0.0f, 0.0f, -1.0f));
    upperScreenMat = glm::scale(upperScreenMat, glm::vec3(float(NDS_W) / float(NDS_H), 1.0f, 1.0f));
    upperScreenMat = glm::translate(upperScreenMat, top_screen_offset);

    if (windowAspectRatio > consoleAspectRatio) {
        lowerScreenMat = glm::scale(lowerScreenMat, glm::vec3(float(win_h) / float(win_w), 1.0f, 1.0f));
        lowerScreenMat = glm::scale(lowerScreenMat, glm::vec3(1.0f / rot_state_height, 1.0f / rot_state_height, 1.0f));
    } else {
        lowerScreenMat = glm::scale(lowerScreenMat, glm::vec3(1.0f, float(win_w) / float(win_h), 1.0f));
        lowerScreenMat = glm::scale(lowerScreenMat, glm::vec3(1.0f / rot_state_width, 1.0f / rot_state_width, 1.0f));
    }
    lowerScreenMat = glm::rotate(lowerScreenMat, rot_state_angle, glm::vec3(0.0f, 0.0f, -1.0f));
    lowerScreenMat = glm::scale(lowerScreenMat, glm::vec3(float(NDS_W) / float(NDS_H), 1.0f, 1.0f));
    lowerScreenMat = glm::translate(lowerScreenMat, bot_screen_offset);

    if (rot_state[CUR_STATE] != rot_state[OLD_STATE] || screen_state[CUR_STATE] != screen_state[OLD_STATE]) {
        anim_state_t += 0.05f;
        if (anim_state_t >= 1.0f) {
            rot_state[OLD_STATE] = rot_state[CUR_STATE];
            screen_state[OLD_STATE] = screen_state[CUR_STATE];
            anim_state_t = 0.0f;
        }
    }
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
