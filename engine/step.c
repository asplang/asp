/*
 * Asp engine step implementation.
 */

#include "asp.h"
#include "data.h"
#include "stack.h"
#include "opcode.h"
#include "data.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include "assign.h"
#include "function.h"
#include "operation.h"
#include "symbols.h"
#include <string.h>
#include <stdint.h>

#ifdef ASP_DEBUG
#include "debug.h"
#include <stdio.h>
#include <ctype.h>
#endif

static AspRunResult Step(AspEngine *);
static AspRunResult LoadUnsignedOperand
    (AspEngine *, unsigned operandSize, uint32_t *operand);
static AspRunResult LoadSignedOperand
    (AspEngine *, unsigned operandSize, int32_t *operand);
static AspRunResult LoadFloatOperand
    (AspEngine *, double *operand);

AspRunResult AspStep(AspEngine *engine)
{
    if (engine->inApp)
        return AspRunResult_InvalidState;
    if (engine->state == AspEngineState_Ready)
        engine->state = AspEngineState_Running;
    if (engine->state != AspEngineState_Running)
        return AspRunResult_InvalidState;

    if (engine->runResult == AspRunResult_OK)
    {
        /* Step the engine and update the run result. Note that the
           run result can be set via a return value (normal) or directly by
           the code (some low-level routines). Direct updates take
           precedence as they indicate a sort of failed assertion. */
        AspRunResult stepResult = Step(engine);
        if (engine->runResult == AspRunResult_OK)
            engine->runResult = stepResult;
        if (engine->runResult != AspRunResult_OK)
            engine->state = AspEngineState_RunError;
    }

    return engine->runResult;
}

static AspRunResult Step(AspEngine *engine)
{
    #ifdef ASP_DEBUG
    printf("@0x%7.7X: ", AspProgramCounter(engine));
    #endif

    if (engine->pc >= engine->code + engine->codeEndIndex)
        return AspRunResult_BeyondEndOfCode;
    uint8_t *pc = engine->pc;
    uint8_t opCode = *engine->pc++;
    #ifdef ASP_DEBUG
    printf("0x%2.2X ", opCode);
    #endif

    unsigned operandSize = 0;
    switch (opCode)
    {
        default:
            return AspRunResult_InvalidInstruction;

        case OpCode_PUSHN:
        {
            #ifdef ASP_DEBUG
            puts("PUSHN");
            #endif

            AspDataEntry *stackEntry = AspPush
                (engine, engine->noneSingleton);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_PUSHE:
        {
            #ifdef ASP_DEBUG
            puts("PUSHE");
            #endif

            AspDataEntry *valueEntry = AspAllocEntry(engine, DataType_Ellipsis);
            if (valueEntry == 0)
                return AspRunResult_OutOfDataMemory;

            AspDataEntry *stackEntry = AspPush(engine, valueEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, valueEntry);

            break;
        }

        case OpCode_PUSHF:
        case OpCode_PUSHT:
        {
            #ifdef ASP_DEBUG
            printf("PUSH%c\n", opCode == OpCode_PUSHF ? 'F' : 'T');
            #endif

            AspDataEntry *valueEntry = AspNewBoolean
                (engine, opCode != OpCode_PUSHF);
            if (valueEntry == 0)
                return AspRunResult_OutOfDataMemory;

            AspDataEntry *stackEntry = AspPush(engine, valueEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, valueEntry);

            break;
        }

        case OpCode_PUSHI4:
            operandSize += 2;
        case OpCode_PUSHI2:
            operandSize++;
        case OpCode_PUSHI1:
            operandSize++;
        case OpCode_PUSHI0:
        {
            #ifdef ASP_DEBUG
            printf("PUSHI ");
            #endif

            /* Fetch the integer value from the operand. */
            int32_t value;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &value);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", value);
            #endif

            AspDataEntry *valueEntry = AspAllocEntry(engine, DataType_Integer);
            if (valueEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetInteger(valueEntry, value);

            AspDataEntry *stackEntry = AspPush(engine, valueEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, valueEntry);

            break;
        }

        case OpCode_PUSHD:
        {
            #ifdef ASP_DEBUG
            printf("PUSHD ");
            #endif

            /* Fetch the floating-point value from the operand. */
            double value;
            AspRunResult operandLoadResult = LoadFloatOperand
                (engine, &value);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%g\n", value);
            #endif

            AspDataEntry *valueEntry = AspAllocEntry(engine, DataType_Float);
            if (valueEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetFloat(valueEntry, value);

            AspDataEntry *stackEntry = AspPush(engine, valueEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, valueEntry);

            break;
        }

        case OpCode_PUSHS4:
            operandSize += 2;
        case OpCode_PUSHS2:
            operandSize++;
        case OpCode_PUSHS1:
            operandSize++;
        case OpCode_PUSHS0:
        {
            #ifdef ASP_DEBUG
            printf("PUSHS ");
            #endif

            /* Fetch the string size from the operand. */
            uint32_t size;
            AspRunResult operandLoadResult = LoadUnsignedOperand
                (engine, operandSize, &size);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d, ", size);
            #endif

            /* Validate the bytes of the string. */
            #ifdef ASP_DEBUG
            putchar('\'');
            #endif
            for (uint32_t i = 0; i < size; i++)
            {
                if (engine->pc + i > engine->code + engine->codeEndIndex)
                {
                    #ifdef ASP_DEBUG
                    puts("");
                    #endif
                    return AspRunResult_BeyondEndOfCode;
                }

                #ifdef ASP_DEBUG
                uint8_t c = engine->pc[i];
                if (c == '\'')
                    putchar('\\');
                putchar(isprint(c) ? c : '.');
                #endif
            }
            #ifdef ASP_DEBUG
            puts("'");
            #endif

            AspDataEntry *stringEntry = AspAllocEntry(engine, DataType_String);
            if (stringEntry == 0)
                return AspRunResult_OutOfDataMemory;

            AspRunResult appendResult = AspStringAppendBuffer
                (engine, stringEntry, (char *)engine->pc, (size_t)size);
            if (appendResult != AspRunResult_OK)
                return appendResult;

            engine->pc += size;

            AspDataEntry *stackEntry = AspPush(engine, stringEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, stringEntry);

            break;
        }

        case OpCode_PUSHTU:
        case OpCode_PUSHLI:
        case OpCode_PUSHSE:
        case OpCode_PUSHDI:
        case OpCode_PUSHAL:
        case OpCode_PUSHPL:
        {
            #ifdef ASP_DEBUG
            const char *suffix = 0;
            #endif
            DataType type = DataType_None;
            switch (opCode)
            {
                default:
                    return AspRunResult_InvalidInstruction;

                case OpCode_PUSHTU:
                    #ifdef ASP_DEBUG
                    suffix = "TU";
                    #endif
                    type = DataType_Tuple;
                    break;

                case OpCode_PUSHLI:
                    #ifdef ASP_DEBUG
                    suffix = "LI";
                    #endif
                    type = DataType_List;
                    break;

                case OpCode_PUSHSE:
                    #ifdef ASP_DEBUG
                    suffix = "SE";
                    #endif
                    type = DataType_Set;
                    break;

                case OpCode_PUSHDI:
                    #ifdef ASP_DEBUG
                    suffix = "DI";
                    #endif
                    type = DataType_Dictionary;
                    break;

                case OpCode_PUSHAL:
                    #ifdef ASP_DEBUG
                    suffix = "AL";
                    #endif
                    type = DataType_ArgumentList;
                    break;

                case OpCode_PUSHPL:
                    #ifdef ASP_DEBUG
                    suffix = "PL";
                    #endif
                    type = DataType_ParameterList;
                    break;
            }

            #ifdef ASP_DEBUG
            printf("PUSH%s\n", suffix);
            #endif

            AspDataEntry *valueEntry = AspAllocEntry(engine, type);
            if (valueEntry == 0)
                return AspRunResult_OutOfDataMemory;

            AspDataEntry *stackEntry = AspPush(engine, valueEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            if (AspIsObject(valueEntry))
                AspUnref(engine, valueEntry);

            break;
        }

        case OpCode_PUSHCA:
        {
            #ifdef ASP_DEBUG
            printf("PUSHCA ");
            #endif

            /* Fetch the code address from the operand. */
            uint32_t codeAddressOperand;
            AspRunResult operandLoadResult = LoadUnsignedOperand
                (engine, 4, &codeAddressOperand);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("@0x%7.7X\n", codeAddressOperand);
            #endif
            if (codeAddressOperand >= engine->codeEndIndex)
                return AspRunResult_BeyondEndOfCode;

            AspDataEntry *codeAddressEntry = AspAllocEntry
                (engine, DataType_CodeAddress);
            AspDataSetCodeAddress(codeAddressEntry, codeAddressOperand);
            AspDataEntry *stackEntry = AspPush(engine, codeAddressEntry);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_PUSHM4:
            operandSize += 2;
        case OpCode_PUSHM2:
            operandSize++;
        case OpCode_PUSHM1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("PUSHM ");
            #endif

            /* Fetch the module's symbol from the operand. */
            int32_t moduleSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &moduleSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", moduleSymbol);
            #endif

            /* Look up the module. */
            AspTreeResult moduleFindResult = AspFindSymbol
                (engine, engine->modules, moduleSymbol);
            if (moduleFindResult.result != AspRunResult_OK)
                return moduleFindResult.result;
            AspDataEntry *module = moduleFindResult.value;
            if (module == 0)
                return AspRunResult_NameNotFound;
            if (AspDataGetType(module) != DataType_Module)
                return AspRunResult_UnexpectedType;

            /* Push the module onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, module);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_POP:
        {
            #ifdef ASP_DEBUG
            puts("POP");
            #endif

            AspDataEntry *operand = AspTop(engine);
            if (operand == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(operand))
                return AspRunResult_UnexpectedType;
            AspPop(engine);

            break;
        }

        case OpCode_LNOT:
        case OpCode_POS:
        case OpCode_NEG:
        case OpCode_NOT:
        {
            #ifdef ASP_DEBUG
            puts("Unary op");
            #endif

            /* Fetch the operand from the stack. */
            AspDataEntry *operand = AspTop(engine);
            if (operand == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(operand))
                return AspRunResult_UnexpectedType;
            AspRef(engine, operand);
            AspPop(engine);

            /* Perform the operation. */
            AspOperationResult operationResult = AspPerformUnaryOperation
                (engine, opCode, operand);
            if (operationResult.result != AspRunResult_OK)
                return operationResult.result;

            /* Push the result onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, operationResult.value);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, operationResult.value);
            AspUnref(engine, operand);

            break;
        }

        case OpCode_OR:
        case OpCode_XOR:
        case OpCode_AND:
        case OpCode_LSH:
        case OpCode_RSH:
        case OpCode_ADD:
        case OpCode_SUB:
        case OpCode_MUL:
        case OpCode_DIV:
        case OpCode_FDIV:
        case OpCode_MOD:
        case OpCode_POW:
        case OpCode_NE:
        case OpCode_EQ:
        case OpCode_LT:
        case OpCode_LE:
        case OpCode_GT:
        case OpCode_GE:
        case OpCode_NIN:
        case OpCode_IN:
        case OpCode_NIS:
        case OpCode_IS:
        {
            #ifdef ASP_DEBUG
            puts("Binary op");
            #endif

            /* Access the right value from the stack. */
            AspDataEntry *right = AspTop(engine);
            if (right == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(right))
                return AspRunResult_UnexpectedType;
            AspRef(engine, right);
            AspPop(engine);

            /* Fetch the left value from the stack. */
            AspDataEntry *left = AspTop(engine);
            if (left == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(left))
                return AspRunResult_UnexpectedType;
            AspRef(engine, left);
            AspPop(engine);

            /* Perform the operation. */
            AspOperationResult operationResult = AspPerformBinaryOperation
                (engine, opCode, left, right);
            if (operationResult.result != AspRunResult_OK)
                return operationResult.result;

            /* Push the result onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, operationResult.value);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, operationResult.value);
            AspUnref(engine, left);
            AspUnref(engine, right);

            break;
        }

        case OpCode_LD4:
            operandSize += 2;
        case OpCode_LD2:
            operandSize++;
        case OpCode_LD1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("LD ");
            #endif

            /* Fetch the variable's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Look up the variable, trying first the local namespace, and
               then failing that, the global and system namespaces in turn.
               Note that a local variable can also defer to the global
               namespace via a global override. */
            AspTreeResult findResult = AspFindSymbol
                (engine, engine->localNamespace, variableSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            if ((findResult.node == 0 ||
                 AspDataGetNamespaceNodeIsGlobal(findResult.node)) &&
                engine->globalNamespace != engine->localNamespace)
            {
                findResult = AspFindSymbol
                    (engine, engine->globalNamespace, variableSymbol);
                if (findResult.result != AspRunResult_OK)
                    return findResult.result;
            }
            if (findResult.node == 0)
            {
                findResult = AspFindSymbol
                    (engine, engine->systemNamespace, variableSymbol);
                if (findResult.result != AspRunResult_OK)
                    return findResult.result;
            }
            if (findResult.node == 0)
                return AspRunResult_NameNotFound;

            /* Push variable's value. */
            AspDataEntry *object = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(findResult.node));
            if (!AspIsObject(object))
                return AspRunResult_UnexpectedType;
            AspDataEntry *stackEntry = AspPush(engine, object);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_LDA4:
            operandSize += 2;
        case OpCode_LDA2:
            operandSize++;
        case OpCode_LDA1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("LDA ");
            #endif

            /* Fetch the variable's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Look up the variable, creating it if it doesn't exist. */
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, engine->localNamespace,
                 variableSymbol, engine->noneSingleton);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            AspDataEntry *node = insertResult.node;

            /* Set the scope usage for the newly created variables. */
            if (insertResult.inserted)
            {
                if (engine->localNamespace != engine->globalNamespace)
                    AspDataSetNamespaceNodeIsLocal(node, true);
                else
                    AspDataSetNamespaceNodeIsGlobal(node, true);
            }
            else if (AspDataGetNamespaceNodeIsGlobal(node) &&
                     engine->localNamespace != engine->globalNamespace)
            {
                /* Use global scope because of global override. */
                insertResult = AspTreeTryInsertBySymbol
                    (engine, engine->globalNamespace,
                     variableSymbol, engine->noneSingleton);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
            }

            /* Push variable's tree node to serve as an address. */
            AspDataEntry *stackEntry = AspPush(engine, insertResult.node);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_SET:
        case OpCode_SETP:
        {
            #ifdef ASP_DEBUG
            puts(opCode == OpCode_SET ? "SET" : "SETP");
            #endif

            /* Obtain destination from the stack. */
            AspDataEntry *address = AspTop(engine);
            if (address == 0)
                return AspRunResult_StackUnderflow;
            if (AspIsObject(address))
                AspRef(engine, address);
            AspPop(engine);

            /* Access value entry on the top of the stack. */
            AspDataEntry *newValue = AspTop(engine);
            if (newValue == 0)
                return AspRunResult_StackUnderflow;
            uint32_t newValueIndex = AspIndex(engine, newValue);

            AspRunResult assignResult =
                AspDataGetType(address) == DataType_Tuple ||
                AspDataGetType(address) == DataType_List ?
                AspAssignSequence(engine, address, newValue) :
                AspAssignSimple(engine, address, newValue);
            if (assignResult != AspRunResult_OK)
                return assignResult;

            if (opCode == OpCode_SETP)
                AspPop(engine);

            break;
        }

        case OpCode_ERASE:
        {
            #ifdef ASP_DEBUG
            puts("ERASE");
            #endif

            /* Access the index/key on top of the stack. */
            AspDataEntry *index = AspTop(engine);
            if (index == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(index))
                return AspRunResult_UnexpectedType;
            AspRef(engine, index);
            AspPop(engine);

            /* Access the container on top of the stack. */
            AspDataEntry *container = AspTop(engine);
            if (container == 0)
                return AspRunResult_StackUnderflow;
            AspRef(engine, container);
            AspPop(engine);
            switch (AspDataGetType(container))
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_List:
                {
                    switch (AspDataGetType(index))
                    {
                        default:
                            return AspRunResult_UnexpectedType;

                        case DataType_Integer:
                        {
                            int32_t indexValue = AspDataGetInteger(index);

                            /* Erase the element. */
                            bool eraseResult = AspSequenceErase
                                (engine, container, indexValue, true);
                            if (!eraseResult)
                                return AspRunResult_ValueOutOfRange;

                            break;
                        }

                        case DataType_Range:
                        {
                            int32_t startValue, endValue, stepValue;
                            AspGetRange
                                (engine, index,
                                 &startValue, &endValue, &stepValue);

                            /* Erase the elements selected by the slice. */
                            AspSequenceResult (*Navigate)
                                (AspEngine *, AspDataEntry *, AspDataEntry *) =
                                stepValue < 0 ?
                                AspSequencePrevious : AspSequenceNext;
                            int32_t i = stepValue < 0 ? -1 : 0;
                            int32_t increment= stepValue < 0 ? -1 : +1;
                            int32_t select = stepValue < 0 ?
                                startValue + 1 : startValue;
                            AspDataEntry *selectedElement = 0;
                            for (AspSequenceResult nextResult =
                                 Navigate(engine, container, 0);
                                 nextResult.element != 0 &&
                                 (stepValue < 0 ? i > endValue : i < endValue);
                                 i += increment, select -= increment,
                                 nextResult = Navigate
                                    (engine, container, nextResult.element))
                            {
                                /* Erase the previously selected element,
                                   if applicable. */
                                if (selectedElement != 0)
                                {
                                    bool eraseResult = AspSequenceEraseElement
                                        (engine, container,
                                         selectedElement, true);
                                    if (!eraseResult)
                                        return AspRunResult_ValueOutOfRange;
                                    selectedElement = 0;
                                }

                                /* Skip if not selected. */
                                if (select != 0)
                                    continue;

                                /* Select the element for deletion. */
                                selectedElement = nextResult.element;

                                /* Prepare to identify the next element. */
                                select = stepValue;
                            }

                            /* Erase the last selected element,
                               if applicable. */
                            if (selectedElement != 0)
                            {
                                bool eraseResult = AspSequenceEraseElement
                                    (engine, container, selectedElement, true);
                                if (!eraseResult)
                                    return AspRunResult_ValueOutOfRange;
                                selectedElement = 0;
                            }

                            break;
                        }
                    }

                    break;
                }

                case DataType_Set:
                case DataType_Dictionary:
                {
                    /* Locate the entry. */
                    AspTreeResult findResult = AspTreeFind
                        (engine, container, index);
                    if (findResult.result != AspRunResult_OK)
                        return findResult.result;
                    if (findResult.node == 0)
                        return AspRunResult_KeyNotFound;

                    /* Erase the member. */
                    bool eraseResult = AspTreeEraseNode
                        (engine, container, findResult.node, true, true);
                    if (eraseResult != AspRunResult_OK)
                        return eraseResult;

                    break;
                }

                case DataType_Module:
                {
                    /* Access the module's namespace. */
                    AspDataEntry *ns = AspEntry
                        (engine, AspDataGetModuleNamespaceIndex(container));

                    /* Ensure the index is a symbol. */
                    if (AspDataGetType(index) != DataType_Integer)
                        return AspRunResult_UnexpectedType;
                    int32_t symbol = AspDataGetInteger(index);

                    /* Locate the entry. */
                    AspTreeResult findResult = AspFindSymbol
                        (engine, ns, symbol);
                    if (findResult.result != AspRunResult_OK)
                        return findResult.result;
                    if (findResult.node == 0)
                        return AspRunResult_KeyNotFound;

                    /* Erase the member. */
                    bool eraseResult = AspTreeEraseNode
                        (engine, ns, findResult.node, true, true);
                    if (eraseResult != AspRunResult_OK)
                        return eraseResult;

                    break;
                }
            }

            AspUnref(engine, index);
            AspUnref(engine, container);

            break;
        }

        case OpCode_DEL4:
            operandSize += 2;
        case OpCode_DEL2:
            operandSize++;
        case OpCode_DEL1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("DEL ");
            #endif

            /* Fetch the variable's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Look up the variable in the local namespace. */
            AspDataEntry *ns = engine->localNamespace;
            AspTreeResult findResult = AspFindSymbol
                (engine, ns, variableSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            AspDataEntry *node = findResult.node;

            /* Check whether there is a global override in place. */
            if (node != 0 && AspDataGetNamespaceNodeIsGlobal(node) &&
                engine->globalNamespace != engine->localNamespace)
            {
                ns = engine->globalNamespace;
                findResult = AspFindSymbol
                    (engine, ns, variableSymbol);
                if (findResult.result != AspRunResult_OK)
                    return findResult.result;
                node = findResult.node;
            }

            /* Ensure the variable was found. Note that we do not allow
               variables in the system namespace to be deleted. */
            if (node == 0)
                return AspRunResult_NameNotFound;

            /* Delete the variable from the namespace in which it was found. */
            AspRunResult eraseResult = AspTreeEraseNode
                (engine, ns, node, true, true);
            if (eraseResult != AspRunResult_OK)
                return eraseResult;

            break;
        }

        case OpCode_GLOB4:
            operandSize += 2;
        case OpCode_GLOB2:
            operandSize++;
        case OpCode_GLOB1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("GLOB ");
            #endif

            /* Fetch the variable's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Ensure we're in the context of a function. */
            if (engine->localNamespace == engine->globalNamespace)
                return AspRunResult_InvalidContext;

            /* Look up the variable in the local namespace. */
            AspTreeResult findResult = AspFindSymbol
                (engine, engine->localNamespace, variableSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            AspDataEntry *node = findResult.node;
            if (node != 0)
            {
                /* Ensure the variable is not already marked as global. */
                if (AspDataGetNamespaceNodeIsGlobal(node))
                    return AspRunResult_Redundant;
            }
            else
            {
                /* Create a local variable as a reference to the global
                   namespace. */
                AspTreeResult insertResult = AspTreeTryInsertBySymbol
                    (engine, engine->localNamespace,
                     variableSymbol, engine->noneSingleton);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
                node = insertResult.node;
            }

            /* Mark the variable with a global override. */
            AspDataSetNamespaceNodeIsGlobal(node, true);

            break;
        }

        case OpCode_LOC4:
            operandSize += 2;
        case OpCode_LOC2:
            operandSize++;
        case OpCode_LOC1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("LOC ");
            #endif

            /* Fetch the variable's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Ensure we're in the context of a function. */
            if (engine->localNamespace == engine->globalNamespace)
                return AspRunResult_InvalidContext;

            /* Look up the variable in the local namespace. */
            AspTreeResult findResult = AspFindSymbol
                (engine, engine->localNamespace, variableSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            AspDataEntry *node = findResult.node;
            if (node == 0)
                return AspRunResult_NameNotFound;

            /* Ensure that a global override is in place. */
            if (!AspDataGetNamespaceNodeIsGlobal(node))
                return AspRunResult_Redundant;

            /* Revert the variable's global override. */
            AspDataSetNamespaceNodeIsGlobal(node, false);

            break;
        }

        case OpCode_SITER:
        {
            #ifdef ASP_DEBUG
            puts("SITER");
            #endif

            /* Access the iterable on top of the stack. */
            AspDataEntry *iterable = AspTop(engine);
            if (index == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(iterable))
                return AspRunResult_UnexpectedType;

            /* Create an iterator entry. */
            AspDataEntry *iterator = AspAllocEntry
                (engine, DataType_Iterator);
            if (iterator == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetIteratorIterableIndex
                (iterator, AspIndex(engine, iterable));

            /* Set iterator specifics based on iterable. */
            AspDataEntry *member = 0;
            switch (AspDataGetType(iterable))
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_Range:
                {
                    /* Determine a start value. */
                    int32_t startValue, endValue, stepValue;
                    AspGetRange
                        (engine, iterable,
                         &startValue, &endValue, &stepValue);
                    bool atEnd = AspIsValueAtRangeEnd
                        (startValue, endValue, stepValue);

                    /* Create an integer set to the start value. */
                    if (!atEnd)
                    {
                        AspDataEntry *value = AspAllocEntry
                            (engine, DataType_Integer);
                        if (value == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetInteger(value, startValue);
                        AspDataSetIteratorMemberNeedsCleanup(iterator, true);
                        member = value;
                    }

                    break;
                }

                case DataType_String:
                case DataType_Tuple:
                case DataType_List:
                {
                    AspSequenceResult startResult = AspSequenceNext
                        (engine, iterable, 0);
                    if (startResult.result != AspRunResult_OK)
                        return startResult.result;
                    member = startResult.element;

                    break;
                }

                case DataType_Set:
                case DataType_Dictionary:
                {
                    AspTreeResult startResult = AspTreeNext
                        (engine, iterable, 0, true);
                    if (startResult.result != AspRunResult_OK)
                        return startResult.result;
                    member = startResult.node;

                    break;
                }
            }
            AspDataSetIteratorMemberIndex(iterator, AspIndex(engine, member));

            /* Replace the top stack entry with the iterator. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, iterator));

            break;
        }

        case OpCode_TITER:
        {
            #ifdef ASP_DEBUG
            puts("TITER");
            #endif

            /* Access the iterator on top of the stack. */
            AspDataEntry *iterator = AspTop(engine);
            if (iterator == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(iterator) != DataType_Iterator)
                return AspRunResult_UnexpectedType;

            /* Test the iterator and push the test result onto the stack. */
            AspDataEntry *testResult = AspNewBoolean
                (engine, AspDataGetIteratorMemberIndex(iterator) != 0);
            if (testResult == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataEntry *stackEntry = AspPush(engine, testResult);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, testResult);

            break;
        }

        case OpCode_NITER:
        {
            #ifdef ASP_DEBUG
            puts("NITER");
            #endif

            /* Access the iterator on top of the stack. */
            AspDataEntry *iterator = AspTop(engine);
            if (iterator == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(iterator) != DataType_Iterator)
                return AspRunResult_UnexpectedType;

            /* Gain access to the underlying iterable. */
            AspDataEntry *iterable = AspValueEntry
                (engine, AspDataGetIteratorIterableIndex(iterator));

            /* Check if the iterator is already at its end. */
            AspDataEntry *member = AspEntry
                (engine, AspDataGetIteratorMemberIndex(iterator));
            if (member == 0)
                break;

            /* Advance the iterator. */
            switch (AspDataGetType(iterable))
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_Range:
                {
                    int32_t endValue, stepValue;

                    if (AspDataGetType(member) != DataType_Integer)
                        return AspRunResult_UnexpectedType;

                    AspGetRange(engine, iterable, 0, &endValue, &stepValue);
                    int32_t newValue = AspDataGetInteger(member) + stepValue;
                    bool atEnd = AspIsValueAtRangeEnd
                        (newValue, endValue, stepValue);
                    if (atEnd)
                    {
                        AspUnref(engine, member);
                        AspDataSetIteratorMemberNeedsCleanup(iterator, false);
                        member = 0;
                    }
                    else
                        AspDataSetInteger(member, newValue);

                    break;
                }

                case DataType_String:
                {
                    AspDataEntry *element = AspEntry
                        (engine, AspDataGetIteratorMemberIndex(iterator));
                    if (AspDataGetType(element) != DataType_Element)
                        return AspRunResult_UnexpectedType;
                    AspDataEntry *fragment = AspEntry
                        (engine, AspDataGetElementValueIndex(element));
                    if (AspDataGetType(fragment) != DataType_StringFragment)
                        return AspRunResult_UnexpectedType;
                    uint8_t fragmentSize =
                        AspDataGetStringFragmentSize(fragment);

                    uint8_t stringIndex =
                        AspDataGetIteratorStringIndex(iterator);
                    if (stringIndex + 1 < fragmentSize)
                    {
                        AspDataSetIteratorStringIndex
                            (iterator, stringIndex + 1);
                        break;
                    }

                    AspDataSetIteratorStringIndex(iterator, 0);

                    /* Fall through... */
                }

                case DataType_Tuple:
                case DataType_List:
                {
                    if (AspDataGetType(member) != DataType_Element)
                        return AspRunResult_UnexpectedType;
                    AspSequenceResult nextResult = AspSequenceNext
                        (engine, iterable, member);
                    if (nextResult.result != AspRunResult_OK)
                        return nextResult.result;
                    member = nextResult.element;

                    break;
                }

                case DataType_Set:
                case DataType_Dictionary:
                {
                    uint8_t memberType = AspDataGetType(member);
                    if (memberType != DataType_SetNode &&
                        memberType != DataType_DictionaryNode)
                        return AspRunResult_UnexpectedType;
                    AspTreeResult nextResult = AspTreeNext
                        (engine, iterable, member, true);
                    if (nextResult.result != AspRunResult_OK)
                        return nextResult.result;
                    member = nextResult.node;

                    break;
                }
            }

            /* Update the iterator on the top of the stack. */
            AspDataSetIteratorMemberIndex(iterator, AspIndex(engine, member));

            break;
        }

        case OpCode_DITER:
        {
            #ifdef ASP_DEBUG
            puts("DITER");
            #endif

            /* Access the iterator on top of the stack. */
            AspDataEntry *iterator = AspTop(engine);
            if (iterator == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(iterator) != DataType_Iterator)
                return AspRunResult_UnexpectedType;

            /* Gain access to the underlying iterable and current member. */
            AspDataEntry *iterable = AspValueEntry
                (engine, AspDataGetIteratorIterableIndex(iterator));
            AspDataEntry *member = AspValueEntry
                (engine, AspDataGetIteratorMemberIndex(iterator));
            if (member == 0)
                return AspRunResult_IteratorAtEnd;

            /* Dereference the iterator. */
            AspDataEntry *value = 0;
            bool newValue = false;
            switch (AspDataGetType(iterable))
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_Range:
                {
                    if (AspDataGetType(member) != DataType_Integer)
                        return AspRunResult_UnexpectedType;
                    value = member;
                    break;
                }

                case DataType_String:
                {
                    AspDataEntry *fragment = AspValueEntry
                        (engine, AspDataGetElementValueIndex(member));
                    if (AspDataGetType(fragment) != DataType_StringFragment)
                        return AspRunResult_UnexpectedType;
                    uint8_t stringIndex =
                        AspDataGetIteratorStringIndex(iterator);
                    const uint8_t *stringData =
                        AspDataGetStringFragmentData(fragment);
                    uint8_t c = stringData[stringIndex];

                    AspDataEntry *resultString = AspAllocEntry
                        (engine, DataType_String);
                    if (resultString == 0)
                        return AspRunResult_OutOfDataMemory;

                    AspDataEntry *newFragment =
                        AspAllocEntry(engine, DataType_StringFragment);
                    if (newFragment == 0)
                        return AspRunResult_OutOfDataMemory;
                    AspDataSetStringFragment(newFragment, &c , 1);

                    AspSequenceResult appendResult = AspSequenceAppend
                        (engine, resultString, newFragment);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;

                    value = resultString;
                    newValue = true;

                    break;
                }

                case DataType_Tuple:
                case DataType_List:
                    value = AspValueEntry
                        (engine, AspDataGetElementValueIndex(member));
                    break;

                case DataType_Set:
                    value = AspValueEntry
                        (engine, AspDataGetTreeNodeKeyIndex(member));
                    break;

                case DataType_Dictionary:
                {
                    AspDataEntry *key = AspValueEntry
                        (engine, AspDataGetTreeNodeKeyIndex(member));
                    AspDataEntry *entryValue = AspValueEntry
                        (engine, AspDataGetTreeNodeValueIndex(member));

                    AspDataEntry *tuple =
                        AspAllocEntry(engine, DataType_Tuple);
                    if (tuple == 0)
                        return AspRunResult_OutOfDataMemory;

                    AspSequenceResult keyAppendResult = AspSequenceAppend
                        (engine, tuple, key);
                    if (keyAppendResult.result != AspRunResult_OK)
                        return keyAppendResult.result;
                    AspSequenceResult valueAppendResult = AspSequenceAppend
                        (engine, tuple, entryValue);
                    if (valueAppendResult.result != AspRunResult_OK)
                        return valueAppendResult.result;

                    value = tuple;
                    newValue = true;

                    break;
                }
            }

            /* Push the dereferenced value onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, value);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            if (newValue)
                AspUnref(engine, value);

            break;
        }

        case OpCode_NOOP:
            #ifdef ASP_DEBUG
            puts("NOOP");
            #endif

            break;

        case OpCode_JMPF:
        case OpCode_JMPT:
        case OpCode_JMP:
        case OpCode_LOR:
        case OpCode_LAND:
        {
            #ifdef ASP_DEBUG
            printf
                ("%s ",
                 opCode == OpCode_JMPF ? "JMPF" :
                 opCode == OpCode_JMPT ? "JMPT" :
                 opCode == OpCode_LOR ? "LOR" :
                 opCode == OpCode_LAND ? "LAND" : "JMP");
            #endif

            /* Fetch the code address from the operand. */
            uint32_t codeAddress = 0;
            AspRunResult operandLoadResult = LoadUnsignedOperand
                (engine, 4, &codeAddress);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("@0x%7.7X\n", codeAddress);
            #endif
            if (codeAddress >= engine->codeEndIndex)
                return AspRunResult_BeyondEndOfCode;

            /* Determine condition if applicable. */
            bool condition = true;
            if (opCode != OpCode_JMP)
            {
                AspDataEntry *value = AspTop(engine);
                if (value == 0)
                    return AspRunResult_StackUnderflow;
                if (!AspIsObject(value))
                    return AspRunResult_UnexpectedType;
                condition = AspIsTrue(engine, value);

                /* Pop value off the stack if applicable. */
                if (opCode == OpCode_JMPF || opCode == OpCode_JMPT ||
                    (opCode == OpCode_LOR && !condition) ||
                    (opCode == OpCode_LAND && condition))
                    AspPop(engine);
            }

            /* Transfer control to the code address if applicable. */
            if (condition == (opCode != OpCode_JMPF && opCode != OpCode_LAND))
                engine->pc = engine->code + codeAddress;

            break;
        }

        case OpCode_CALL:
        {
            #ifdef ASP_DEBUG
            puts("CALL");
            #endif

            AspDataEntry *function = 0, *ns = 0;
            if (!engine->again)
            {
                /* Pop the function off the stack. */
                function = AspTop(engine);
                if (function == 0)
                    return AspRunResult_StackUnderflow;
                if (AspDataGetType(function) != DataType_Function)
                    return AspRunResult_UnexpectedType;
                AspRef(engine, function);
                AspPop(engine);

                /* Pop argument list off the stack. */
                AspDataEntry *arguments = AspTop(engine);
                if (arguments == 0)
                    return AspRunResult_StackUnderflow;
                if (AspDataGetType(arguments) != DataType_ArgumentList)
                    return AspRunResult_UnexpectedType;
                AspPop(engine);

                /* Gain access to the parameter list within the function. */
                AspDataEntry *parameters = AspEntry
                    (engine, AspDataGetFunctionParametersIndex(function));
                if (AspDataGetType(parameters) != DataType_ParameterList)
                    return AspRunResult_UnexpectedType;

                /* Create a local namespace for the call. */
                ns = AspAllocEntry(engine, DataType_Namespace);
                if (ns == 0)
                    return AspRunResult_OutOfDataMemory;
                AspRunResult loadArgumentsResult = AspLoadArguments
                    (engine, arguments, parameters, ns);
                if (loadArgumentsResult != AspRunResult_OK)
                    return loadArgumentsResult;
                AspUnref(engine, arguments);
            }

            /* Call the function. */
            if (engine->again || AspDataGetFunctionIsApp(function))
            {
                if (!engine->again)
                {
                    engine->appFunctionSymbol = AspDataGetFunctionSymbol
                        (function);
                    engine->appFunctionNamespace = ns;
                    engine->appFunctionReturnValue = 0;
                }

                /* Call the application function. */
                engine->inApp = true;
                AspRunResult callResult = engine->appSpec->dispatch
                    (engine,
                     engine->appFunctionSymbol, engine->appFunctionNamespace,
                     &engine->appFunctionReturnValue);
                engine->inApp = false;
                if (callResult == AspRunResult_OK)
                    engine->again = false;
                else if (callResult == AspRunResult_Again)
                {
                    /* Cause this instruction to execute again. */
                    engine->pc = pc;
                    engine->again = true;
                    callResult = AspRunResult_OK;
                }
                if (callResult != AspRunResult_OK)
                {
                    if (callResult == AspRunResult_Complete)
                        callResult = AspRunResult_MalformedFunctionCall;
                    return callResult;
                }

                if (!engine->again)
                {
                    /* We're now done with the local namespace. */
                    AspUnref(engine, engine->appFunctionNamespace);
                    engine->appFunctionSymbol = 0;
                    engine->appFunctionNamespace = 0;

                    /* Ensure there's a return value and push it onto the
                       stack. */
                    AspDataEntry *returnValue = engine->appFunctionReturnValue;
                    engine->appFunctionReturnValue = 0;
                    if (returnValue == 0)
                        returnValue = AspAllocEntry(engine, DataType_None);
                    AspDataEntry *stackEntry = AspPush(engine, returnValue);
                    if (stackEntry == 0)
                        return AspRunResult_OutOfDataMemory;
                    AspUnref(engine, returnValue);
                }
            }
            else
            {
                /* Obtain the code address of the script-defined function. */
                uint32_t codeAddress = AspDataGetFunctionCodeAddress(function);
                if (codeAddress >= engine->codeEndIndex)
                    return AspRunResult_BeyondEndOfCode;

                /* Create a new frame and push it onto the stack. */
                AspDataEntry *frame = AspAllocEntry(engine, DataType_Frame);
                if (frame == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetFrameReturnAddress
                    (frame, AspProgramCounter(engine));
                AspRef(engine, engine->module);
                AspDataSetFrameModuleIndex
                    (frame, AspIndex(engine, engine->module));
                AspDataSetFrameLocalNamespaceIndex
                    (frame, AspIndex(engine, engine->localNamespace));
                AspDataEntry *newTop = AspPush(engine, frame);
                if (newTop == 0)
                    return AspRunResult_OutOfDataMemory;

                /* Replace the current module and global namespace with
                   those of the function. */
                AspDataEntry *functionModule = AspValueEntry
                    (engine, AspDataGetFunctionModuleIndex(function));
                engine->module = functionModule;
                engine->globalNamespace = AspEntry
                    (engine, AspDataGetModuleNamespaceIndex(functionModule));

                /* Replace the current local namespace with function's new
                   namespace. */
                engine->localNamespace = ns;

                /* Transfer control to the function's code. */
                engine->pc = engine->code + codeAddress;
            }

            if (function != 0)
                AspUnref(engine, function);

            break;
        }

        case OpCode_RET:
        {
            #ifdef ASP_DEBUG
            puts("RET");
            #endif

            /* Access the return value on top of the stack. */
            AspDataEntry *returnValue = AspTop(engine);
            if (returnValue == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(returnValue))
                return AspRunResult_UnexpectedType;
            AspRef(engine, returnValue);
            AspPop(engine);

            /* Discard the function's local namespace. */
            AspUnref(engine, engine->localNamespace);

            /* Access the frame on top of the stack. */
            AspDataEntry *frame = AspTop(engine);
            if (frame == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(frame) != DataType_Frame)
                return AspRunResult_UnexpectedType;

            /* Restore the caller's local namespace. */
            engine->localNamespace = AspEntry
                (engine, AspDataGetFrameLocalNamespaceIndex(frame));

            /* Restore the caller's global namespace and module. */
            AspDataEntry *module = AspEntry
                (engine, AspDataGetFrameModuleIndex(frame));
            engine->globalNamespace = AspValueEntry
                (engine, AspDataGetModuleNamespaceIndex(module));
            engine->module = module;

            /* Save the return address. */
            uint32_t returnAddress = AspDataGetFrameReturnAddress(frame);

            /* Pop the frame off the stack. */
            AspPop(engine);
            AspUnref(engine, frame);

            /* Push the return value onto the stack. There is no need to check
               for success because we just popped things off the stack. */
            AspPush(engine, returnValue);
            AspUnref(engine, returnValue);

            /* Return control back to the caller. */
            engine->pc = engine->code + returnAddress;

            break;
        }

        case OpCode_ADDMOD4:
            operandSize += 2;
        case OpCode_ADDMOD2:
            operandSize++;
        case OpCode_ADDMOD1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("ADDMOD ");
            #endif

            /* Fetch the module's symbol from the first operand. */
            int32_t moduleSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &moduleSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?, ?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d, @", moduleSymbol);
            #endif

            /* Fetch the module's code address from the second operand. */
            uint32_t codeAddress = 0;
            operandLoadResult = LoadUnsignedOperand
                (engine, 4, &codeAddress);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("0x%7.7X\n", codeAddress);
            #endif
            if (codeAddress >= engine->codeEndIndex)
                return AspRunResult_BeyondEndOfCode;

            /* Create a namespace for the module. */
            AspDataEntry *ns = AspAllocEntry(engine, DataType_Namespace);
            if (ns == 0)
                return AspRunResult_OutOfDataMemory;

            /* Add an entry for the system module to the module's namespace. */
            if (moduleSymbol != AspSystemModuleSymbol)
            {
                AspTreeResult addSystemResult = AspTreeTryInsertBySymbol
                    (engine, ns, AspSystemModuleSymbol, engine->systemModule);
                if (addSystemResult.result != AspRunResult_OK)
                    return addSystemResult.result;
            }

            /* Create the module. */
            AspDataEntry *module = AspAllocEntry(engine, DataType_Module);
            if (module == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetModuleCodeAddress(module, codeAddress);
            AspDataSetModuleNamespaceIndex(module, AspIndex(engine, ns));

            /* Add the module to the engine. */
            AspTreeResult addResult = AspTreeTryInsertBySymbol
                (engine, engine->modules, moduleSymbol, module);
            if (addResult.result != AspRunResult_OK)
                return addResult.result;
            if (addResult.inserted)
                AspUnref(engine, module);

            break;
        }

        case OpCode_XMOD:
        {
            #ifdef ASP_DEBUG
            puts("XMOD");
            #endif

            /* Access the frame on top of the stack. */
            AspDataEntry *frame = AspTop(engine);
            if (frame == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(frame) != DataType_Frame)
                return AspRunResult_UnexpectedType;

            /* Restore the loader's namespaces and module. */
            engine->localNamespace = AspEntry
                (engine, AspDataGetFrameLocalNamespaceIndex(frame));

            /* Restore the caller's global namespace and module. */
            AspDataEntry *module = AspEntry
                (engine, AspDataGetFrameModuleIndex(frame));
            engine->globalNamespace = AspValueEntry
                (engine, AspDataGetModuleNamespaceIndex(module));
            engine->module = module;

            /* Save the return address. */
            uint32_t returnAddress = AspDataGetFrameReturnAddress(frame);

            /* Pop the frame off the stack. */
            AspPop(engine);
            AspUnref(engine, frame);

            /* Return control to the caller. */
            engine->pc = engine->code + returnAddress;

            break;
        }

        case OpCode_LDMOD4:
            operandSize += 2;
        case OpCode_LDMOD2:
            operandSize++;
        case OpCode_LDMOD1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("LDMOD ");
            #endif

            /* Fetch the module's symbol from the operand. */
            int32_t moduleSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &moduleSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", moduleSymbol);
            #endif

            /* Look up the module. */
            AspTreeResult findResult = AspFindSymbol
                (engine, engine->modules, moduleSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            if (findResult.node == 0)
                return AspRunResult_NameNotFound;
            if (AspDataGetType(findResult.value) != DataType_Module)
                return AspRunResult_UnexpectedType;
            AspDataEntry *module = findResult.value;

            /* Run the module on first load. */
            if (AspDataGetModuleIsLoaded(module))
                break;
            AspDataSetModuleIsLoaded(module, true);

            /* Create a new frame and push it onto the stack. */
            AspDataEntry *frame = AspAllocEntry(engine, DataType_Frame);
            if (frame == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetFrameReturnAddress
                (frame, AspProgramCounter(engine));
            AspRef(engine, engine->module);
            AspDataSetFrameModuleIndex
                (frame, AspIndex(engine, engine->module));
            AspDataSetFrameLocalNamespaceIndex
                (frame, AspIndex(engine, engine->localNamespace));
            AspDataEntry *newTop = AspPush(engine, frame);
            if (newTop == 0)
                return AspRunResult_OutOfDataMemory;

            /* Replace the global and local namespaces with those of the
               module. */
            engine->module = module;
            engine->globalNamespace = AspValueEntry
                (engine, AspDataGetModuleNamespaceIndex(module));
            engine->localNamespace = engine->globalNamespace;

            /* Transfer control to the module's code. */
            engine->pc = engine->code + AspDataGetModuleCodeAddress(module);

            break;
        }

        case OpCode_MKARG:
        case OpCode_MKGARG:
        {
            bool isGroup = opCode == OpCode_MKGARG;

            #ifdef ASP_DEBUG
            puts(isGroup ? "MKGARG" : "MKARG");
            #endif

            /* Access argument value on top of the stack. */
            AspDataEntry *top = AspTop(engine);
            if (top == 0)
                return AspRunResult_StackUnderflow;

            /* For a group argument, ensure the value is a tuple. */
            if (isGroup && AspDataGetType(top) != DataType_Tuple)
                return AspRunResult_UnexpectedType;

            /* Create an argument. */
            AspDataEntry *argument = AspAllocEntry(engine, DataType_Argument);
            if (argument == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetArgumentValueIndex(argument, AspIndex(engine, top));
            AspDataSetArgumentIsGroup(argument, isGroup);

            /* Replace the top stack entry with the argument. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, argument));

            break;
        }

        case OpCode_MKNARG4:
            operandSize += 2;
        case OpCode_MKNARG2:
            operandSize++;
        case OpCode_MKNARG1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("MKNARG ");
            #endif

            /* Fetch the argument's symbol from the operand. */
            int32_t argumentSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &argumentSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", argumentSymbol);
            #endif

            /* Access argument value on top of the stack. */
            AspDataEntry *top = AspTop(engine);
            if (top == 0)
                return AspRunResult_StackUnderflow;

            /* Create an argument. */
            AspDataEntry *argument = AspAllocEntry(engine, DataType_Argument);
            if (argument == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetArgumentHasName(argument, true);
            AspDataSetArgumentSymbol(argument, argumentSymbol);
            AspDataSetArgumentValueIndex(argument, AspIndex(engine, top));

            /* Replace the top stack entry with the argument. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, argument));

            break;
        }

        case OpCode_MKPAR4:
        case OpCode_MKGPAR4:
            operandSize += 2;
        case OpCode_MKPAR2:
        case OpCode_MKGPAR2:
            operandSize++;
        case OpCode_MKPAR1:
        case OpCode_MKGPAR1:
        {
            operandSize++;

            bool isGroup =
                opCode == OpCode_MKGPAR1 ||
                opCode == OpCode_MKGPAR2 ||
                opCode == OpCode_MKGPAR4;

            #ifdef ASP_DEBUG
            printf("%s ", isGroup ? "MKGPAR" : "MKPAR");
            #endif

            /* Fetch the parameter's symbol from the operand. */
            int32_t parameterSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &parameterSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", parameterSymbol);
            #endif

            /* Create a parameter. */
            AspDataEntry *parameter = AspAllocEntry(engine, DataType_Parameter);
            if (parameter == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetParameterSymbol(parameter, parameterSymbol);
            AspDataSetParameterIsGroup(parameter, isGroup);

            /* Push the parameter onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, parameter);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            break;
        }

        case OpCode_MKDPAR4:
            operandSize += 2;
        case OpCode_MKDPAR2:
            operandSize++;
        case OpCode_MKDPAR1:
        {
            operandSize++;

            #ifdef ASP_DEBUG
            printf("MKDPAR ");
            #endif

            /* Fetch the parameter's symbol from the operand. */
            int32_t parameterSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &parameterSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", parameterSymbol);
            #endif

            /* Access parameter default value on top of the stack. */
            AspDataEntry *defaultValue = AspTop(engine);
            if (defaultValue == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(defaultValue))
                return AspRunResult_UnexpectedType;

            /* Create a parameter with a default. */
            AspDataEntry *parameter = AspAllocEntry(engine, DataType_Parameter);
            if (parameter == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetParameterSymbol(parameter, parameterSymbol);
            AspDataSetParameterHasDefault(parameter, true);
            AspDataSetParameterDefaultIndex
                (parameter, AspIndex(engine, defaultValue));

            /* Replace the top stack entry with the parameter. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, parameter));

            break;
        }

        case OpCode_MKFUN:
        {
            #ifdef ASP_DEBUG
            printf("MKFUN @");
            #endif

            /* Access code address on top of the stack. */
            AspDataEntry *codeAddressEntry = AspTop(engine);
            if (codeAddressEntry == 0)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return AspRunResult_StackUnderflow;
            }
            if (AspDataGetType(codeAddressEntry) != DataType_CodeAddress)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return AspRunResult_UnexpectedType;
            }
            uint32_t codeAddress = AspDataGetCodeAddress(codeAddressEntry);
            #ifdef ASP_DEBUG
            printf("0x%7.7X\n", codeAddress);
            #endif
            if (codeAddress >= engine->codeEndIndex)
                return AspRunResult_BeyondEndOfCode;
            AspPop(engine);
            AspUnref(engine, codeAddressEntry);

            /* Access parameter list on top of the stack. */
            AspDataEntry *parameters = AspTop(engine);
            if (parameters == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(parameters) != DataType_ParameterList)
                return AspRunResult_UnexpectedType;

            /* Create a function. */
            AspDataEntry *function = AspAllocEntry(engine, DataType_Function);
            if (function == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetFunctionCodeAddress(function, codeAddress);
            AspDataSetFunctionModuleIndex
                (function, AspIndex(engine, engine->module));
            AspDataSetFunctionParametersIndex
                (function, AspIndex(engine, parameters));

            /* Replace the top stack entry with the function. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, function));

            break;
        }

        case OpCode_MKKVP:
        {
            #ifdef ASP_DEBUG
            puts("MKKVP");
            #endif

            /* Access the key on top of the stack. */
            AspDataEntry *key = AspTop(engine);
            if (key == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(key))
                return AspRunResult_UnexpectedType;
            AspRef(engine, key);
            AspPop(engine);

            /* Access the value on top of the stack. */
            AspDataEntry *value = AspTop(engine);
            if (value == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(value))
                return AspRunResult_UnexpectedType;

            /* Create a key value pair entry. */
            AspDataEntry *keyValuePairEntry = AspAllocEntry
                (engine, DataType_KeyValuePair);
            if (keyValuePairEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetKeyValuePairKeyIndex
                (keyValuePairEntry, AspIndex(engine, key));
            AspDataSetKeyValuePairValueIndex
                (keyValuePairEntry, AspIndex(engine, value));

            /* Replace the top stack entry with the dictionary entry. */
            AspDataSetStackEntryValueIndex
                (engine->stackTop, AspIndex(engine, keyValuePairEntry));

            break;
        }

        case OpCode_MKR0:
        case OpCode_MKRS:
        case OpCode_MKRE:
        case OpCode_MKRSE:
        case OpCode_MKRT:
        case OpCode_MKRST:
        case OpCode_MKRET:
        case OpCode_MKR:
        {
            bool hasStart =
                opCode == OpCode_MKRS ||
                opCode == OpCode_MKRSE ||
                opCode == OpCode_MKRST ||
                opCode == OpCode_MKR;
            bool hasEnd =
                opCode == OpCode_MKRE ||
                opCode == OpCode_MKRSE ||
                opCode == OpCode_MKRET ||
                opCode == OpCode_MKR;
            bool hasStep =
                opCode == OpCode_MKRT ||
                opCode == OpCode_MKRST ||
                opCode == OpCode_MKRET ||
                opCode == OpCode_MKR;
            #ifdef ASP_DEBUG
            printf("MKR");
            if (!hasStart || !hasEnd || !hasStep)
            {
                if (hasStart)
                    putchar('S');
                if (hasEnd)
                    putchar('E');
                if (hasStep)
                    putchar('T');
            }
            putchar('\n');
            #endif

            /* Access the start, end, and step entries on top of the stack,
               as applicable. */
            AspDataEntry *start = 0, *end = 0, *step = 0;
            if (hasStart)
            {
                start = AspTop(engine);
                if (start == 0)
                    return AspRunResult_StackUnderflow;
                if (AspDataGetType(start) != DataType_Integer)
                    return AspRunResult_UnexpectedType;
                AspRef(engine, start);
                AspPop(engine);
            }
            if (hasEnd)
            {
                end = AspTop(engine);
                if (end == 0)
                    return AspRunResult_StackUnderflow;
                if (AspDataGetType(end) != DataType_Integer)
                    return AspRunResult_UnexpectedType;
                AspRef(engine, end);
                AspPop(engine);
            }
            if (hasStep)
            {
                step = AspTop(engine);
                if (step == 0)
                    return AspRunResult_StackUnderflow;
                if (AspDataGetType(step) != DataType_Integer)
                    return AspRunResult_UnexpectedType;
                AspRef(engine, step);
                AspPop(engine);
            }

            /* Create a range entry. */
            AspDataEntry *range = AspAllocEntry(engine, DataType_Range);
            if (range == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetRangeHasStart(range, hasStart);
            if (hasStart)
                AspDataSetRangeStartIndex(range, AspIndex(engine, start));
            AspDataSetRangeHasEnd(range, hasEnd);
            if (hasEnd)
                AspDataSetRangeEndIndex(range, AspIndex(engine, end));
            AspDataSetRangeHasStep(range, hasStep);
            if (hasStep)
                AspDataSetRangeStepIndex(range, AspIndex(engine, step));

            /* Push the range onto the stack. */
            AspDataEntry *stackEntry = AspPush(engine, range);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, range);

            break;
        }

        case OpCode_INS:
        case OpCode_INSP:
        case OpCode_BLD:
        {
            #ifdef ASP_DEBUG
            puts
                (opCode == OpCode_BLD ? "BLD" :
                 opCode == OpCode_INS ? "INS" : "INSP");
            #endif

            /* Access item on top of the stack to insert. */
            AspDataEntry *item = AspTop(engine);
            if (item == 0)
                return AspRunResult_StackUnderflow;
            AspRef(engine, item);
            AspPop(engine);
            uint8_t itemType = AspDataGetType(item);

            /* Access object on top of the stack to build/insert into. */
            AspDataEntry *container = AspTop(engine);
            if (container == 0)
                return AspRunResult_StackUnderflow;

            /* Ensure item is of an expected type. */
            AspDataEntry *key = 0, *value = 0;
            uint8_t containerType = AspDataGetType(container);
            switch (containerType)
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_Tuple:
                    if (opCode != OpCode_BLD)
                        return AspRunResult_UnexpectedType;
                    if (!AspIsObject(item) &&
                        itemType != DataType_DictionaryNode &&
                        itemType != DataType_NamespaceNode &&
                        itemType != DataType_Element)
                        return AspRunResult_UnexpectedType;
                    break;

                case DataType_ParameterList:
                    if (opCode != OpCode_BLD ||
                        itemType != DataType_Parameter)
                        return AspRunResult_UnexpectedType;
                    break;

                case DataType_ArgumentList:
                    if (opCode != OpCode_BLD ||
                        itemType != DataType_Argument)
                        return AspRunResult_UnexpectedType;
                    break;

                case DataType_Set:
                    if (!AspIsObject(item))
                        return AspRunResult_UnexpectedType;
                    break;

                case DataType_List:
                    if (itemType != DataType_KeyValuePair &&
                        !AspIsObject(item))
                        return AspRunResult_UnexpectedType;
                    if (itemType != DataType_KeyValuePair)
                        break;

                    /* Fall through... */

                case DataType_Dictionary:
                    if (itemType != DataType_KeyValuePair)
                        return AspRunResult_UnexpectedType;
                    key = AspValueEntry
                        (engine, AspDataGetKeyValuePairKeyIndex(item));
                    if (!AspIsImmutableObject(key))
                        return AspRunResult_UnexpectedType;
                    value = AspValueEntry
                        (engine, AspDataGetKeyValuePairValueIndex(item));
                    if (!AspIsObject(value))
                        return AspRunResult_UnexpectedType;
                    break;
            }

            /* Add the item into the container. */
            switch (containerType)
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_ArgumentList:
                {
                    if (AspDataGetArgumentIsGroup(item))
                    {
                        /* Access the underlying tuple. */
                        AspDataEntry *tuple = AspValueEntry
                            (engine, AspDataGetArgumentValueIndex(item));
                        if (AspDataGetType(tuple) != DataType_Tuple)
                            return AspRunResult_UnexpectedType;

                        /* Add each value in the group as an argument. */
                        for (AspSequenceResult nextResult =
                             AspSequenceNext(engine, tuple, 0);
                             nextResult.element != 0;
                             nextResult = AspSequenceNext
                                (engine, tuple, nextResult.element))
                        {
                            AspDataEntry *value = nextResult.value;

                            /* Create an argument. */
                            AspDataEntry *argument = AspAllocEntry
                                (engine, DataType_Argument);
                            if (argument == 0)
                                return AspRunResult_OutOfDataMemory;
                            AspDataSetArgumentValueIndex
                                (argument, AspIndex(engine, value));
                            AspRef(engine, value);

                            /* Append the argument to the list. */
                            AspSequenceResult appendResult = AspSequenceAppend
                                (engine, container, argument);
                            if (appendResult.result != AspRunResult_OK)
                                return appendResult.result;
                        }

                        AspUnref(engine, item);

                        break;
                    }

                    /* Fall through... */
                }

                case DataType_Tuple:
                case DataType_ParameterList:
                {
                    AspSequenceResult appendResult = AspSequenceAppend
                        (engine, container, item);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;

                    break;
                }

                case DataType_List:
                {
                    AspSequenceResult insertResult =
                        {AspRunResult_InternalError, 0, 0};
                    if (key == 0)
                    {
                        /* Append the entry. */
                        insertResult = AspSequenceAppend
                            (engine, container, item);
                    }
                    else
                    {
                        /* Ensure the index is an integer. */
                        if (AspDataGetType(key) != DataType_Integer)
                            return AspRunResult_UnexpectedType;
                        int32_t index = AspDataGetInteger(key);

                        /* Insert the value at the given index. */
                        insertResult = AspSequenceInsertByIndex
                            (engine, container, index, value);
                    }

                    if (insertResult.result != AspRunResult_OK)
                        return insertResult.result;

                    break;
                }

                case DataType_Set:
                {
                    AspTreeResult insertResult = AspTreeInsert
                        (engine, container, item, 0);
                    if (insertResult.result != AspRunResult_OK)
                        return insertResult.result;

                    break;
                }

                case DataType_Dictionary:
                {
                    AspTreeResult insertResult = AspTreeInsert
                        (engine, container, key, value);
                    if (insertResult.result != AspRunResult_OK)
                        return insertResult.result;
                    AspUnref(engine, item);

                    break;
                }
            }

            if (AspIsObject(item))
                AspUnref(engine, item);

            if (opCode == OpCode_INSP)
                AspPop(engine);

            break;
        }

        case OpCode_IDX:
        case OpCode_IDXA:
        {
            #ifdef ASP_DEBUG
            puts(opCode == OpCode_IDXA ? "IDXA" : "IDX");
            #endif

            /* Access the index/key on top of the stack. */
            AspDataEntry *index = AspTop(engine);
            if (index == 0)
                return AspRunResult_StackUnderflow;
            if (!AspIsObject(index))
                return AspRunResult_UnexpectedType;
            AspRef(engine, index);
            AspPop(engine);

            /* Access the container on top of the stack. */
            AspDataEntry *container = AspTop(engine);
            if (container == 0)
                return AspRunResult_StackUnderflow;
            AspRef(engine, container);
            AspPop(engine);
            DataType containerType = AspDataGetType(container);
            switch (containerType)
            {
                default:
                    return AspRunResult_UnexpectedType;

                case DataType_String:
                {
                    if (opCode == OpCode_IDXA)
                        return AspRunResult_UnexpectedType;

                    switch (AspDataGetType(index))
                    {
                        default:
                            return AspRunResult_UnexpectedType;

                        case DataType_Integer:
                        {
                            int32_t indexValue = AspDataGetInteger(index);

                            /* Get the indexed character. */
                            char c = AspStringElement
                                (engine, container, indexValue);
                            if (c == 0)
                            {
                                uint32_t count = AspDataGetSequenceCount
                                    (container);
                                if (indexValue >= count ||
                                    indexValue < -(int32_t)count)
                                    return AspRunResult_ValueOutOfRange;
                            }

                            /* Create a single-character string fragment. */
                            AspDataEntry *fragment =
                                AspAllocEntry(engine, DataType_StringFragment);
                            if (fragment == 0)
                                return AspRunResult_OutOfDataMemory;
                            AspDataSetStringFragment(fragment, &c, 1);

                            /* Create a new string. */
                            AspDataEntry *element = AspAllocEntry
                                (engine, DataType_String);
                            if (element == 0)
                                return AspRunResult_OutOfDataMemory;

                            /* Add the single-character fragment to the new
                               string. */
                            AspSequenceResult appendResult = AspSequenceAppend
                                (engine, element, fragment);
                            if (appendResult.result != AspRunResult_OK)
                                return appendResult.result;

                            /* Push the single-character string. */
                            AspDataEntry *stackEntry = AspPush(engine, element);
                            if (stackEntry == 0)
                                return AspRunResult_OutOfDataMemory;
                            AspUnref(engine, element);

                            break;
                        }

                        case DataType_Range:
                        {
                            int32_t startValue, endValue, stepValue;
                            AspGetRange
                                (engine, index,
                                 &startValue, &endValue, &stepValue);

                            /* Create a new string to receive the sliced
                               characters. */
                            AspDataEntry *result = AspAllocEntry
                                (engine, containerType);

                            /* Perform the slice. */
                            AspSequenceResult (*Navigate)
                                (AspEngine *, AspDataEntry *, AspDataEntry *) =
                                stepValue < 0 ?
                                AspSequencePrevious : AspSequenceNext;
                            int32_t i = stepValue < 0 ? -1 : 0;
                            int32_t increment= stepValue < 0 ? -1 : +1;
                            int32_t select = stepValue < 0 ?
                                startValue + 1 : startValue;
                            for (AspSequenceResult nextResult =
                                 Navigate(engine, container, 0);
                                 nextResult.element != 0 &&
                                 (stepValue < 0 ? i > endValue : i < endValue);
                                 nextResult = Navigate
                                    (engine, container, nextResult.element))
                            {
                                AspDataEntry *fragment = nextResult.value;
                                uint8_t fragmentSize =
                                    AspDataGetStringFragmentSize(fragment);
                                char *fragmentData =
                                    AspDataGetStringFragmentData(fragment);

                                for (uint8_t fragmentIndex = 0;
                                     fragmentIndex < fragmentSize &&
                                     (stepValue < 0 ?
                                      i > endValue : i < endValue);
                                     i += increment, select -= increment,
                                     fragmentIndex++)
                                {
                                    /* Skip if not selected. */
                                    if (select != 0)
                                        continue;

                                    /* Append the character. */
                                    uint8_t charIndex =
                                        stepValue < 0 ?
                                        fragmentSize - fragmentIndex - 1 :
                                        fragmentIndex;
                                    char c = fragmentData[charIndex];
                                    AspRunResult appendResult =
                                        AspStringAppendBuffer
                                        (engine, result, &c, 1);
                                    if (appendResult != AspRunResult_OK)
                                        return appendResult;

                                    /* Prepare to identify the next element. */
                                    select = stepValue;
                                }
                            }

                            /* Push the resulting sequence. */
                            AspDataEntry *stackEntry = AspPush(engine, result);
                            if (stackEntry == 0)
                                return AspRunResult_OutOfDataMemory;

                            break;
                        }
                    }

                    break;
                }

                case DataType_Tuple:
                    if (opCode == OpCode_IDXA)
                        return AspRunResult_UnexpectedType;

                    /* Fall through... */

                case DataType_List:
                {
                    switch (AspDataGetType(index))
                    {
                        default:
                            return AspRunResult_UnexpectedType;

                        case DataType_Integer:
                        {
                            int32_t indexValue = AspDataGetInteger(index);

                            /* Locate the element. */
                            AspSequenceResult indexResult = AspSequenceIndex
                                (engine, container, indexValue);
                            if (indexResult.result != AspRunResult_OK)
                                return indexResult.result;
                            if (indexResult.value == 0)
                                return AspRunResult_ValueOutOfRange;

                            /* Push the value or address as applicable. */
                            AspDataEntry *stackEntry = AspPush
                                (engine,
                                 opCode == OpCode_IDX ?
                                 indexResult.value : indexResult.element);
                            if (stackEntry == 0)
                                return AspRunResult_OutOfDataMemory;

                            break;
                        }

                        case DataType_Range:
                        {
                            int32_t startValue, endValue, stepValue;
                            AspGetRange
                                (engine, index,
                                 &startValue, &endValue, &stepValue);

                            /* Create a new container to receive the sliced
                               elements. */
                            AspDataEntry *result = AspAllocEntry
                                (engine, containerType);

                            /* Perform the slice. */
                            AspSequenceResult (*Navigate)
                                (AspEngine *, AspDataEntry *, AspDataEntry *) =
                                stepValue < 0 ?
                                AspSequencePrevious : AspSequenceNext;
                            int32_t i = stepValue < 0 ? -1 : 0;
                            int32_t increment= stepValue < 0 ? -1 : +1;
                            int32_t select = stepValue < 0 ?
                                startValue + 1 : startValue;
                            for (AspSequenceResult nextResult =
                                 Navigate(engine, container, 0);
                                 nextResult.element != 0 &&
                                 (stepValue < 0 ? i > endValue : i < endValue);
                                 i += increment, select -= increment,
                                 nextResult = Navigate
                                    (engine, container, nextResult.element))
                            {
                                /* Skip if not selected. */
                                if (select != 0)
                                    continue;

                                /* Append the value or address as applicable. */
                                AspDataEntry *value =
                                    opCode == OpCode_IDX ?
                                    nextResult.value : nextResult.element;
                                AspSequenceAppend(engine, result, value);

                                /* Prepare to identify the next element. */
                                select = stepValue;
                            }

                            /* Push the resulting sequence. */
                            AspDataEntry *stackEntry = AspPush(engine, result);
                            if (stackEntry == 0)
                                return AspRunResult_OutOfDataMemory;

                            break;
                        }
                    }

                    break;
                }

                case DataType_Dictionary:
                {
                    /* Locate the entry. */
                    AspTreeResult findResult = AspTreeFind
                        (engine, container, index);
                    if (findResult.result != AspRunResult_OK)
                        return findResult.result;

                    if (opCode == OpCode_IDX)
                    {
                        /* Fail if the key was not found. */
                        if (findResult.node == 0)
                            return AspRunResult_KeyNotFound;

                        /* Push the value onto the stack. */
                        AspDataEntry *stackEntry = AspPush
                            (engine, findResult.value);
                        if (stackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                    }
                    else
                    {
                        /* Create a new entry if the key was not found. */
                        AspDataEntry *node = findResult.node;
                        if (node == 0)
                        {
                            AspTreeResult insertResult = AspTreeInsert
                                (engine, container, index,
                                 engine->noneSingleton);
                            if (insertResult.result != AspRunResult_OK)
                                return insertResult.result;
                            node = insertResult.node;
                        }

                        /* Push the address onto the stack. */
                        AspDataEntry *stackEntry = AspPush(engine, node);
                        if (stackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                    }

                    break;
                }
            }

            AspUnref(engine, index);
            AspUnref(engine, container);

            break;
        }

        case OpCode_MEM4:
        case OpCode_MEMA4:
            operandSize += 2;
        case OpCode_MEM2:
        case OpCode_MEMA2:
            operandSize++;
        case OpCode_MEM1:
        case OpCode_MEMA1:
        {
            operandSize++;

            bool isAddressInstruction =
                opCode == OpCode_MEMA1 ||
                opCode == OpCode_MEMA2 ||
                opCode == OpCode_MEMA4;
            #ifdef ASP_DEBUG
            {
                printf(isAddressInstruction ? "MEMA" : "MEM");
                putchar(' ');
            }
            #endif

            /* Fetch the member variables's symbol from the operand. */
            int32_t variableSymbol;
            AspRunResult operandLoadResult = LoadSignedOperand
                (engine, operandSize, &variableSymbol);
            if (operandLoadResult != AspRunResult_OK)
            {
                #ifdef ASP_DEBUG
                puts("?");
                #endif
                return operandLoadResult;
            }
            #ifdef ASP_DEBUG
            printf("%d\n", variableSymbol);
            #endif

            /* Obtain the module from the stack. */
            AspDataEntry *module = AspTop(engine);
            if (module == 0)
                return AspRunResult_StackUnderflow;
            if (AspDataGetType(module) != DataType_Module)
                return AspRunResult_UnexpectedType;
            AspRef(engine, module);
            AspPop(engine);

            /* Access the module's global namespace. */
            AspDataEntry *moduleNamespace = AspValueEntry
                (engine, AspDataGetModuleNamespaceIndex(module));
            if (AspDataGetType(moduleNamespace) != DataType_Namespace)
                return AspRunResult_UnexpectedType;

            /* Look up the variable in the module's namespace, creating
               it for an address lookup if it doesn't exist. */
            AspTreeResult memberResult =
                isAddressInstruction ?
                AspTreeTryInsertBySymbol
                    (engine, moduleNamespace,
                     variableSymbol, engine->noneSingleton) :
                AspFindSymbol
                    (engine, moduleNamespace, variableSymbol);
            if (memberResult.result != AspRunResult_OK)
                return memberResult.result;
            if (memberResult.value == 0)
                return AspRunResult_NameNotFound;

            /* Push variable's value or address as applicable. */
            AspDataEntry *stackEntry = AspPush
                (engine,
                 isAddressInstruction ?
                 memberResult.node : memberResult.value);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;

            AspUnref(engine, module);

            break;
        }

        case OpCode_ABORT:
            return AspRunResult_Abort;

        case OpCode_END:
        {
            #ifdef ASP_DEBUG
            puts("END");
            #endif

            /* Ensure the stack is empty. */
            engine->state = AspEngineState_Ended;
            return engine->stackTop != 0 ?
                AspRunResult_InvalidEnd :
                AspRunResult_Complete;
        }
    }

    return AspRunResult_OK;
}

static AspRunResult LoadUnsignedOperand
    (AspEngine *engine, unsigned operandSize, uint32_t *operand)
{
    unsigned i;

    *operand = 0;
    for (i = 0; i < operandSize; i++)
    {
        *operand <<= 8;
        *operand |= *engine->pc++;
        if (engine->pc > engine->code + engine->codeEndIndex)
            return AspRunResult_BeyondEndOfCode;
    }
    return AspRunResult_OK;
}

static AspRunResult LoadSignedOperand
    (AspEngine *engine, unsigned operandSize, int32_t *operand)
{
    bool negative = operandSize != 0 && (*engine->pc & 0x80) != 0;
    uint32_t unsignedOperand = 0;
    AspRunResult loadResult = LoadUnsignedOperand
        (engine, operandSize, &unsignedOperand);
    if (loadResult != AspRunResult_OK)
        return loadResult;

    if (negative)
    {
        unsigned i = operandSize;
        for (; i < 4; i++)
            unsignedOperand |= 0xFFU << (i << 3);
    }

    *operand = *(int32_t *)&unsignedOperand;
    return AspRunResult_OK;
}

static AspRunResult LoadFloatOperand
    (AspEngine *engine, double *operand)
{
    static const uint16_t word = 1;
    bool be = *(const char *)&word == 0;

    uint8_t data[8];
    int i;
    for (i = 0; i < 8; i++)
    {
        data[be ? i : 7 - i] = *engine->pc++;
        if (engine->pc > engine->code + engine->codeEndIndex)
            return AspRunResult_BeyondEndOfCode;
    }

    *operand = *(double *)data;
    return AspRunResult_OK;
}
