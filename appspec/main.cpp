//
// Asp application specification generator main.
//

#include "lexer.h"
#include "generator.h"
#include "symbol.hpp"
#include "app.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cctype>

// Lemon parser.
extern "C" {
void *ParseAlloc(void *(*malloc)(size_t), Generator *);
void Parse(void *parser, int yymajor, Token *yyminor);
void ParseFree(void *parser, void (*free)(void *));
void ParseTrace(FILE *, const char *prefix);
}

using namespace std;

int main(int argc, char **argv)
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
    auto suffixIndex = sourceFileName.size() - sourceSuffix.size();
    badSourceFileName = sourceFileName.substr(suffixIndex) != sourceSuffix;
    if (badSourceFileName)
    {
        cerr << "File name must end with " << sourceSuffix << endl;
        return 1;
    }
    static string specSuffix = ".aspec", headerSuffix = ".h", codeSuffix = ".c";
    string baseFileName = sourceFileName.substr(0, suffixIndex);
    string specFileName = baseFileName + specSuffix;
    string headerFileName = baseFileName + headerSuffix;
    string codeFileName = baseFileName + codeSuffix;

    // Open input source file.
    ifstream sourceStream(sourceFileName);
    if (!sourceStream)
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

    // Determine the base name for use in code generation.
    auto baseNameIter = find_if_not
        (baseFileName.rbegin(), baseFileName.rend(),
         [](char c){return isalnum(c) || c == '_';});
    string baseName =
        baseFileName.substr(baseFileName.rend() - baseNameIter);

    Lexer lexer(sourceStream);
    SymbolTable symbolTable;
    Generator generator(cerr, symbolTable, baseName);

    #ifdef ASP_APPSPEC_DEBUG
    cout << "Parsing module " << sourceFileName << "..." << endl;
    ParseTrace(stdout, "Trace: ");
    #endif

    void *parser = ParseAlloc(malloc, &generator);

    // Compile spec.
    bool errorDetected = false;
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

        Parse(parser, token->type, token);
        if (generator.ErrorCount() > 0)
            errorDetected = true;

    } while (!errorDetected && token->type != 0);

    ParseFree(parser, free);

    if (errorDetected)
    {
        cerr << "Ended in ERROR" << endl;
        return 1;
    }

    // Write all output files.
    generator.WriteCompilerSpec(specStream);
    generator.WriteApplicationHeader(headerStream);
    generator.WriteApplicationCode(codeStream);

    // Close all output files.
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
    for (unsigned i = 0; i < sizeof files / sizeof *files; i++)
    {
        files[i].stream.close();
        if (!files[i].stream)
            cerr
                << "Error writing " << files[i].fileName
                << ": " << strerror(errno) << endl;
    }

    return 0;
}
