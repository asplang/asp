//
// Asp application specification generator main.
//

#include "lexer.h"
#include "generator.h"
#include "app.h"
#include "search-path.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <cerrno>
#include <utility>

#if !defined ASP_GENERATOR_VERSION_MAJOR || \
    !defined ASP_GENERATOR_VERSION_MINOR || \
    !defined ASP_GENERATOR_VERSION_PATCH || \
    !defined ASP_GENERATOR_VERSION_TWEAK
#error ASP_GENERATOR_VERSION_* macros undefined
#endif
#ifndef COMMAND_OPTION_PREFIXES
#error COMMAND_OPTION_PREFIXES macro undefined
#endif
#ifndef FILE_NAME_SEPARATORS
#error FILE_NAME_SEPARATORS macro undefined
#endif

// Lemon parser.
extern "C" {
void *ParseAlloc(void *(*malloc)(size_t), Generator *);
void Parse(void *parser, int yymajor, Token *yyminor);
void ParseFree(void *parser, void (*free)(void *));
void ParseTrace(FILE *, const char *prefix);
}

using namespace std;

struct ActiveSourceFile
{
    string sourceFileName;
    unique_ptr<istream> sourceStream;
    bool isLibrary;
    SourceLocation oldSourceLocation;
    unique_ptr<Lexer> lexer;
    void *parser;
};

pair<istream *, string> OpenSourceFile
    (const string &sourceFileName,
     const vector<string> &searchPath, const string &fileNameSeparators);

static void Usage()
{
    cerr
        << "Usage:      aspg [OPTION]... ["
        << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << "] SOURCE\n"
        << "\n"
        << "Generate binary application specification file and C code"
        << " from the source\n"
        << "file (*.asps) given as SOURCE.\n"
        << "\n"
        << "Use " << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << " before the SOURCE argument if it starts with an option prefix.\n"
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
        << "c CODE     Write generated C code files to CODE.h and CODE.c"
        << " instead of basing\n"
        << "            file names on the SOURCE file name. If CODE"
        << " ends with " << FILE_NAME_SEPARATORS[0] << ", the output\n"
        << "            file names will be based on SOURCE and the files"
        << " will be written\n"
        << "            into the directory given by CODE. In this"
        << " case, the directory must\n"
        << "            already exist.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "h          Print usage information and exit.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "q          Quiet. Don't output usual generator information.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "s SPEC     Write the binary spec file to SPEC.aspec"
        << " instead of basing the file\n"
        << "            name on the SOURCE file name. If SPEC ends"
        << " with .aspec, the name\n"
        << "            will be used as is. If SPEC ends with "
        << FILE_NAME_SEPARATORS[0] << ", the output file name will\n"
        << "            be based on SOURCE and the file will be"
        << " written into the directory\n"
        << "            given by SPEC. In this case, the"
        << " directory must already exist.\n"
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
    bool quiet = false, reportVersion = false;
    string outputCodeBaseName, outputSpecBaseName;
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
        else if (option == "c")
        {
            if (argc <= 2)
            {
                Usage();
                return 1;
            }

            outputCodeBaseName = (++argv)[1];
            argc--;
        }
        else if (option == "q")
            quiet = true;
        else if (option == "s")
        {
            if (argc <= 2)
            {
                Usage();
                return 1;
            }

            outputSpecBaseName = (++argv)[1];
            argc--;
        }
        else if (option == "v")
            reportVersion = true;
        else
        {
            cerr << "Invalid option: "  << arg1 << endl;
            return 1;
        }
    }

    // Report version information and exit if requested.
    if (reportVersion)
    {
        cout
            << "Asp generator version "
            << ASP_GENERATOR_VERSION_MAJOR << '.'
            << ASP_GENERATOR_VERSION_MINOR << '.'
            << ASP_GENERATOR_VERSION_PATCH << '.'
            << ASP_GENERATOR_VERSION_TWEAK
            << endl;
        return 0;
    }

    // Obtain spec source file name.
    if (argc != 2)
    {
        Usage();
        return 1;
    }
    const string sourceFileName(argv[1]);

    // Validate file name suffix.
    static string sourceSuffix = ".asps";
    bool badSourceFileName = sourceFileName.size() <= sourceSuffix.size();
    size_t suffixPos;
    if (!badSourceFileName)
    {
        suffixPos = sourceFileName.size() - sourceSuffix.size();
        badSourceFileName = sourceFileName.substr(suffixPos) != sourceSuffix;
    }
    if (badSourceFileName)
    {
        cerr << "File name must end with " << sourceSuffix << endl;
        return 1;
    }

    // Prepare to handle the slash as the universal file name separator in
    // addition to the ones specified for the platform.
    string fileNameSeparators = FILE_NAME_SEPARATORS;
    if (strchr(FILE_NAME_SEPARATORS, '/') == nullptr)
        fileNameSeparators += '/';

    // Determine the base of the source file name, excluding any directory
    // path.
    auto sourceDirectorySeparatorPos = sourceFileName.find_last_of
        (fileNameSeparators);
    size_t baseNamePos = sourceDirectorySeparatorPos == string::npos ?
        0 : sourceDirectorySeparatorPos + 1;
    auto baseName = sourceFileName.substr
        (baseNamePos, suffixPos - baseNamePos);
    if (baseName.empty())
    {
        cerr << "Invalid source file name: " << sourceFileName << endl;
        return 1;
    }

    // Determine the base name of the output spec file.
    string specBaseName =
        outputSpecBaseName.empty() ? baseName :
        strchr(FILE_NAME_SEPARATORS, outputSpecBaseName.back()) == nullptr ?
        outputSpecBaseName : outputSpecBaseName + baseName;

    // Determine the base name of output code files.
    string codeBaseName =
        outputCodeBaseName.empty() ? baseName :
        strchr(FILE_NAME_SEPARATORS, outputCodeBaseName.back()) == nullptr ?
        outputCodeBaseName : outputCodeBaseName + baseName;

    // Determine output file names.
    static string specSuffix = ".aspec", headerSuffix = ".h", codeSuffix = ".c";
    string specFileName = specBaseName + specSuffix;
    string headerFileName = codeBaseName + headerSuffix;
    string codeFileName = codeBaseName + codeSuffix;

    // Open input source file.
    auto sourceStream = unique_ptr<istream>(new ifstream(sourceFileName));
    if (!*sourceStream)
    {
        cerr
            << "Error opening " << sourceFileName
            << ": " << strerror(errno) << endl;
        return 1;
    }

    // Open output spec file.
    ofstream specStream(specFileName, ios::binary);
    if (!specStream)
    {
        cerr
            << "Error creating " << specFileName
            << ": " << strerror(errno) << endl;
        return 1;
    }

    // Open output header file.
    ofstream headerStream(headerFileName);
    if (!headerStream)
    {
        cerr
            << "Error creating " << headerFileName
            << ": " << strerror(errno) << endl;
        return 1;
    }

    // Open output code file.
    ofstream codeStream(codeFileName);
    if (!codeStream)
    {
        cerr
            << "Error creating " << codeFileName
            << ": " << strerror(errno) << endl;
        return 1;
    }

    // Prepare to compile the top-level source file.
    Generator generator(cerr, baseName);
    generator.AddModule(baseName);

    // Prepare to search for and process included files.
    vector<string> includePath;
    const char *includePathString = getenv("ASP_SPEC_INCLUDE");
    if (includePathString != nullptr)
        includePath = SearchPath(includePathString);

    // Compile the main spec module and any other modules that are imported.
    bool errorDetected = generator.ErrorCount() > 0;
    while (!errorDetected)
    {
        // Obtain the next module to process.
        auto nextModule = generator.NextModule();
        const auto &moduleName = nextModule.first;
        const auto &sourceReferences = nextModule.second;
        if (moduleName.empty() || errorDetected)
            break;
        string moduleFileName = moduleName + sourceSuffix;

        // Open the module file.
        auto moduleStream = unique_ptr<istream>();
        if (moduleName == baseName)
        {
            // Open the specified main module file.
            auto stream = unique_ptr<istream>
                (new ifstream(sourceFileName));
            if (*stream)
                moduleStream = move(stream);
        }
        else
        {
            // Search for the module file using the search path.
            auto openResult = OpenSourceFile
                (moduleFileName, includePath, fileNameSeparators);
            auto stream = unique_ptr<istream>(openResult.first);
            if (stream != nullptr)
                moduleStream = move(stream);
        }
        if (moduleStream == nullptr)
        {
            // Report the error for every import statement that references the
            // module that cannot be opened.
            for (const auto &sourceReference: sourceReferences)
            {
                const auto &importName = sourceReference.first;
                const auto &sourceLocation =
                    sourceReference.second.sourceLocation;

                cerr
                    << sourceLocation.fileName << ':'
                    << sourceLocation.line << ':'
                    << sourceLocation.column
                    << ": Error opening " << moduleFileName;
                if (importName != moduleName)
                    cerr << " imported as '" << importName << '\'';
                cerr << ": " << strerror(errno) << endl;
            }
            return 1;
        }
        generator.CurrentSource(moduleFileName);

        #ifdef ASP_APPSPEC_DEBUG
        cout << "Parsing module " << moduleFileName << "..." << endl;
        ParseTrace(stdout, "Trace: ");
        #endif

        // Prepare to process top-level source file for the module.
        deque<ActiveSourceFile> activeSourceFiles;
        auto lexer = unique_ptr<Lexer>
            (new Lexer(*moduleStream, moduleFileName));
        activeSourceFiles.emplace_back(ActiveSourceFile
        {
            moduleFileName, move(moduleStream),
            false, SourceLocation(),
            move(lexer), ParseAlloc(malloc, &generator),
        });

        while (!errorDetected)
        {
            auto &activeSourceFile = activeSourceFiles.back();

            Token *token = activeSourceFile.lexer->Next();
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

            generator.SetSourceLocation(token->sourceLocation);

            Parse(activeSourceFile.parser, token->type, token);
            if (generator.ErrorCount() > 0)
            {
                errorDetected = true;
                break;
            }

            // Check for end of source file.
            if (token->type == 0)
            {
                // We're done with the current source file.
                auto oldSourceLocation = activeSourceFile.oldSourceLocation;
                ParseFree(activeSourceFile.parser, free);
                activeSourceFiles.pop_back();
                if (activeSourceFiles.empty())
                    break;
                const auto &activeSourceFile = activeSourceFiles.back();

                // Update the source file name in the generator for error
                // reporting.
                generator.CurrentSource
                    (activeSourceFile.sourceFileName,
                     false, activeSourceFile.isLibrary,
                     oldSourceLocation);
            }
            else
            {
                // Check for library declaration.
                if (generator.IsLibrary())
                    activeSourceFile.isLibrary = true;

                // Check for included source file.
                auto includeFileName = generator.CurrentSourceFileName();
                const auto &oldSourceFileName = activeSourceFile.sourceFileName;
                if (includeFileName != oldSourceFileName)
                {
                    auto openResult = OpenSourceFile
                        (includeFileName, includePath, fileNameSeparators);
                    auto newSourceStream = unique_ptr<istream>
                        (openResult.first);
                    auto newSourceFileName = openResult.second;
                    if (newSourceStream == nullptr)
                    {
                        cerr
                            << token->sourceLocation.fileName << ':'
                            << token->sourceLocation.line << ':'
                            << token->sourceLocation.column
                            << ": Error opening " << includeFileName
                            << ": " << strerror(errno) << endl;
                        return 1;
                    }

                    // Ensure there's no recursive inclusion.
                    for (const auto &activeSourceFile: activeSourceFiles)
                    {
                        if (newSourceFileName ==
                            activeSourceFile.sourceFileName)
                        {
                            cerr
                                << token->sourceLocation.fileName << ':'
                                << token->sourceLocation.line << ':'
                                << token->sourceLocation.column
                                << ": Include cycle detected: "
                                << newSourceFileName << endl;
                            return 1;
                        }
                    }

                    auto newModuleName = generator.CurrentModuleName();
                    auto oldSourceLocation = generator.CurrentSourceLocation();
                    generator.CurrentSource(newSourceFileName);
                    auto lexer = unique_ptr<Lexer>
                        (new Lexer(*newSourceStream, newSourceFileName));
                    activeSourceFiles.emplace_back(ActiveSourceFile
                    {
                        newSourceFileName, move(newSourceStream),
                        false, oldSourceLocation,
                        move(lexer), ParseAlloc(malloc, &generator),
                    });
                }
            }
        }
    }

    // Define a function to close all output files.
    auto closeAll = [&] (bool reportError)
    {
        bool writeError = false;
        struct
        {
            string fileName;
            ofstream &stream;
        } files[] =
        {
            {specFileName, specStream},
            {headerFileName, headerStream},
            {codeFileName, codeStream},
        };
        for (auto &file: files)
        {
            file.stream.close();
            if (!file.stream)
            {
                writeError = true;
                if (reportError)
                    cerr
                        << "Error writing " << file.fileName
                        << ": " << strerror(errno) << endl;
            }
        }
        return !writeError;
    };

    if (errorDetected)
    {
        cerr << "Ended in ERROR" << endl;

        // Remove output files.
        closeAll(false);
        remove(specFileName.c_str());
        remove(headerFileName.c_str());
        remove(codeFileName.c_str());
        return 1;
    }

    // Prepare to write output files.
    generator.Finalize();

    // Write all output files.
    if (!quiet)
        cout << "Writing spec to " << specFileName << endl;
    generator.WriteCompilerSpec(specStream);
    if (!quiet)
        cout
            << "Writing code to " << headerFileName << " and " << codeFileName
            << endl;
    generator.WriteApplicationHeader(headerStream);
    generator.WriteApplicationCode(codeStream);

    // Close all output files.
    return closeAll(true) ? 0 : 2;
}

pair<istream *, string> OpenSourceFile
    (const string &sourceFileName,
     const vector<string> &searchPath, const string &fileNameSeparators)
{
    if (sourceFileName.empty())
        return make_pair(nullptr, sourceFileName);

    // Determine if the file name is a fixed path and, if so, use it as is.
    static const vector<string> absoluteSearchPath = {""};
    auto isFixedPath =
        strchr(FILE_NAME_SEPARATORS, sourceFileName[0]) != 0 ||
        sourceFileName.size() >= 2 && sourceFileName[0] == '.' &&
        strchr(FILE_NAME_SEPARATORS, sourceFileName[1]) != 0 ||
        sourceFileName.size() >= 3 && sourceFileName.substr(0, 2) == ".." &&
        strchr(FILE_NAME_SEPARATORS, sourceFileName[2]) != 0;
    auto &localSearchPath = isFixedPath ? absoluteSearchPath : searchPath;

    // Look for the file in each of the directories on the path.
    for (auto directory: localSearchPath)
    {
        // Construct a path name for the file.
        if (!directory.empty())
        {
            auto separatorIter =
                string(fileNameSeparators).find_first_of(directory.back());
            if (separatorIter == string::npos)
                directory += FILE_NAME_SEPARATORS[0];
        }
        string sourcePathName = directory + sourceFileName;

        // Attempt opening the source file.
        auto sourceStream = new ifstream(sourcePathName);
        if (*sourceStream)
            return make_pair(sourceStream, sourcePathName);
        delete sourceStream;
    }
    return make_pair(nullptr, sourceFileName);
}
