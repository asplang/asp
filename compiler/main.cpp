//
// Asp compiler main.
//

#include "lexer.h"
#include "compiler.h"
#include "executable.hpp"
#include "symbol.hpp"
#include "asp.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>

// Lemon parser.
extern "C" {
void *ParseAlloc(void *(*malloc)(size_t), Compiler *);
void Parse(void *parser, int yymajor, Token *yyminor);
void ParseFree(void *parser, void (*free)(void *));
void ParseTrace(FILE *, const char *prefix);
}

using namespace std;

static void Usage()
{
    cerr
        << "Usage: aspc [spec] script\n"
        << "Arguments:\n"
        << "spec    = Application specification file *.aspec."
        << " Default is app.aspec.\n"
        << "script  = Script file *.asp.\n";
}

static int main1(int argc, char **argv);
int main(int argc, char **argv)
{
    try
    {
        return main1(argc, argv);
    }
    catch (const string &e)
    {
        cerr << "Error: " << e << endl;
    }
    catch (...)
    {
        cerr << "Unknown error" << endl;
    }
    return EXIT_FAILURE;
}
static int main1(int argc, char **argv)
{
    // Obtain input file names.
    if (argc < 2 || argc > 3)
    {
        Usage();
        return 1;
    }
    string specFileName, mainModuleFileName;
    size_t specSuffixIndex, mainModuleSuffixIndex;
    struct InputSpec
    {
        string suffix;
        string *fileName;
        size_t *suffixIndex;
    };
    struct InputSpec inputs[] =
    {
        {".aspec", &specFileName, &specSuffixIndex},
        {".asp", &mainModuleFileName, &mainModuleSuffixIndex},
    };
    for (int argi = 1; argi < argc; argi++)
    {
        string fileName(argv[argi]);
        for (unsigned i = 0; i < sizeof inputs / sizeof *inputs; i++)
        {
            auto &input = inputs[i];

            if (fileName.size() <= input.suffix.size())
                continue;
            *input.suffixIndex = fileName.size() - input.suffix.size();
            if (fileName.substr(*input.suffixIndex) == input.suffix)
            {
                if (!input.fileName->empty())
                {
                    Usage();
                    return 1;
                }
                *input.fileName = fileName;
            }
        }
    }

    // Use default application specification if one is not given.
    if (specFileName.empty())
        specFileName = "app.aspec";

    // Ensure the script has been identified.
    if (mainModuleFileName.empty())
    {
        Usage();
        return 1;
    }
    string baseFileName = mainModuleFileName.substr(0, mainModuleSuffixIndex);

    // Open application specification.
    ifstream specStream(specFileName, ios::binary);
    if (!specStream)
    {
        cerr
            << "Error opening " << specFileName
            << ": " << strerror(errno) << endl;
        Usage();
        return 2;
    }

    // Open output executable.
    static string executableSuffix = ".aspe";
    string executableFileName = baseFileName + executableSuffix;
    ofstream executableStream(executableFileName, ios::binary);
    if (!executableStream)
    {
        cerr
            << "Error creating " << executableFileName
            << ": " << strerror(errno) << endl;
        Usage();
        return 2;
    }

    // Open output listing.
    static string listingSuffix = ".lst";
    string listingFileName = baseFileName + listingSuffix;
    ofstream listingStream(listingFileName);
    if (!listingStream)
    {
        cerr
            << "Error creating " << listingFileName
            << ": " << strerror(errno) << endl;
        Usage();
        return 2;
    }

    // Predefine symbols for main module and application functions.
    SymbolTable symbolTable;
    Executable executable(symbolTable);
    Compiler compiler(cerr, symbolTable, executable);
    compiler.AddModuleFileName(mainModuleFileName);
    compiler.PredefineSymbols(specStream);

    // Compile main module and any other modules that are imported.
    bool errorDetected = false;
    while (true)
    {
        string moduleFileName = compiler.NextModuleFileName();
        if (moduleFileName.empty())
            break;
        ifstream moduleStream(moduleFileName);
        if (!moduleStream)
        {
            cerr
                << "Error opening " << moduleFileName
                << ": " << strerror(errno) << endl;
            errorDetected = true;
            break;
        }
        Lexer lexer(moduleStream);

        #ifdef ASP_COMPILER_DEBUG
        cout << "Parsing module " << moduleFileName << "..." << endl;
        ParseTrace(stdout, "Trace: ");
        #endif

        void *parser = ParseAlloc(malloc, &compiler);

        Token *token;
        do
        {
            token = lexer.Next();
            if (token->type == -1)
            {
                cerr << "Error: BAD token: '" << token->s << '\'' << endl;
                delete token;
                errorDetected = true;
                break;
            }

            if (token->type == TOKEN_UNEXPECTED_INDENT)
                errorDetected = true;
            else if (token->type == TOKEN_MISSING_INDENT)
                errorDetected = true;
            else if (token->type == TOKEN_MISMATCHED_UNINDENT)
                errorDetected = true;
            else if (token->type == TOKEN_INCONSISTENT_WS)
                errorDetected = true;

            Parse(parser, token->type, token);
            if (compiler.ErrorCount() > 0)
                errorDetected = true;

        } while (!errorDetected && token->type != 0);

        ParseFree(parser, free);
    }

    compiler.Finalize();

    if (errorDetected)
    {
        cerr << "Ended in ERROR" << endl;
        return 4;
    }

    // Write the code.
    executable.Write(executableStream);
    auto executableByteCount = executableStream.tellp();
    executableStream.close();
    if (!executableStream)
    {
        cerr
            << "Error writing " << executableFileName
            << ": " << strerror(errno) << endl;
        remove(executableFileName.c_str());
    }

    // Write the listing.
    executable.WriteListing(listingStream);
    listingStream.close();
    if (!listingStream)
    {
        cerr
            << "Error writing " << listingFileName
            << ": " << strerror(errno) << endl;
    }

    // Indicate any error writing the code.
    if (!executableStream)
        return 5;

    // Write statistics.
    if (executableStream)
    {
        cout
            << executableFileName << ": "
            << executableByteCount << " bytes" << endl;
    }

    return 0;
}
