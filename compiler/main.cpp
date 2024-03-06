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
#include <cerrno>

#if !defined ASP_COMPILER_VERSION_MAJOR || \
    !defined ASP_COMPILER_VERSION_MINOR || \
    !defined ASP_COMPILER_VERSION_PATCH || \
    !defined ASP_COMPILER_VERSION_TWEAK
#error ASP_COMPILER_VERSION_* macros undefined
#endif
#ifndef COMMAND_OPTION_PREFIXES
#error COMMAND_OPTION_PREFIXES macro undefined
#endif
#ifndef FILE_NAME_SEPARATORS
#error FILE_NAME_SEPARATORS macro undefined
#endif

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
        << "Usage:      aspc [OPTION]... ["
        << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << "] [SPEC] SCRIPT\n"
        << " or         aspc [OPTION]... ["
        << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << "] SCRIPT [SPEC]\n"
        << "\n"
        << "Compile the Asp script source file SCRIPT (*.asp)."
        << " The application specification\n"
        << "file (*.aspec) may be given as SPEC."
        << " If omitted, an app.aspec file in the same\n"
        << "directory as the source file is used if present."
        << " Otherwise, the value of the\n"
        << "ASP_SPEC_FILE environment variable is used if defined."
        << " If the application\n"
        << "specification file is not specified in any of these ways,"
        << " the command ends\n"
        << "in error.\n"
        << "\n"
        << "Use " << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << " before the first argument if it starts with an option prefix.\n"
        << "\n"
        << "Options";
    if (strlen(COMMAND_OPTION_PREFIXES) > 1)
    {
        cerr << " (may be prefixed by";
        for (unsigned i = 1; i < strlen(COMMAND_OPTION_PREFIXES); i++)
        {
            if (i != 1)
            {
                if (i == strlen(COMMAND_OPTION_PREFIXES) - 1)
                    cerr << " or";
                else
                    cerr << ',';
            }
            cerr << ' ' << COMMAND_OPTION_PREFIXES[i];
        }
        cerr << " instead of " << COMMAND_OPTION_PREFIXES[0] << ')';
    }
    cerr
        << ":\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "o FILE     Write outputs to FILE.* instead of basing file names"
        << " on the SCRIPT\n"
        << "            file name. If FILE ends with .aspe, its base name is"
        << " used instead.\n"
        << "            If FILE ends with " << FILE_NAME_SEPARATORS[0]
        << ", SCRIPT.* will be written in the directory\n"
        << "            given by FILE. In this case, the directory must"
        << " already exist.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "s          Silent. Don't output usual compiler information.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "v          Print version information and exit.\n";
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
    bool silent = false, reportVersion = false;
    string outputBaseName;
    for (; argc >= 2; argc--, argv++)
    {
        string arg1 = argv[1];
        string prefix = arg1.substr(0, 1);
        auto prefixIndex =
            string(COMMAND_OPTION_PREFIXES).find_first_of(prefix);
        if (prefixIndex == string::npos)
            break;
        string option = arg1.substr(1);
        if (option == string(1, COMMAND_OPTION_PREFIXES[prefixIndex]))
        {
            argc--; argv++;
            break;
        }

        if (option == "?" || option == "h")
        {
            Usage();
            return 0;
        }
        else if (option == "o")
        {
            outputBaseName = (++argv)[1];
            argc--;
        }
        else if (option == "s")
            silent = true;
        else if (option == "v")
            reportVersion = true;
        else
        {
            cerr << "Invalid option: " << arg1 << endl;
            return 1;
        }
    }

    // Report version information and exit if requested.
    if (reportVersion)
    {
        cout
            << "Asp compiler version "
            << ASP_COMPILER_VERSION_MAJOR << '.'
            << ASP_COMPILER_VERSION_MINOR << '.'
            << ASP_COMPILER_VERSION_PATCH << '.'
            << ASP_COMPILER_VERSION_TWEAK
            << endl;
        return 0;
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
        {".aspec", &specFileName, nullptr},
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
                    cerr
                        << "Error: Multiple files of the same type specified"
                        << endl;
                    Usage();
                    return 1;
                }

                accepted = true;
                *input.fileName = fileName;
                if (input.suffixPos != nullptr)
                    *input.suffixPos = suffixPos;
            }
        }
        if (!accepted)
        {
            cerr << "Error: Unrecognized input file type" << endl;
            Usage();
            return 1;
        }
    }

    // If the application specification is not defined at this point, issue an
    // error and exit.
    if (mainModuleFileName.empty())
    {
        cerr << "Error: Source file not specified" << endl;
        Usage();
        return 1;
    }

    // Split the main module file name into its constituent parts.
    auto mainModuleDirectorySeparatorPos = mainModuleFileName.find_last_of
        (FILE_NAME_SEPARATORS);
    size_t baseNamePos = mainModuleDirectorySeparatorPos == string::npos ?
        0 : mainModuleDirectorySeparatorPos + 1;
    string mainModuleDirectoryName = mainModuleFileName.substr(0, baseNamePos);
    string mainModuleBaseFileName = mainModuleFileName.substr(baseNamePos);

    // If the application specification is not specified, check for the
    // existence of a fixed-named file in the same directory as the source
    // file.
    if (specFileName.empty())
    {
        static const string localSpecBaseFileName = "app.aspec";
        string localSpecFileName = mainModuleDirectoryName;
        if (!localSpecFileName.empty() &&
            strchr(FILE_NAME_SEPARATORS, localSpecFileName.back()) == nullptr)
            localSpecFileName += FILE_NAME_SEPARATORS[0];
        localSpecFileName += localSpecBaseFileName;
        ifstream localSpecStream(localSpecFileName, ios::binary);
        if (localSpecStream)
            specFileName = localSpecFileName;
    }

    // If the application specification is still not defined, resort to the
    // environment variable.
    if (specFileName.empty())
    {
        const char *envSpecFileNameString = getenv("ASP_SPEC_FILE");
        if (envSpecFileNameString != nullptr)
            specFileName = envSpecFileNameString;
    }

    // If the application specification is not defined at this point, issue an
    // error and exit.
    if (specFileName.empty())
    {
        cerr
            << "Error: Application specification not specified"
            << " and no default found" << endl;
        return 2;
    }

    // Open the application specification file.
    ifstream specStream(specFileName, ios::binary);
    if (!specStream)
    {
        cerr
            << "Error opening " << specFileName
            << ": " << strerror(errno) << endl;
        return 2;
    }
    if (!silent)
        cout << "Using " << specFileName << endl;

    // Determine the base name of all output files.
    static string executableSuffix = ".aspe";
    if (outputBaseName.size() > executableSuffix.size() &&
        outputBaseName.substr(outputBaseName.size() - executableSuffix.size())
        == executableSuffix)
        outputBaseName.erase(outputBaseName.size() - executableSuffix.size());
    string baseName =
        outputBaseName.empty() ?
            mainModuleBaseFileName.substr
                (0, mainModuleSuffixPos - baseNamePos) :
        strchr(FILE_NAME_SEPARATORS, outputBaseName.back()) == nullptr ?
            outputBaseName :
            outputBaseName +
            mainModuleBaseFileName.substr
                (0, mainModuleSuffixPos - baseNamePos);

    // Open output executable.
    string executableFileName = baseName + executableSuffix;
    ofstream executableStream(executableFileName, ios::binary);
    if (!executableStream)
    {
        cerr
            << "Error creating " << executableFileName
            << ": " << strerror(errno) << endl;
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
        return 2;
    }

    // Open output source info file.
    static string sourceInfoSuffix = ".aspd";
    string sourceInfoFileName = baseName + sourceInfoSuffix;
    ofstream sourceInfoStream(sourceInfoFileName, ios::binary);
    if (!sourceInfoStream)
    {
        cerr
            << "Error creating " << sourceInfoFileName
            << ": " << strerror(errno) << endl;
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
    if (includePathString != nullptr)
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
        ifstream *moduleStream = nullptr;
        if (moduleFileName == mainModuleBaseFileName)
        {
            // Open specified main module file.
            auto *stream = new ifstream(mainModuleFileName);
            if (*stream)
                moduleStream = stream;
        }
        else
        {
            // Search for the module file using the search path.
            for (auto iter = searchPath.begin();
                 iter != searchPath.end(); iter++)
            {
                auto directory = *iter;

                // For an empty entry, use the main module's directory (which,
                // it may be noted, may also be empty).
                if (directory.empty())
                    directory = mainModuleDirectoryName;

                // Construct a path name for the module file.
                if (!directory.empty() &&
                    strchr(FILE_NAME_SEPARATORS, directory.back()) == nullptr)
                    directory += FILE_NAME_SEPARATORS[0];
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
        if (moduleStream == nullptr)
        {
            cerr
                << "Error opening " << moduleFileName
                << ": " << strerror(errno) << endl;
            errorDetected = true;
            break;
        }
        Lexer lexer(*moduleStream, moduleFileName);

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
                cerr
                    << token->sourceLocation.fileName << ':'
                    << token->sourceLocation.line << ':'
                    << token->sourceLocation.column
                    << ": Bad token encountered: '"
                    << token->s << '\'';
                if (!token->error.empty())
                    cerr << ": " << token->error;
                cerr << endl;
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

        // Remove output files.
        executableStream.close();
        remove(executableFileName.c_str());
        listingStream.close();
        remove(listingFileName.c_str());
        sourceInfoStream.close();
        remove(sourceInfoFileName.c_str());
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

    // Write the source info file.
    executable.WriteSourceInfo(sourceInfoStream);
    sourceInfoStream.close();
    if (!sourceInfoStream)
    {
        cerr
            << "Error writing " << sourceInfoFileName
            << ": " << strerror(errno) << endl;
    }

    // Indicate any error writing the code.
    if (!executableStream)
        return 5;
    if (!listingStream || !sourceInfoStream)
        return 6;

    // Write statistics.
    if (executableStream && !silent)
    {
        cout
            << executableFileName << ": "
            << executableByteCount << " bytes" << endl;
    }

    return 0;
}
