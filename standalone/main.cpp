//
// Standalone Asp application main.
//

#include "asp.h"
#include "asp-info.h"
#include "standalone.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace std;

#ifndef COMMAND_OPTION_PREFIX
#define COMMAND_OPTION_PREFIX "-"
#endif

static const size_t DEFAULT_CODE_BYTE_COUNT = 4096;
static const size_t DEFAULT_DATA_ENTRY_COUNT = 2048;

static void Usage()
{
    cerr
        << "Usage: asps [options] script [args]\n"
        << "Options:\n"
        << "-v      Verbose. Output version and statistical information.\n"
        << "-c n    Code size, in bytes."
        << " Default is " << DEFAULT_CODE_BYTE_COUNT << ".\n"
        << "-d n    Data entry count, where each entry is 16 bytes."
        << " Default is " << DEFAULT_DATA_ENTRY_COUNT << ".\n"
        << "Arguments:\n"
        << "script  = Script executable (*.aspe). The suffix may be omitted.\n"
        << "args    = Arguments passed to the script.\n";
}

int main(int argc, char **argv)
{
    // Process command line options.
    bool verbose = false;
    size_t codeByteCount = DEFAULT_CODE_BYTE_COUNT;
    size_t dataEntryCount = DEFAULT_DATA_ENTRY_COUNT;
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
        else if (option == "c")
        {
            string value = (++argv)[1];
            argc--;
            codeByteCount = atoi(value.c_str());
        }
        else if (option == "d")
        {
            string value = (++argv)[1];
            argc--;
            dataEntryCount = atoi(value.c_str());
        }
        else if (option == "v")
            verbose = true;
        else
        {
            cerr << "Invalid option: " << arg1 << endl;
            return 1;
        }
    }

    // Obtain executable file name.
    if (argc < 2)
    {
        cerr << "Specify program" << endl;
        return 1;
    }

    // Open the executable file.
    string executableFileName = argv[1];
    auto openExecutable = [](const char *fileName)
    {
        return fopen(fileName, "rb");
    };
    FILE *fp = openExecutable(executableFileName.c_str());
    static const string executableSuffix = ".aspe";
    if (fp == 0)
    {
        // Try appending the appropriate suffix if the specified file did not
        // exist.
        executableFileName += executableSuffix;
        fp = openExecutable(executableFileName.c_str());
    }
    if (fp == 0)
    {
        cerr << "Error opening " << executableFileName << endl;
        return 1;
    }

    // Determine byte size of data area.
    size_t dataEntrySize = AspDataEntrySize();
    size_t dataByteSize = dataEntryCount * dataEntrySize;

    // Initialize the Asp engine.
    AspEngine engine;
    char *code = (char *)malloc(codeByteCount);
    char *data = (char *)malloc(dataByteSize);
    AspRunResult initializeResult = AspInitialize
        (&engine,
         code, codeByteCount,
         data, dataByteSize,
         &AspAppSpec_standalone, 0);
    if (initializeResult != AspRunResult_OK)
    {
        cerr
            << "Initialize error 0x" << hex << uppercase << setfill('0')
            << setw(2) << initializeResult << ": "
            << AspRunResultToString((int)initializeResult) << endl;
        return 2;
    }

    // Load the executable.
    while (true)
    {
        char c = fgetc(fp);
        if (feof(fp))
            break;
        AspAddCodeResult addResult = AspAddCode(&engine, &c, 1);
        if (addResult != AspAddCodeResult_OK)
        {
            cerr
                << "Load error 0x" << hex << uppercase << setfill('0')
                << setw(2) << addResult << ": "
                << AspAddCodeResultToString((int)addResult) << endl;
            return 2;
        }
    }
    fclose(fp);
    AspAddCodeResult sealResult = AspSeal(&engine);
    if (sealResult != AspAddCodeResult_OK)
    {
        cerr
            << "Seal error 0x" << hex << uppercase << setfill('0')
            << setw(2) << sealResult << ": "
            << AspAddCodeResultToString((int)sealResult) << endl;
        return 2;
    }

    // Report version information.
    if (verbose)
    {
        uint8_t engineVersion[4], codeVersion[4];
        AspEngineVersion(engineVersion);
        AspCodeVersion(&engine, codeVersion);
        cout << "Engine version: ";
        for (unsigned i = 0; i < sizeof engineVersion; i++)
        {
            if (i != 0)
                cout.put('.');
            cout << static_cast<unsigned>(engineVersion[i]);
        }
        cout << endl;
        cout << "Code version: ";
        for (unsigned i = 0; i < sizeof codeVersion; i++)
        {
            if (i != 0)
                cout.put('.');
            cout << static_cast<unsigned>(codeVersion[i]);
        }
        cout << endl;
    }

    // Set arguments.
    AspRunResult argumentResult = AspSetArguments(&engine, argv + 2);
    if (argumentResult != AspRunResult_OK)
    {
        cerr
            << "Arguments error 0x" << hex << uppercase << setfill('0')
            << setw(2) << argumentResult << ": "
            << AspRunResultToString((int)argumentResult) << endl;
        return 2;
    }

    // Run the code.
    AspRunResult runResult;
    while (true)
    {
        runResult = AspStep(&engine);
        if (runResult != AspRunResult_OK)
            break;
    }

    // Check completion status of the run.
    if (runResult != AspRunResult_Complete)
    {
        auto oldFlags = cerr.flags();
        auto oldFill = cerr.fill();
        cerr
            << "Run error 0x" << hex << uppercase << setfill('0')
            << setw(2) << runResult << ": "
            << AspRunResultToString((int)runResult) << endl;
        cerr.flags(oldFlags);
        cerr.fill(oldFill);

        // Report the program counter.
        auto programCounter = AspProgramCounter(&engine);
        cerr
            << "Program counter: 0x" << hex << uppercase << setfill('0')
            << setw(7) << programCounter;
        cerr.flags(oldFlags);
        cerr.fill(oldFill);

        // Attempt to translate the program counter into a source location
        // using the associated source info file.
        size_t suffixPos =
            executableFileName.size() - executableSuffix.size();
        static const string sourceInfoSuffix = ".aspd";
        string sourceInfoFileName =
            executableFileName.substr(0, suffixPos) + sourceInfoSuffix;
        AspSourceInfo *sourceInfo = AspLoadSourceInfoFromFile
            (sourceInfoFileName.c_str());
        if (sourceInfo != 0)
        {
            AspSourceLocation sourceLocation = AspGetSourceLocation
                (sourceInfo, programCounter);
            if (sourceLocation.fileName != 0)
                cerr
                    << "; " << sourceLocation.fileName << ':'
                    << sourceLocation.line << ':' << sourceLocation.column;
            AspUnloadSourceInfo(sourceInfo);
        }
        cerr << endl;
    }

    // Dump data area in debug mode.
    #ifdef ASP_DEBUG
    cout << "Dump:" << endl;
    AspDump(&engine, stdout);
    #endif

    // Report low free count.
    if (verbose)
    {
        cout
            << "Low free count: "
            << AspLowFreeCount(&engine)
            << " (max " << AspMaxDataSize(&engine) << ')' << endl;
    }

    free(code);
    free(data);

    return runResult == AspRunResult_Complete ? 0 : 2;
}
