//
// Standalone Asp application main.
//

#include "asp.h"
#include "asp-info.h"
#include "standalone.h"
#include "context.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>

#ifndef COMMAND_OPTION_PREFIX
#error COMMAND_OPTION_PREFIX macro undefined
#endif

using namespace std;

static const size_t DEFAULT_CODE_BYTE_COUNT = 4096;
static const size_t DEFAULT_DATA_ENTRY_COUNT = 2048;

static void Usage()
{
    cerr
        << "Usage:      asps [OPTION]... SCRIPT [ARG]...\n"
        << "\n"
        << "Run the Asp script executable SCRIPT (*.aspe)."
        << " The suffix may be omitted.\n"
        << "If one or more ARG are given,"
        << " they are passed as arguments to the script.\n"
        << "\n"
        << "Options:\n"
        << COMMAND_OPTION_PREFIX
        << "h          Print usage information.\n"
        << COMMAND_OPTION_PREFIX
        << "v          Verbose. Output version and statistical information.\n"
        << COMMAND_OPTION_PREFIX
        << "c n        Code size, in bytes."
        << " Default is " << DEFAULT_CODE_BYTE_COUNT << ".\n"
        << COMMAND_OPTION_PREFIX
        << "d n        Data entry count, where each entry is "
        << AspDataEntrySize() << " bytes."
        << " Default is " << DEFAULT_DATA_ENTRY_COUNT << ".\n"
        #ifdef ASP_DEBUG
        << COMMAND_OPTION_PREFIX
        << "n n        Number of instructions to execute before exiting."
        << " Useful for\n"
        << "            debugging. Available only in debug builds.\n"
        #endif
        ;
}

int main(int argc, char **argv)
{
    // Process command line options.
    bool verbose = false;
    size_t codeByteCount = DEFAULT_CODE_BYTE_COUNT;
    size_t dataEntryCount = DEFAULT_DATA_ENTRY_COUNT;
    unsigned stepCountLimit = UINT_MAX;
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
            codeByteCount = static_cast<size_t>(atoi(value.c_str()));
        }
        else if (option == "d")
        {
            string value = (++argv)[1];
            argc--;
            dataEntryCount = static_cast<size_t>(atoi(value.c_str()));
        }
        #ifdef ASP_DEBUG
        else if (option == "n")
        {
            string value = (++argv)[1];
            argc--;
            stepCountLimit = static_cast<unsigned>(atoi(value.c_str()));
        }
        #endif
        else if (option == "v")
            verbose = true;
        else
        {
            cerr << "Invalid option: " << arg1 << endl;
            Usage();
            return 1;
        }
    }

    // Obtain executable file name.
    if (argc < 2)
    {
        cerr << "No program specified" << endl;
        Usage();
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
    StandaloneAspContext context;
    AspEngine engine;
    char *code = (char *)malloc(codeByteCount);
    char *data = (char *)malloc(dataByteSize);
    AspRunResult initializeResult = AspInitialize
        (&engine,
         code, codeByteCount,
         data, dataByteSize,
         &AspAppSpec_standalone, &context);
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
    AspRunResult runResult = AspRunResult_OK;
    unsigned stepCount = 0;
    #ifdef ASP_DEBUG
    if (stepCountLimit == UINT_MAX)
        cout << "Executing instructions indefinitely..." << endl;
    else
        cout << "Executing " << stepCountLimit << " instructions..." << endl;
    #endif
    for (;
         runResult == AspRunResult_OK &&
         (stepCountLimit == UINT_MAX || stepCount < stepCountLimit);
         stepCount++)
    {
        runResult = AspStep(&engine);
    }

    // Dump data area in debug mode.
    #ifdef ASP_DEBUG
    cout << "---\nDump:" << endl;
    AspDump(&engine, stdout);
    #endif

    #ifdef ASP_DEBUG
    cout << "---\nExecuted " << stepCount << " instructions" << endl;
    if (stepCountLimit != UINT_MAX && stepCount != stepCountLimit)
        cout
            << "WARNING: Did not execute specified number of instructions ("
            << stepCountLimit << ')' << endl;
    #endif

    // Check completion status of the run.
    #ifndef ASP_DEBUG
    if (runResult != AspRunResult_Complete)
    #endif
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
