/*
 * Asp info utility main.
 */

#include "asp-info.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>

using namespace std;

#ifndef COMMAND_OPTION_PREFIX
#define COMMAND_OPTION_PREFIX "-"
#endif

static void Usage()
{
    cerr
        << "Usage: aspinfo {options}... [file]\n"
        << "Options:\n"
        << "-a code     Translate the add code result to descriptive text.\n"
        << "-e code     Translate the run result to descriptive text.\n"
        << "-l          List all source files.\n"
        << "-p pc       Translate program counter source location.\n"
        << "Arguments:\n"
        << "file      = Source info file (*.aspd). The suffix may be omitted\n"
        << "            The file argument may be omitted if neither -l nor -p\n"
        << "            is used.\n";
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        Usage();
        return 0;
    }

    // Locate the source info file name argument.
    string optionPrefix = string(COMMAND_OPTION_PREFIX);
    int argumentIndex = 1;
    for (; argumentIndex < argc; argumentIndex++)
    {
        string argument(argv[argumentIndex]);
        if (argument.empty())
        {
            cerr << "Command line error at argument " << argumentIndex << endl;
            Usage();
            return 1;
        }
        if (argument.substr(0, optionPrefix.size()) != optionPrefix)
            break;
        string option = argument.substr(optionPrefix.size());
        if (option == "e" || option == "a" || option == "p")
            argumentIndex++;
    }

    // Open the source info file, if specified.
    AspSourceInfo *sourceInfo = 0;
    if (argumentIndex != argc)
    {
        if (argumentIndex + 1 != argc)
        {
            cerr << "Command line error at argument " << argumentIndex << endl;
            Usage();
            return 1;
        }
        string sourceInfoFileName = argv[argumentIndex];

        // Load the source info.
        sourceInfo = AspLoadSourceInfoFromFile
            (sourceInfoFileName.c_str());
        if (sourceInfo == 0)
        {
            // Try appending the appropriate suffix if the specified file did
            // not exist.
            static const string sourceInfoSuffix = ".aspd";
            string sourceInfoFileName2 = sourceInfoFileName + sourceInfoSuffix;
            sourceInfo = AspLoadSourceInfoFromFile
                (sourceInfoFileName2.c_str());
        }
        if (sourceInfo == 0)
        {
            cerr << "Error loading " << sourceInfoFileName << endl;
            return 1;
        }
    }

    // Process command line options.
    for (int optionIndex = 1; optionIndex < argumentIndex; optionIndex++)
    {
        string argument(argv[optionIndex]);
        string option = argument.substr(optionPrefix.size());

        if (sourceInfo == 0 &&
            (option == "l" || option == "p"))
        {
            cerr
                << argument
                << " option ignored in absence of source info file"
                << endl;

            if (option == "p")
                ++optionIndex;

            continue;
        }

        if (option == "a" || option == "e" || option == "p")
        {
            string optionArgument = string(argv[++optionIndex]);
            size_t scannedSize = 0;
            int value;
            try
            {
                value = stoi(optionArgument, &scannedSize, 0);
            }
            catch (const logic_error &)
            {
                scannedSize = 0;
            }
           
            if (scannedSize != optionArgument.size())
            {
                cerr
                    << "Invalid value for " << argument
                    << ": " << optionArgument << endl;
                break;
            }

            if (option == "a")
            {
                cout
                    << "Add code error " << value << ": "
                    << AspRunResultToString(value) << endl;
            }
            else if (option == "e")
            {
                cout
                    << "Run error " << value << ": "
                    << AspRunResultToString(value) << endl;
            }
            else if (option == "p")
            {
                if (value < 0)
                {
                    cerr << "Invalid value for " << argument << endl;
                    break;
                }
                uint32_t pc = static_cast<uint32_t>(value);
                auto sourceLocation = AspGetSourceLocation(sourceInfo, pc);
                cout << "Program counter " << value << ": ";
                if (sourceLocation.fileName == 0)
                {
                    cout << "No source";
                }
                else
                {
                    cout
                        << sourceLocation.fileName << ':'
                        << sourceLocation.line << ':' << sourceLocation.column;
                }
                cout << endl;
            }
        }
        else if (option == "l")
        {
            cout << "Source file names:" << endl;
            for (unsigned i = 0; ; i++)
            {
                auto fileName = AspGetSourceFileName(sourceInfo, i);
                if (fileName == 0)
                    break;
                cout << setw(4) << i << ". " << fileName << endl;
            }
            cout << '-' << endl;
        }
    }

    if (sourceInfo != 0)
        AspUnloadSourceInfo(sourceInfo);

    return 0;
}
