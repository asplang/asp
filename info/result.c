/*
 * Asp info library implementation - result codes to strings.
 */

#include "asp-info.h"
#include "asp.h"

const char *AspAddCodeResultToString(int result)
{
    switch ((AspAddCodeResult)result)
    {
        default:
            return "Unknown add code result";
        case AspAddCodeResult_OK:
            return "OK";
        case AspAddCodeResult_InvalidFormat:
            return "Invalid format";
        case AspAddCodeResult_InvalidVersion:
            return "Invalid version";
        case AspAddCodeResult_InvalidCheckValue:
            return "Invalid check value";
        case AspAddCodeResult_OutOfCodeMemory:
            return "Out of code memory";
        case AspAddCodeResult_InvalidState:
            return "Invalid state";
    }
}

const char *AspRunResultToString(int result)
{
    switch ((AspRunResult)result)
    {
        default:
            return
                result > (int)AspRunResult_Application ?
                "Application specific run result" : "Unknown run result";
        case AspRunResult_OK:
            return "OK";
        case AspRunResult_Complete:
            return "Complete";
        case AspRunResult_InitializationError:
            return "Initialization error";
        case AspRunResult_InvalidState:
            return "Invalid state";
        case AspRunResult_InvalidInstruction:
            return "Invalid instruction";
        case AspRunResult_InvalidEnd:
            return "Invalid end";
        case AspRunResult_BeyondEndOfCode:
            return "Beyond end of code";
        case AspRunResult_StackUnderflow:
            return "Stack underflow";
        case AspRunResult_CycleDetected:
            return "Cycle detected";
        case AspRunResult_InvalidContext:
            return "Invalid context";
        case AspRunResult_Redundant:
            return "Redundant";
        case AspRunResult_UnexpectedType:
            return "Unexpected type";
        case AspRunResult_SequenceMismatch:
            return "Sequence mismatch";
        case AspRunResult_StringFormattingError:
            return "String formatting error";
        case AspRunResult_InvalidFormatString:
            return "Invalid format string";
        case AspRunResult_NameNotFound:
            return "Name not found";
        case AspRunResult_KeyNotFound:
            return "Key not found";
        case AspRunResult_ValueOutOfRange:
            return "Value out of range";
        case AspRunResult_IteratorAtEnd:
            return "Iterator at end";
        case AspRunResult_MalformedFunctionCall:
            return "Malformed function call";
        case AspRunResult_UndefinedAppFunction:
            return "Undefined app function";
        case AspRunResult_DivideByZero:
            return "Divide by zero";
        case AspRunResult_OutOfDataMemory:
            return "Out of data memory";
        case AspRunResult_Again:
            return "Again";
        case AspRunResult_Abort:
            return "Abort";
        case AspRunResult_InternalError:
            return "Internal error";
        case AspRunResult_NotImplemented:
            return "Not implemented";
        case AspRunResult_Application:
            return "Application error";
    }
}
