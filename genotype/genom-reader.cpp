#include <iostream>
#include <fstream>
#include "unistd.h"

#include "genommodule.h"
#include "preprocess.h"

using namespace std;

static void usage()
{
    cerr << "Usage: genom-reader <file.gen>" << endl;
    exit(1);
}

int main(int argc,char* argv[])
{
    if (argc < 2) usage();

    string file(argv[1]);

    std::list<std::string> args;
    // get -I arguments
    for (int i = 2; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (arg[0] == '-' && arg[1] == 'I')
            args.push_back(arg);
    }

    string i_file = preprocess(file, args);
    if (i_file.empty())
    {
        cerr << "Could not preprocess " << argv[1] << ", aborting" << endl;
        return 1;
    }
    
    GenomModule reader;
    const Registry* registry = reader.getRegistry();
    int old_count = registry -> getCount();

    if (reader.read(i_file))
    {
        // Dump type list
        cout << "Found " << registry -> getCount() - old_count << " types in " << file << ":" << endl;
        registry -> dump(std::cout, Registry::NameOnly | Registry::WithFile, "*");

        reader.dump(cout);
    }

    unlink(i_file.data());
    return 0;
}

