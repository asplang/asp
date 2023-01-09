//
// Asp compiler main.
//

#include "lexer.h"
#include "compiler.h"
#include "executable.hpp"
#include "symbol.hpp"
#include "asp.h"
#include "search-path.hpp"
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
        << " or    aspc script [spec]\n"
        << "Options:\n"
        << "-s  Silent. Don't output usual compiler information.\n"
        << "Arguments:\n"
        << "spec    = Application specification file *.aspec.\n"
        << "          If omitted, the value of the ASP_SPEC_FILE environment\n"
        << "          variable is used, or app.aspec if that is not defined.\n"
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
    // Process command line options.
    bool silent = false;
    auto optionPrefixSize = strlen(COMMAND_OPTION_PREFIX);
    for (; argc >= 2; argc--, argv++)
    {
        string arg1 = argv[1];
        string prefix = arg1.substr(0, optionPrefixSize);
        if (prefix != COMMAND_OPTION_PREFIX)
            break;
        string option = arg1.substr(optionPrefixSize);

        if (option == "?" || option == "h")
        {
            Usage();
            return 0;
        }
        else if (option == "s")
            silent = true;
        else
        {
            cerr << "Invalid option: " << arg1 << endl;
            return 1;
        }
    }

    // Obtain input file names.
    if (argc < 2 || argc > 3)
    {
        Usage();
        return 1;
    }
    string specFileName, mainModuleFileName;
    size_t mainModuleSuffixPos;
    struct InputSpec
    {
        string suffix;
        string *fileName;
        size_t *suffixPos;
    };
    struct InputSpec inputs[] =
    {
        {".aspec", &specFileName, 0},
        {".asp", &mainModuleFileName, &mainModuleSuffixPos},
    };
    for (int argi = 1; argi < argc; argi++)
    {
        string fileName(argv[argi]);
        bool accepted = false;
        for (unsigned i = 0; i < sizeof inputs / sizeof *inputs; i++)
        {
            auto &input = inputs[i];

            if (fileName.size() <= input.suffix.size())
                continue;
            auto suffixPos = fileName.size() - input.suffix.size();
            if (fileName.substr(suffixPos) == input.suffix)
            {
                if (!input.fileName->empty())
                {
                    cerr << "Multiple files of the same type specified" << endl;
                    Usage();
                    return 1;
                }

                accepted = true;
                *input.fileName = fileName;
                if (input.suffixPos != 0)
                    *input.suffixPos = suffixPos;
            }
        }
        if (!accepted)
        {
            cerr << "Unrecognized input file type" << endl;
            Usage();
            return 1;
        }
    }

    // Use default application specification if one is not given.
    if (specFileName.empty())
    {
        const char *specFileNameString = getenv("ASP_SPEC_FILE");
        if (specFileNameString != 0)
            specFileName = specFileNameString;
    }
    if (specFileName.empty())
        specFileName = "app.aspec";

    // Ensure the script has been identified.
    if (mainModuleFileName.empty())
    {
        cerr << "Script not specified" << endl;
        Usage();
        return 1;
    }

    // Split the main module file name into its constituent parts.
    auto mainModuleDirectorySeparatorPos = mainModuleFileName.find_last_of
        (FILE_NAME_SEPARATOR);
    size_t baseNamePos = mainModuleDirectorySeparatorPos == string::npos ?
        0 : mainModuleDirectorySeparatorPos + 1;
    string mainModuleBaseFileName = mainModuleFileName.substr(baseNamePos);
    string baseName = mainModuleBaseFileName.substr
        (0, mainModuleSuffixPos - baseNamePos);

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
    if (!silent)
        cout << "Using " << specFileName << endl;

    // Open output executable.
    static string executableSuffix = ".aspe";
    string executableFileName = baseName + executableSuffix;
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
    string listingFileName = baseName + listingSuffix;
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
    compiler.LoadApplicationSpec(specStream);
    compiler.AddModuleFileName(mainModuleBaseFileName);

    // Prepare to search for imported module files.
    vector<string> searchPath;
    const char *includePathString = getenv("ASP_INCLUDE");
    if (includePathString != 0)
    {
        auto includePath = SearchPath(includePathString);
        searchPath.insert
            (searchPath.end(), includePath.begin(), includePath.end());
    }
    if (searchPath.empty())
        searchPath.emplace_back();

    // Compile main module and any other modules that are imported.
    bool errorDetected = false;
    while (true)
    {
        string moduleFileName = compiler.NextModuleFileName();
        if (moduleFileName.empty())
            break;

        // Open module file.
        ifstream *moduleStream = 0;
        if (moduleFileName == mainModuleBaseFileName)
        {
            // Open specified main module file.
            auto *stream = new ifstream(mainModuleFileName);
            if (*stream)
                moduleStream = stream;
        }
        else
        {
            // Search for module file.
            for (auto iter = searchPath.begin();
                 iter != searchPath.end(); iter++)
            {
                auto directory = *iter;

                // Determine path name of module file.
                if (!directory.empty() &&
                    directory.back() != FILE_NAME_SEPARATOR)
                    directory += FILE_NAME_SEPARATOR;
                auto modulePathName = directory + moduleFileName;

                // Attempt opening the module file.
                auto *stream = new ifstream(modulePathName);
                if (*stream)
                {
                    moduleStream = stream;
                    break;
                }
                delete stream;
            }
        }
        if (moduleStream == 0)
        {
            cerr
                << "Error opening " << moduleFileName
                << ": " << strerror(errno) << endl;
            errorDetected = true;
            break;
        }
        Lexer lexer(*moduleStream);

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
        delete moduleStream;
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
    if (executableStream && !silent)
    {
        cout
            << executableFileName << ": "
            << executableByteCount << " bytes" << endl;
    }

    return 0;
}
