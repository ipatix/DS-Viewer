#pragma once

#include <exception>
#include <string>
#include <boost/format.hpp>

#include "Util.h"

class Xcept : public std::exception
{
    public:
        template <typename... TArgs>
            Xcept(const char *fmt, TArgs&&... args) {
                using namespace boost::io;
                boost::format message(fmt);
                msg = FormatStringRecurse(message, std::forward<TArgs>(args)...);
            }
        ~Xcept();

        const char *what() const throw() override;
    private:
        std::string msg;
};
