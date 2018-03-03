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
