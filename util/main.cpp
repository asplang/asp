/*
 * Asp info utility main.
 */

#include "asp-info.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cerrno>

using namespace std;

#ifndef COMMAND_OPTION_PREFIXES
#error COMMAND_OPTION_PREFIXES macro undefined
#endif

static void Usage()
{
    cerr
        << "Usage:      aspinfo {OPTION}... ["
        << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << "] [INFO]\n"
        << "\n"
        << "Print the requested error information."
        << " Some options require INFO, the Asp\n"
        << "source info file (*.aspd). The suffix may be omitted."
        << " The INFO argument may be\n"
        << "omitted if neither " << COMMAND_OPTION_PREFIXES[0] << "l nor "
        << COMMAND_OPTION_PREFIXES[0] << "p is used.\n"
        << "\n"
        << "Use " << COMMAND_OPTION_PREFIXES[0] << COMMAND_OPTION_PREFIXES[0]
        << " before the INFO argument if it starts with an option prefix.\n"
        << "\n"
        << "Options";
    if (strlen(COMMAND_OPTION_PREFIXES) > 1)
    {
        cerr << " (may be prefixed by";
        for (unsigned i = 1; i < strlen(COMMAND_OPTION_PREFIXES); i++)
        {
            if (i != 1)
                cerr << ',';
            cerr << ' ' << COMMAND_OPTION_PREFIXES[i];
        }
        cerr << " instead of " << COMMAND_OPTION_PREFIXES[0] << ')';
    }
    cerr
        << ":\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "a code     Translate the add code result to descriptive text.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "e code     Translate the run result to descriptive text.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "l          List all source files.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "p pc       Translate program counter source location.\n"
        << COMMAND_OPTION_PREFIXES[0]
        << "s name     Translate symbol number to name.\n";
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        Usage();
        return 0;
    }

    // Locate the source info file name argument.
    int argIndex = 1;
    for (; argIndex < argc; argIndex++)
    {
        string arg(argv[argIndex]);
        string prefix = arg.substr(0, 1);
        auto prefixIndex =
            string(COMMAND_OPTION_PREFIXES).find_first_of(prefix);
        if (prefixIndex == string::npos)
            break;
        if (arg.substr(1) == string(1, COMMAND_OPTION_PREFIXES[prefixIndex]))
        {
            argIndex++;
            break;
        }

        string option = arg.substr(1);
        if (option == "e" || option == "a" || option == "p" || option == "s")
            argIndex++;
    }

    // Open the source info file, if specified.
    AspSourceInfo *sourceInfo = nullptr;
    if (argIndex != argc)
    {
        if (argIndex + 1 != argc)
        {
            cerr << "Command line error at argument " << argIndex << endl;
            Usage();
            return 1;
        }
        string sourceInfoFileNameArg = argv[argIndex];

        // Load the source info.
        static const string sourceInfoSuffix = ".aspd";
        string sourceInfoFileName = sourceInfoFileNameArg;
        if (sourceInfoFileName.size() <= sourceInfoSuffix.size() ||
            sourceInfoFileName.substr
                (sourceInfoFileName.size() - sourceInfoSuffix.size())
            != sourceInfoSuffix)
            sourceInfoFileName += sourceInfoSuffix;
        errno = 0;
        sourceInfo = AspLoadSourceInfoFromFile
            (sourceInfoFileName.c_str());
        if (sourceInfo == nullptr)
        {
            cerr << "Error loading " << sourceInfoFileName;
            if (errno != 0)
                cerr << ": " << strerror(errno);
            else
                cerr << ": Possible file corruption";
            cerr << endl;
            return 1;
        }
    }

    // Process command line options.
    for (int optionIndex = 1; optionIndex < argIndex; optionIndex++)
    {
        string arg(argv[optionIndex]);
        string option = arg.substr(1);

        if (sourceInfo == nullptr &&
            (option == "l" || option == "p" || option == "s"))
        {
            cerr
                << arg
                << " option ignored in absence of source info file"
                << endl;

            if (option == "p")
                ++optionIndex;

            continue;
        }

        if (option == "a" || option == "e" || option == "p" || option == "s")
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
                    << "Invalid value for " << arg
                    << ": " << optionArgument << endl;
                break;
            }

            if (option == "a")
            {
                auto oldFlags = cout.flags();
                auto oldFill = cout.fill();
                cout
                    << "Add code error " << value
                    << " (0x" << hex << uppercase << setfill('0')
                    << setw(2) << value << "): "
                    << AspRunResultToString(value) << endl;
                cout.flags(oldFlags);
                cout.fill(oldFill);
            }
            else if (option == "e")
            {
                auto oldFlags = cout.flags();
                auto oldFill = cout.fill();
                cout
                    << "Run error " << value
                    << " (0x" << hex << uppercase << setfill('0')
                    << setw(2) << value << "): "
                    << AspRunResultToString(value) << endl;
                cout.flags(oldFlags);
                cout.fill(oldFill);
            }
            else if (option == "p")
            {
                if (value < 0)
                {
                    cerr << "Invalid value for " << arg << endl;
                    break;
                }
                uint32_t pc = static_cast<uint32_t>(value);
                auto sourceLocation = AspGetSourceLocation(sourceInfo, pc);
                auto oldFlags = cout.flags();
                auto oldFill = cout.fill();
                cout
                    << "Program counter " << pc
                    << " (0x" << hex << uppercase << setfill('0')
                    << setw(7) << pc << "): ";
                cout.flags(oldFlags);
                cout.fill(oldFill);
                if (sourceLocation.fileName == nullptr)
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
            else if (option == "s")
            {
                int32_t symbol = static_cast<int32_t>(value);
                auto name = AspGetSymbolName(sourceInfo, symbol);
                cout << "Symbol " << symbol << ": ";
                if (name == nullptr)
                    cout << "? (name information not present)";
                else if (strlen(name) == 0)
                    cout << "? (symbol not found)";
                else
                    cout << name;
                cout << endl;
            }
        }
        else if (option == "l")
        {
            cout << "Source file names:" << endl;
            for (unsigned i = 0; ; i++)
            {
                auto fileName = AspGetSourceFileName(sourceInfo, i);
                if (fileName == nullptr)
                    break;
                cout << setw(4) << i << ". " << fileName << endl;
            }
            cout << '-' << endl;
        }
    }

    if (sourceInfo != nullptr)
        AspUnloadSourceInfo(sourceInfo);

    return 0;
}
