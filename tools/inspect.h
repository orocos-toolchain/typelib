#ifndef TYPELIB_INSPECT_H
#define TYPELIB_INSPECT_H

#include "mode.h"

class Inspect : public Mode
{
public:
    Inspect();
    
    virtual bool apply(int argc, char* const argv[]);
    virtual void help(std::ostream& out) const;
};

#endif

