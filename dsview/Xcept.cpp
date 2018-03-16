#include "Xcept.h"

using namespace std;

Xcept::~Xcept()
{
}

const char *Xcept::what() const throw()
{
    return msg.c_str();
}
