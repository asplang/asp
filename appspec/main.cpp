//
// Asp application specification generator main.
//

#include "lexer.h"
#include "generator.h"
#include "symbol.hpp"
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
    // Obtain spec source file name.
    if (argc != 2)
    {
        cerr << "Specify file" << endl;
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

    // Split the source file name into its constituent parts.
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

    // Determine output file names.
    static string specSuffix = ".aspec", headerSuffix = ".h", codeSuffix = ".c";
    string specFileName = baseName + specSuffix;
    string headerFileName = baseName + headerSuffix;
    string codeFileName = baseName + codeSuffix;

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

    SymbolTable symbolTable;
    Generator generator(cerr, symbolTable, baseName);
    generator.CurrentSource(sourceFileName);

    #ifdef ASP_APPSPEC_DEBUG
    cout << "Parsing module " << sourceFileName << "..." << endl;
    ParseTrace(stdout, "Trace: ");
    #endif

    // Prepare to search for and process included files.
    vector<string> includePath;
    const char *includePathString = getenv("ASP_SPEC_INCLUDE");
    if (includePathString != nullptr)
        includePath = SearchPath(includePathString);

    // Prepare to process top-level source file.
    deque<ActiveSourceFile> activeSourceFiles;
    auto lexer = unique_ptr<Lexer>
        (new Lexer(*sourceStream, sourceFileName));
    activeSourceFiles.emplace_back(ActiveSourceFile
    {
        sourceFileName, move(sourceStream),
        false, SourceLocation(),
        move(lexer), ParseAlloc(malloc, &generator)
    });

    // Compile spec.
    bool errorDetected = false;
    Token *token;
    while (true)
    {
        auto &activeSourceFile = activeSourceFiles.back();

        token = activeSourceFile.lexer->Next();
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

            // Update source file name in generator for error reporting.
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
            string includeFileName = generator.CurrentSourceFileName();
            const auto &oldSourceFileName = activeSourceFile.sourceFileName;
            if (includeFileName != oldSourceFileName)
            {
                // Determine search path for locating included file.
                auto sourceDirectorySeparatorPos =
                    oldSourceFileName.find_last_of(fileNameSeparators);
                auto localDirectoryName = oldSourceFileName.substr
                    (0,
                     sourceDirectorySeparatorPos == string::npos ?
                     0 : sourceDirectorySeparatorPos + 1);
                vector<string> searchPath {localDirectoryName};
                searchPath.insert
                    (searchPath.end(), includePath.begin(), includePath.end());

                string newSourceFileName;
                unique_ptr<istream> newSourceStream;
                for (auto directory: searchPath)
                {
                    // Determine path name of source file.
                    if (!directory.empty())
                    {
                        auto separatorIter =
                            string(fileNameSeparators).find_first_of
                                (directory.back());
                        if (separatorIter == string::npos)
                            directory += FILE_NAME_SEPARATORS[0];
                    }
                    newSourceFileName = directory + includeFileName;

                    // Attempt opening the source file.
                    auto sourceStream = unique_ptr<istream>
                        (new ifstream(newSourceFileName));
                    if (*sourceStream)
                    {
                        newSourceStream = move(sourceStream);
                        break;
                    }
                }
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
                    if (newSourceFileName == activeSourceFile.sourceFileName)
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

                auto oldSourceLocation = generator.CurrentSourceLocation();
                generator.CurrentSource(newSourceFileName);
                auto lexer = unique_ptr<Lexer>
                    (new Lexer(*newSourceStream, newSourceFileName));
                activeSourceFiles.emplace_back(ActiveSourceFile
                {
                    newSourceFileName, move(newSourceStream),
                    false, oldSourceLocation,
                    move(lexer), ParseAlloc(malloc, &generator)
                });
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

    // Write all output files.
    generator.WriteCompilerSpec(specStream);
    generator.WriteApplicationHeader(headerStream);
    generator.WriteApplicationCode(codeStream);

    // Close all output files.
    return closeAll(true) ? 0 : 2;
}
