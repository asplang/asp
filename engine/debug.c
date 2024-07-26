/*
 * Asp engine debug implementation.
 */

#include "debug.h"
#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

static void DumpData(const AspEngine *, FILE *);
static void DumpDataEntry(uint32_t index, const AspDataEntry *, FILE *);

void AspTraceFile(AspEngine *engine, FILE *fp)
{
    engine->traceFile = fp;
}

void AspDump(const AspEngine *engine, FILE *fp)
{
    DumpData(engine, fp);
    fputs("---\n", fp);

    fprintf
        (fp, "Program counter: 0x%07X\n",
         (uint32_t)AspProgramCounter(engine));

    fprintf
        (fp, "Free count: %d; next free entry: 0x%07X\n",
         (uint32_t)engine->freeCount, (uint32_t)engine->freeListIndex);
    fprintf
        (fp, "Free count low water mark: %d\n",
         (uint32_t)engine->lowFreeCount);

    fprintf(fp, "Stack: ");
    if (engine->stackTop == 0)
        fputs("empty", fp);
    else
    {
        fprintf(fp, "top=0x%07X", AspIndex(engine, engine->stackTop));
        fprintf(fp, ", count=%d", engine->stackCount);
    }
    fputc('\n', fp);

    fprintf
        (fp, "Modules: 0x%07X\n",
         AspIndex(engine, engine->modules));
    fprintf
        (fp, "Current module: 0x%07X\n",
         AspIndex(engine, engine->module));
    fprintf
        (fp, "System namespace: 0x%07X\n",
         AspIndex(engine, engine->systemNamespace));
    fprintf
        (fp, "Current global namespace: 0x%07X\n",
         AspIndex(engine, engine->globalNamespace));
    fprintf
        (fp, "Current local namespace: 0x%07X\n",
         AspIndex(engine, engine->localNamespace));
}

static void DumpData(const AspEngine *engine, FILE *fp)
{
    AspDataEntry *data = engine->data;
    bool inFree = false;
    unsigned freeRangeStart = 0;
    for (unsigned i = 0; i < engine->dataEndIndex; i++)
    {
        uint8_t t = AspDataGetType(data + i);
        if (inFree)
        {
            if (t != DataType_Free)
            {
                if (freeRangeStart != i - 1)
                    fprintf(fp, "0x%07X...", freeRangeStart);
                fprintf(fp, "0x%07X: t=0x%02X(free)\n",
                    i - 1, DataType_Free);
                inFree = false;
                DumpDataEntry(i, data + i, fp);
            }
        }
        else
        {
            if (t == DataType_Free)
            {
                freeRangeStart = i;
                inFree = true;
            }
            else
                DumpDataEntry(i, data + i, fp);
        }
    }
    if (inFree)
    {
        if (freeRangeStart != engine->dataEndIndex - 1)
            fprintf(fp, "0x%07X...", freeRangeStart);
        fprintf(fp, "0x%07X: t=0x%02X(free)\n",
            (uint32_t)engine->dataEndIndex - 1, DataType_Free);
    }
}

typedef struct
{
    DataType type;
    const char *name;
} TypeName;
static TypeName gTypeNames[] =
{
    /* Objects. */
    {DataType_None, "None"},
    {DataType_Ellipsis, "..."},
    {DataType_Boolean, "bool"},
    {DataType_Integer, "int"},
    {DataType_Float, "float"},
    /* {DataType_Complex, "cmplx"}, */
    {DataType_Symbol, "sym"},
    {DataType_Range, "range"},
    {DataType_String, "str"},
    {DataType_Tuple, "tuple"},
    {DataType_List, "list"},
    {DataType_Set, "set"},
    {DataType_Dictionary, "dict"},
    {DataType_Function, "func"},
    {DataType_Module, "mod"},
    {DataType_ReverseIterator, "iter-rev"},
    {DataType_ForwardIterator, "iter"},
    {DataType_AppIntegerObject, "app-int"},
    {DataType_AppPointerObject, "app-ptr"},
    {DataType_Type, "type"},

    /* Support types. */
    {DataType_CodeAddress, "caddr"},
    {DataType_StackEntry, "stkent"},
    {DataType_Frame, "frame"},
    {DataType_AppFrame, "appframe"},
    {DataType_Element, "elem"},
    {DataType_StringFragment, "strfrag"},
    {DataType_KeyValuePair, "kvp"},
    {DataType_Namespace, "ns"},
    {DataType_SetNode, "snode"},
    {DataType_DictionaryNode, "dnode"},
    {DataType_NamespaceNode, "nsnode"},
    {DataType_TreeLinksNode, "lrnode"},
    {DataType_Parameter, "parm"},
    {DataType_ParameterList, "parms"},
    {DataType_Argument, "arg"},
    {DataType_ArgumentList, "args"},
    {DataType_AppIntegerObjectInfo, "app-ii"},
    {DataType_AppPointerObjectInfo, "app-pi"},
    {DataType_Free, "free"},
};

static int CompareTypeNames(const void *key, const void *element)
{
    const TypeName *typeNameKey = key;
    const TypeName *typeNameElement = element;
    return
        (int)typeNameKey->type == (int)typeNameElement->type ? 0 :
        (int)typeNameKey->type < (int)typeNameElement->type ? -1 : 1;
}

static void DumpDataEntry(uint32_t index, const AspDataEntry *entry, FILE *fp)
{
    uint8_t t = AspDataGetType(entry);
    fprintf(fp, "0x%07X: t=0x%02X", index, t);
    TypeName keyTypeName = {t, ""};
    TypeName *nameEntry = (TypeName *)bsearch
        (&keyTypeName, gTypeNames,
         sizeof gTypeNames / sizeof *gTypeNames, sizeof *gTypeNames,
         CompareTypeNames);
    if (nameEntry != 0)
        fprintf(fp, "(%s)", nameEntry->name);
    if (AspIsObject(entry))
        fprintf(fp, " use=%d", AspDataGetUseCount(entry));
    switch (t)
    {
        case DataType_Boolean:
            fprintf(fp, " value=%s",
                AspDataGetBoolean(entry) ? "True" : "False");
            break;

        case DataType_Integer:
            fprintf(fp, " value=%d", AspDataGetInteger(entry));
            break;

        case DataType_Float:
            fprintf(fp, " value=%g", AspDataGetFloat(entry));
            break;

        case DataType_Symbol:
            fprintf(fp, " value=%d", AspDataGetSymbol(entry));
            break;

        case DataType_Range:
            fprintf(fp, " start=0x%07X end=0x%07X step=0x%07X",
                AspDataGetRangeStartIndex(entry),
                AspDataGetRangeEndIndex(entry),
                AspDataGetRangeStepIndex(entry));
            break;

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
        case DataType_ParameterList:
        case DataType_ArgumentList:
            fprintf(fp, " count=%u head=0x%07X tail=0x%07X",
                AspDataGetSequenceCount(entry),
                AspDataGetSequenceHeadIndex(entry),
                AspDataGetSequenceTailIndex(entry));
            break;

        case DataType_Set:
        case DataType_Dictionary:
        case DataType_Namespace:
            fprintf(fp, " count=%u root=0x%07X",
                AspDataGetTreeCount(entry),
                AspDataGetTreeRootIndex(entry));
            break;

        case DataType_ForwardIterator:
        case DataType_ReverseIterator:
            fprintf(fp, " coll=0x%07X",
                AspDataGetIteratorIterableIndex(entry));
            fprintf(fp, " mem=0x%07X si=%d",
                AspDataGetIteratorMemberIndex(entry),
                AspDataGetIteratorStringIndex(entry));
            if (AspDataGetIteratorMemberNeedsCleanup(entry))
                fputs(" nc", fp);
            break;

        case DataType_Function:
            if (AspDataGetFunctionIsApp(entry))
                fprintf(fp, " s=%d", AspDataGetFunctionSymbol(entry));
            else
                fprintf(fp, " code=0x%07X",
                    AspDataGetFunctionCodeAddress(entry));
            fprintf(fp, " mod=0x%07X params=0x%07X",
                AspDataGetFunctionModuleIndex(entry),
                AspDataGetFunctionParametersIndex(entry));
            break;

        case DataType_Module:
            fprintf(fp, " code=0x%07X ns=0x%07X ld=%d",
                AspDataGetModuleCodeAddress(entry),
                AspDataGetModuleNamespaceIndex(entry),
                AspDataGetModuleIsLoaded(entry));
            break;

        case DataType_AppIntegerObject:
        case DataType_AppPointerObject:
            #ifdef ASP_WIDE_PTR
            fprintf(fp, " info=0x%07X",
                AspDataGetAppObjectInfoIndex(entry));
            #else
            fprintf(fp, " type=%d",
                AspDataGetAppObjectType(entry));
            if (t == DataType_AppIntegerObject)
                fprintf(fp, " val=%d",
                    AspDataGetAppIntegerObjectValue(entry));
            else
                fprintf(fp, " ptr=%d",
                    AspDataGetAppPointerObjectValue(entry));
            #endif
            if (t == DataType_AppIntegerObject)
            {
                union
                {
                    void (*fp)(AspEngine *, int16_t, int32_t);
                    void *vp;
                } u = {.fp = AspDataGetAppIntegerObjectDestructor(entry)};
                fprintf(fp, " dtor=%p", u.vp);
            }
            else
            {
                union
                {
                    void (*fp)(AspEngine *, int16_t, void *); void *vp;
                } u = {.fp = AspDataGetAppPointerObjectDestructor(entry)};
                fprintf(fp, " dtor=%p", u.vp);
            }
            break;

        case DataType_Type:
            fprintf(fp, " type=0x%02X",
                AspDataGetTypeValue(entry));
            break;

        case DataType_CodeAddress:
            fprintf(fp, " addr=0x%07X",
                AspDataGetCodeAddress(entry));
            break;

        case DataType_StackEntry:
            fprintf(fp, " prev=0x%07X val=0x%07X",
                AspDataGetStackEntryPreviousIndex(entry),
                AspDataGetStackEntryValueIndex(entry));
            if (AspDataGetStackEntryHasValue2(entry))
                fprintf(fp, " val2=0x%07X",
                    AspDataGetStackEntryValue2Index(entry));
            if (AspDataGetStackEntryFlag(entry))
                fputs(" fl", fp);
            break;

        case DataType_Frame:
            fprintf(fp, " ra=0x%07X mod=0x%07X locns=0x%07X",
                AspDataGetFrameReturnAddress(entry),
                AspDataGetFrameModuleIndex(entry),
                AspDataGetFrameLocalNamespaceIndex(entry));
            break;

        case DataType_AppFrame:
            fprintf(fp, " func=0x%07X locns=0x%07X",
                AspDataGetAppFrameFunctionIndex(entry),
                AspDataGetAppFrameLocalNamespaceIndex(entry));
            if (AspDataGetAppFrameReturnValueDefined(entry))
                fprintf(fp, " rv=0x%07X",
                    AspDataGetAppFrameReturnValueIndex(entry));
            break;

        case DataType_Element:
            fprintf(fp, " prev=0x%07X next=0x%07X val=0x%07X",
                AspDataGetElementPreviousIndex(entry),
                AspDataGetElementNextIndex(entry),
                AspDataGetElementValueIndex(entry));
            break;

        case DataType_StringFragment:
        {
            uint8_t count = AspDataGetStringFragmentSize(entry);
            const char *s = AspDataGetStringFragmentData(entry);
            fprintf(fp, " c=%d s='", count);
            for (unsigned i = 0; i < count; i++)
            {
                char c = s[i];
                uint8_t uc = *(uint8_t *)&c;
                if (c == '\'')
                    fputc('\\', fp);
                if (isprint(c))
                    fputc(c, fp);
                else
                    fprintf(fp, "\\x%02x", uc);
            }
            fputc('\'', fp);
            break;
        }

        case DataType_KeyValuePair:
            fprintf(fp, " key=0x%07X val=0x%07X",
                AspDataGetKeyValuePairKeyIndex(entry),
                AspDataGetKeyValuePairValueIndex(entry));
            break;

        case DataType_SetNode:
            fprintf(fp, " key=0x%07X p=0x%07X l=0x%07X r=0x%07X clr=%c",
                AspDataGetTreeNodeKeyIndex(entry),
                AspDataGetTreeNodeParentIndex(entry),
                AspDataGetSetNodeLeftIndex(entry),
                AspDataGetSetNodeRightIndex(entry),
                AspDataGetTreeNodeIsBlack(entry) ? 'B' : 'R');
            break;

        case DataType_DictionaryNode:
            fprintf(fp, " key=0x%07X p=0x%07X lr=0x%07X val=0x%07X clr=%c",
                AspDataGetTreeNodeKeyIndex(entry),
                AspDataGetTreeNodeParentIndex(entry),
                AspDataGetTreeNodeLinksIndex(entry),
                AspDataGetTreeNodeValueIndex(entry),
                AspDataGetTreeNodeIsBlack(entry) ? 'B' : 'R');
            break;

        case DataType_NamespaceNode:
            fprintf(fp,
                " sym=%d p=0x%07X lr=0x%07X val=0x%07X clr=%c gl=%d loc=%d",
                AspDataGetNamespaceNodeSymbol(entry),
                AspDataGetTreeNodeParentIndex(entry),
                AspDataGetTreeNodeLinksIndex(entry),
                AspDataGetTreeNodeValueIndex(entry),
                AspDataGetTreeNodeIsBlack(entry) ? 'B' : 'R',
                AspDataGetNamespaceNodeIsGlobal(entry),
                !AspDataGetNamespaceNodeIsNotLocal(entry));
            break;

        case DataType_TreeLinksNode:
            fprintf(fp, " l=0x%07X r=0x%07X",
                AspDataGetTreeLinksNodeLeftIndex(entry),
                AspDataGetTreeLinksNodeRightIndex(entry));
            break;

        case DataType_Parameter:
            fprintf(fp, " s=%u", AspDataGetParameterSymbol(entry));
            if (AspDataGetParameterHasDefault(entry))
                fprintf(fp, " dflt=0x%07X",
                    AspDataGetParameterDefaultIndex(entry));
            else if (AspDataGetParameterIsTupleGroup(entry))
                fputs(" tgrp", fp);
            else if (AspDataGetParameterIsDictionaryGroup(entry))
                fputs(" dgrp", fp);
            break;

        case DataType_Argument:
            fprintf(fp, " val=0x%07X", AspDataGetArgumentValueIndex(entry));
            if (AspDataGetArgumentHasName(entry))
                fprintf(fp, " sym=%d", AspDataGetArgumentSymbol(entry));
            else if (AspDataGetArgumentIsIterableGroup(entry))
                fputs(" igrp", fp);
            else if (AspDataGetArgumentIsDictionaryGroup(entry))
                fputs(" dgrp", fp);
            else
                fputs(" pos", fp);
            break;

        case DataType_AppIntegerObjectInfo:
            fprintf(fp, " type=%d val=%d",
                AspDataGetAppObjectType(entry),
                AspDataGetAppIntegerObjectValue(entry));
            break;

        case DataType_AppPointerObjectInfo:
            fprintf(fp, " type=%d ptr=%p",
                AspDataGetAppObjectType(entry),
                AspDataGetAppPointerObjectValue(entry));
            break;

        case DataType_Free:
            fprintf(fp, " next=0x%07X", AspDataGetFreeNext(entry));
            break;
    }
    fputc('\n', fp);
}
