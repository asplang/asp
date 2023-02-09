/*
 * Asp engine data definitions.
 */

#ifndef ASP_DATA_H
#define ASP_DATA_H

#include "asp.h"
#include "bits.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Data types. */
typedef enum
{
    /* Objects. */
    DataType_None = 0x00,
    DataType_Ellipsis = 0x01,
    DataType_Boolean = 0x02,
    DataType_Integer = 0x03,
    DataType_Float = 0x04,
    /* DataType_Complex = 0x05, placeholder; not supported */
    DataType_Range = 0x07,
    DataType_String = 0x08,
    DataType_Tuple = 0x09,
    DataType_List = 0x0A,
    DataType_Set = 0x0B,
    DataType_Dictionary = 0x0D,
    DataType_Iterator = 0x0E,
    DataType_Function = 0x0F,
    DataType_Module = 0x10,
    DataType_Type = 0x1F,
    DataType_ObjectMask = 0x3F,

    /* Support types. */
    DataType_CodeAddress = 0x40,
    DataType_StackEntry = 0x50,
    DataType_Frame = 0x52,
    DataType_Element = 0x62,
    DataType_StringFragment = 0x64,
    DataType_KeyValuePair = 0x66,
    DataType_Namespace = 0x70,
    DataType_SetNode = 0x74,
    DataType_DictionaryNode = 0x78,
    DataType_NamespaceNode = 0x7C,
    DataType_TreeLinksNode = 0x7D,
    DataType_Parameter = 0x80,
    DataType_ParameterList = 0x81,
    DataType_Argument = 0x82,
    DataType_ArgumentList = 0x83,
    DataType_Free = 0xFF,
} DataType;

/* Data entry. */
typedef struct AspDataEntry
{
    union
    {
        bool b;
        struct
        {
            uint8_t c;
            uint8_t s[14];
            uint8_t t;
        };
        struct
        {
            uint32_t u0, u1, u2;
        };
        int32_t i;
        double d;
    };
} AspDataEntry;

/* Low-level field access. */
#define AspWordBitSize 28
#define AspDataSetWord0(eptr, value) \
    (AspBitSetField(&(eptr)->u0, 0, (AspWordBitSize), (value)))
#define AspDataGetWord0(eptr) \
    (AspBitGetField((eptr)->u0, 0, (AspWordBitSize)))
#define AspDataSetSignedWord0(eptr, value) \
    (AspBitSetSignedField(&(eptr)->u0, 0, (AspWordBitSize), (value)))
#define AspDataGetSignedWord0(eptr) \
    (AspBitGetSignedField((eptr)->u0, 0, (AspWordBitSize)))
#define AspDataSetWord1(eptr, value) \
    (AspBitSetField(&(eptr)->u1, 0, (AspWordBitSize), (value)))
#define AspDataGetWord1(eptr) \
    (AspBitGetField((eptr)->u1, 0, (AspWordBitSize)))
#define AspDataSetWord2(eptr, value) \
    (AspBitSetField(&(eptr)->u2, 0, (AspWordBitSize), (value)))
#define AspDataGetWord2(eptr) \
    (AspBitGetField((eptr)->u2, 0, (AspWordBitSize)))
void AspDataSetWord3(AspDataEntry *, uint32_t value);
uint32_t AspDataGetWord3(const AspDataEntry *);
#define AspDataSetBit0(eptr, value) \
    (AspBitSet(&(eptr)->u1, (AspWordBitSize), (value)))
#define AspDataGetBit0(eptr) \
    (AspBitGet((eptr)->u1, (AspWordBitSize)))
#define AspDataSetBit1(eptr, value) \
    (AspBitSet(&(eptr)->u1, (AspWordBitSize) + 1, (value)))
#define AspDataGetBit1(eptr) \
    (AspBitGet((eptr)->u1, (AspWordBitSize) + 1))
#define AspDataSetBit2(eptr, value) \
    (AspBitSet(&(eptr)->u1, (AspWordBitSize) + 2, (value)))
#define AspDataGetBit2(eptr) \
    (AspBitGet((eptr)->u1, (AspWordBitSize) + 2))

/* Common field access. */
#define AspDataSetType(eptr, ty) \
    ((eptr)->t = (uint8_t)(ty))
#define AspDataGetType(eptr) \
    ((eptr)->t)
#define AspDataSetUseCount(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetUseCount(eptr) \
    (AspDataGetWord2((eptr)))

/* Boolean entry field access. */
#define AspDataSetBoolean(eptr, value) \
    ((eptr)->b = (value))
#define AspDataGetBoolean(eptr) \
    ((eptr)->b)

/* Integer entry field access. */
#define AspDataSetInteger(eptr, value) \
    ((eptr)->i = (value))
#define AspDataGetInteger(eptr) \
    ((eptr)->i)

/* Float entry field access. */
#define AspDataSetFloat(eptr, value) \
    ((eptr)->d = (value))
#define AspDataGetFloat(eptr) \
    ((eptr)->d)

/* Range entry field access. */
#define AspDataSetRangeHasStart(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetRangeHasStart(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetRangeStartIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetRangeStartIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetRangeHasEnd(eptr, value) \
    (AspDataSetBit1((eptr), (unsigned)(value)))
#define AspDataGetRangeHasEnd(eptr) \
    ((bool)(AspDataGetBit1((eptr))))
#define AspDataSetRangeEndIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetRangeEndIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetRangeHasStep(eptr, value) \
    (AspDataSetBit2((eptr), (unsigned)(value)))
#define AspDataGetRangeHasStep(eptr) \
    ((bool)(AspDataGetBit2((eptr))))
#define AspDataSetRangeStepIndex(eptr, value) \
    (AspDataSetWord3((eptr), (value)))
#define AspDataGetRangeStepIndex(eptr) \
    (AspDataGetWord3((eptr)))

/* Sequence entry field access for String, Tuple, List, ParameterList, and
   ArgumentList. */
#define AspDataSetSequenceCount(eptr, value) \
    (AspDataSetWord3((eptr), (value)))
#define AspDataGetSequenceCount(eptr) \
    (AspDataGetWord3((eptr)))
#define AspDataSetSequenceHeadIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetSequenceHeadIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetSequenceTailIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetSequenceTailIndex(eptr) \
    (AspDataGetWord1((eptr)))

/* Common tree entry field access for Set, Dictionary, and Namespace. */
#define AspDataSetTreeCount(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetTreeCount(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetTreeRootIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetTreeRootIndex(eptr) \
    (AspDataGetWord1((eptr)))

/* Iterator entry field access. */
#define AspDataSetIteratorIterableIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetIteratorIterableIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetIteratorMemberIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetIteratorMemberIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetIteratorMemberNeedsCleanup(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetIteratorMemberNeedsCleanup(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetIteratorStringIndex(eptr, value) \
    ((eptr)->s[11] = (value))
#define AspDataGetIteratorStringIndex(eptr) \
    ((eptr)->s[11])

/* Function entry field access. */
#define AspDataSetFunctionIsApp(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetFunctionIsApp(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetFunctionSymbol(eptr, value) \
    (AspDataSetSignedWord0((eptr), (value)))
#define AspDataGetFunctionSymbol(eptr) \
    (AspDataGetSignedWord0((eptr)))
#define AspDataSetFunctionCodeAddress(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetFunctionCodeAddress(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetFunctionModuleIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetFunctionModuleIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetFunctionParametersIndex(eptr, value) \
    (AspDataSetWord3((eptr), (value)))
#define AspDataGetFunctionParametersIndex(eptr) \
    (AspDataGetWord3((eptr)))

/* Module entry field access. */
#define AspDataSetModuleCodeAddress(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetModuleCodeAddress(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetModuleNamespaceIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetModuleNamespaceIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetModuleIsLoaded(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetModuleIsLoaded(eptr) \
    ((bool)(AspDataGetBit0((eptr))))

/* Type entry field access. */
#define AspDataSetTypeValue(eptr, value) \
    ((eptr)->c = (value))
#define AspDataGetTypeValue(eptr) \
    ((eptr)->c)

/* CodeAddress entry field access. */
#define AspDataSetCodeAddress(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetCodeAddress(eptr) \
    (AspDataGetWord0((eptr)))

/* StackEntry entry field access. */
#define AspDataSetStackEntryPreviousIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetStackEntryPreviousIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetStackEntryValueIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetStackEntryValueIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetStackEntryHasValue2(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetStackEntryHasValue2(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetStackEntryValue2Index(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetStackEntryValue2Index(eptr) \
    (AspDataGetWord2((eptr)))
#define AspDataSetStackEntryFlag(eptr, value) \
    (AspDataSetBit1((eptr), (unsigned)(value)))
#define AspDataGetStackEntryFlag(eptr) \
    ((bool)(AspDataGetBit1((eptr))))

/* Frame entry field access. */
#define AspDataSetFrameReturnAddress(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetFrameReturnAddress(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetFrameModuleIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetFrameModuleIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetFrameLocalNamespaceIndex(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetFrameLocalNamespaceIndex(eptr) \
    (AspDataGetWord2((eptr)))

/* Common element entry field access. */
#define AspDataSetElementPreviousIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetElementPreviousIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetElementNextIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetElementNextIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetElementValueIndex(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetElementValueIndex(eptr) \
    (AspDataGetWord2((eptr)))

/* StringFragment entry field access. */
#define AspDataGetStringFragmentMaxSize() \
    ((uint8_t)(offsetof(AspDataEntry, t) - offsetof(AspDataEntry, s)))
#define AspDataSetStringFragment(eptr, value, count) \
    ((eptr)->c = (count), memcpy((eptr)->s, (value), (count)))
#define AspDataSetStringFragmentData(eptr, index, value, count) \
    (memcpy((eptr)->s + (index), (value), (count)))
#define AspDataSetStringFragmentSize(eptr, count) \
    ((eptr)->c = (count))
#define AspDataGetStringFragmentSize(eptr) \
    ((eptr)->c)
#define AspDataGetStringFragmentData(eptr) \
    ((char *)(eptr)->s)

/* KeyValuePair entry field access. */
#define AspDataSetKeyValuePairKeyIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetKeyValuePairKeyIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetKeyValuePairValueIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetKeyValuePairValueIndex(eptr) \
    (AspDataGetWord1((eptr)))

/* Common tree node entry field access. */
#define AspDataSetTreeNodeKeyIndex(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetTreeNodeKeyIndex(eptr) \
    (AspDataGetWord0((eptr)))
#define AspDataSetTreeNodeParentIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetTreeNodeParentIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetTreeNodeIsBlack(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetTreeNodeIsBlack(eptr) \
    ((bool)(AspDataGetBit0((eptr))))

/* SetNode entry field access. */
#define AspDataSetSetNodeLeftIndex(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetSetNodeLeftIndex(eptr) \
    (AspDataGetWord2((eptr)))
#define AspDataSetSetNodeRightIndex(eptr, value) \
    (AspDataSetWord3((eptr), (value)))
#define AspDataGetSetNodeRightIndex(eptr) \
    (AspDataGetWord3((eptr)))

/* DictionaryNode and NamespaceNode entry field access. */
#define AspDataSetTreeNodeLinksIndex(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetTreeNodeLinksIndex(eptr) \
    (AspDataGetWord2((eptr)))
#define AspDataSetTreeNodeValueIndex(eptr, value) \
    (AspDataSetWord3((eptr), (value)))
#define AspDataGetTreeNodeValueIndex(eptr) \
    (AspDataGetWord3((eptr)))

/* NamespaceNode entry field access. */
#define AspDataSetNamespaceNodeSymbol(eptr, value) \
    (AspDataSetSignedWord0((eptr), (value)))
#define AspDataGetNamespaceNodeSymbol(eptr) \
    (AspDataGetSignedWord0((eptr)))
#define AspDataSetNamespaceNodeIsGlobal(eptr, value) \
    (AspDataSetBit1((eptr), (unsigned)(value)))
#define AspDataGetNamespaceNodeIsGlobal(eptr) \
    ((bool)(AspDataGetBit1((eptr))))
#define AspDataSetNamespaceNodeIsLocal(eptr, value) \
    (AspDataSetBit2((eptr), (unsigned)(value)))
#define AspDataGetNamespaceNodeIsLocal(eptr) \
    ((bool)(AspDataGetBit2((eptr))))

/* TreeLinksNode entry field access. */
#define AspDataSetTreeLinksNodeLeftIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetTreeLinksNodeLeftIndex(eptr) \
    (AspDataGetWord1((eptr)))
#define AspDataSetTreeLinksNodeRightIndex(eptr, value) \
    (AspDataSetWord2((eptr), (value)))
#define AspDataGetTreeLinksNodeRightIndex(eptr) \
    (AspDataGetWord2((eptr)))

/* Parameter entry field access. */
#define AspDataSetParameterSymbol(eptr, value) \
    (AspDataSetSignedWord0((eptr), (value)))
#define AspDataGetParameterSymbol(eptr) \
    (AspDataGetSignedWord0((eptr)))
#define AspDataSetParameterHasDefault(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetParameterHasDefault(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetParameterIsGroup(eptr, value) \
    (AspDataSetBit1((eptr), (unsigned)(value)))
#define AspDataGetParameterIsGroup(eptr) \
    ((bool)(AspDataGetBit1((eptr))))
#define AspDataSetParameterDefaultIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetParameterDefaultIndex(eptr) \
    (AspDataGetWord1((eptr)))

/* Argument entry field access. */
#define AspDataSetArgumentSymbol(eptr, value) \
    (AspDataSetSignedWord0((eptr), (value)))
#define AspDataGetArgumentSymbol(eptr) \
    (AspDataGetSignedWord0((eptr)))
#define AspDataSetArgumentHasName(eptr, value) \
    (AspDataSetBit0((eptr), (unsigned)(value)))
#define AspDataGetArgumentHasName(eptr) \
    ((bool)(AspDataGetBit0((eptr))))
#define AspDataSetArgumentIsGroup(eptr, value) \
    (AspDataSetBit1((eptr), (unsigned)(value)))
#define AspDataGetArgumentIsGroup(eptr) \
    ((bool)(AspDataGetBit1((eptr))))
#define AspDataSetArgumentValueIndex(eptr, value) \
    (AspDataSetWord1((eptr), (value)))
#define AspDataGetArgumentValueIndex(eptr) \
    (AspDataGetWord1((eptr)))

/* Free entry field access. */
#define AspDataSetFreeNext(eptr, value) \
    (AspDataSetWord0((eptr), (value)))
#define AspDataGetFreeNext(eptr) \
    (AspDataGetWord0((eptr)))

/* Functions. */
void AspClearData(AspEngine *);
uint32_t AspAlloc(AspEngine *);
bool AspFree(AspEngine *, uint32_t index);
bool AspIsObject(const AspDataEntry *);
AspRunResult AspCheckIsImmutableObject
    (AspEngine *, const AspDataEntry *, bool *isImmutable);
AspDataEntry *AspAllocEntry(AspEngine *, DataType);
AspDataEntry *AspEntry(AspEngine *, uint32_t index);
AspDataEntry *AspValueEntry(AspEngine *, uint32_t index);
uint32_t AspIndex(const AspEngine *, const AspDataEntry *);

#ifdef __cplusplus
}
#endif

#endif
