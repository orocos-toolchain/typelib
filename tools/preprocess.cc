#include "preprocess.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include <iostream>
using namespace std;

/**
 ** Appel du preprocesseur C 
 **/

#ifndef STDINCPP
# define STDINCPP "gcc -E -"
#endif /* STDINCPP */

typedef std::list<std::string> StringList;
typedef std::vector<std::string> StringVector;

static StringVector split(const string& str, const char* char_set = " \t")
{
    StringVector ret;
    
    size_t length(str.length());
    size_t begin = 0;
    while (begin < length)
    {
        size_t end = str.find_first_of(char_set, begin);
        if (end > length)
            end = length;

        ret.push_back(str.substr(begin, end - begin));
        begin = end + 1;
    }

    return ret;
}

std::string preprocess(const string& fichier, const StringList& options)
{
    char tmpName[17] = "/tmp/genomXXXXXX";

    /* open input and output files */
    int in = open(fichier.c_str(), O_RDONLY, 0);
    if (in < 0) {
        cerr << "Cannot open " << fichier << " for reading: " << strerror(errno) << endl;
        return string();
    }

    /* use a safe temporary file */
    int out = mkstemp(tmpName);
    if (out < 0) {
        cerr << "Cannot open temporary file " << tmpName << "for writing: " << strerror(errno) << endl;
        return string();
    }

    /* build the argv array: split cpp into argvs and copy options */
    StringVector argv = split(STDINCPP);
    if (argv.empty()) 
    {
        cerr << "No C preprocessor specified, aborting" << endl;
        return string();
    }
    argv.insert(argv.end(), options.begin(), options.end());

    // Convert argv into a C-style array (null terminated)
    int    argv_size = argv.size();
    char** c_argv = new char*[argv_size + 1];
    for (int i = 0; i < argv_size; ++i)
        c_argv[i] = const_cast<char *>(argv[i].c_str());
    c_argv[argv_size] = 0;

    /* cpp output goes to stdout */
    pid_t pid = fork();
    if (pid == 0) {
        /* read stdin from nomFic */
        if (dup2(in, fileno(stdin)) < 0)
            cerr << "Cannot redirect cpp input: " << strerror(errno) << endl;
        else if (dup2(out, fileno(stdout)) < 0)
            cerr << "Cannot redirect cpp output: " << strerror(errno) << endl;
        else if (execvp(c_argv[0], c_argv) == -1)
        {
            cerr << "Error running " << c_argv[0] << ": " << strerror(errno) << endl;
            exit(1);
        }
    }

    // Cleanup
    // If we are still in the child, pid == 0 and it means
    // that one of the calls to dup2 or execvp failed
    delete[] c_argv;
    close(in);
    close(out);
    if (pid == 0) return string();

    int status;
    wait( &status );
    if (! WIFEXITED(status) || WEXITSTATUS(status) != 0) 
    {
        cerr << "C preprocessor failure" << endl;
        return string();
    }

    return tmpName;
} /* callCpp */

