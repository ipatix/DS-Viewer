#pragma once

#include <stdexcept>
#include <boost/format.hpp>


inline static std::string FormatStringRecurse(boost::format& message)
{
    return message.str();
}

template <typename TValue, typename... TArgs>
std::string FormatStringRecurse(boost::format& message, TValue&& arg, TArgs&&... args)
{
    message % std::forward<TValue>(arg);
    return FormatStringRecurse(message, std::forward<TArgs>(args)...);
}

template <typename... TArgs>
std::string FormatString(const char* fmt, TArgs&&... args)
{
    using namespace boost::io;
    boost::format message(fmt);
    return FormatStringRecurse(message, std::forward<TArgs>(args)...);
}

#define GL_ERR_CHK() do {\
    GLuint err = glGetError();\
    if (err != GL_NO_ERROR)\
        throw Xcept("OpenGL Error in file %s:%d with code %d", __FILE__, __LINE__, err);\
    } while (0)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct stereo_sample {
    stereo_sample() {
        l = r = 0.0f;
    }
    float l;
    float r;
};