//
// Standalone Asp application main.
//

#include "asp.h"
#include "standalone.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace std;

const size_t ASP_DATA_ENTRY_COUNT = 2048;
const size_t ASP_CODE_BYTE_COUNT = 4096;

int main(int argc, char **argv)
{
    // Obtain executable file name.
    if (argc != 2)
    {
        cerr << "Specify program" << endl;
        return 1;
    }

    // Open the executable file.
    const char *executableFileName = argv[1];
    FILE *fp = fopen(executableFileName, "rb");
    if (fp == 0)
    {
        cerr << "Error opening " << executableFileName << endl;
        return 1;
    }

    // Determine byte size of data area.
    size_t dataEntrySize = AspDataEntrySize();
    size_t dataByteSize = ASP_DATA_ENTRY_COUNT * dataEntrySize;

    // Initialize the Asp engine.
    AspEngine engine;
    char *code = (char *)malloc(ASP_CODE_BYTE_COUNT);
    char *data = (char *)malloc(dataByteSize);
    AspRunResult initializeResult = AspInitialize
        (&engine,
         code, ASP_CODE_BYTE_COUNT,
         data, dataByteSize,
         &AspAppSpec_standalone, 0);
    if (initializeResult != AspRunResult_OK)
    {
        cerr
            << "Error 0x" << hex << initializeResult
            << " initializing Asp engine" << endl;
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
                << "Error 0x" << hex << addResult
                << " loading code" << endl;
            return 2;
        }
    }
    AspAddCodeResult sealResult = AspSeal(&engine);
    if (sealResult != AspAddCodeResult_OK)
    {
        cerr
            << "Error 0x" << hex << sealResult
            << " sealing code" << endl;
        return 2;
    }

    // Report version information.
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
        cerr << "Run error 0x" << hex << runResult << endl;

    // Dump data area in debug mode.
    #ifdef ASP_DEBUG
    cout << "Dump:" << endl;
    AspDump(&engine, stdout);
    #endif

    // Report low free count.
    cout
        << "Low free count: "
        << AspLowFreeCount(&engine)
        << " (max " << AspMaxDataSize(&engine) << ')' << endl;

    return 0;
}
