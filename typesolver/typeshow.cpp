#include "CPPLexer.hpp"
#include "typesolver.h"
#include <iostream>
#include <fstream>
#include "type.h"


ANTLR_USING_NAMESPACE(std)
ANTLR_USING_NAMESPACE(antlr)

int main(int argc,char* argv[])
{
    // Adapt the following if block to the requirements of your application
    if (argc < 2 || argc > 3)
    {
        cerr << "Usage:\n" << argv[0] << " filename.i (/a or /A)" << "\n";
        exit(1);
    }

    const char* file = argv[1];
    try {
        ifstream s(file);
        if (!s)
        {
            cerr << "Input stream could not be opened for " << file << "\n";
            exit(1);
        }

        // Create a scanner that reads from the input stream
        CPPLexer lexer(s);
        lexer.setFilename(file);

        // Create subclass of parser for MYCODE code that reads from scanner
        TypeSolver typefind(lexer);
        typefind.init();
        typefind.translation_unit();
    }
    catch (ANTLRException& e) 
    {
        cerr << "parser exception: " << e.toString() << endl;
        //		e.printStackTrace();   // so we can get stack trace		
    }
    catch (TypeBuilder::NotFound& e)
    {
        cerr << "Type solver exception: " << e.toString() << endl;
    }
    printf("\nParse ended\n");
    return 0;
}

void process_line_directive(const char *includedFile, const char *includedLineNo) {}


