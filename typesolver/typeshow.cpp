#include <iostream>
#include <fstream>

#include "genomreader.h"

using namespace std;

int main(int argc,char* argv[])
{
    if (argc < 2 || argc > 3)
    {
        cerr << "Usage:\n" << argv[0] << " filename.i (/a or /A)" << "\n";
        exit(1);
    }

    const char* file = argv[1];
    GenomReader reader;
    reader.read(file);
    return 0;
}

void process_line_directive(const char *includedFile, const char *includedLineNo) {}


