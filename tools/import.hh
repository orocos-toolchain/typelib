#ifndef IMPORT_H
#define IMPORT_H

#include "mode.hh"

class Import : public Mode
{
public:
    Import();
    
    virtual bool apply(int argc, char* const argv[]);
    virtual void help(std::ostream& stream) const;
};

#endif

