/*
 * Asp application specification definitions.
 */

#ifndef ASP_ASPEC_H
#define ASP_ASPEC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AppSpecPrefix
{
    AppSpecPrefix_MaxFunctionParameterCount = 0xFA,
    AppSpecPrefix_Function = 0xFB,
    AppSpecPrefix_Import = 0xFC,
    AppSpecPrefix_Module = 0xFD,
    AppSpecPrefix_Symbol = 0xFE,
    #ifdef ASP_FEATURE_CLASS
    AppSpecPrefix_Class = 0xFE,
    #endif
    AppSpecPrefix_Variable = 0xFF,

} AppSpecPrefix;

typedef enum AppSpecParameterType
{
    AppSpecParameterType_Defaulted = 0x1,
    AppSpecParameterType_TupleGroup = 0x2,
    AppSpecParameterType_DictionaryGroup = 0x3,

} AppSpecParameterType;

typedef enum AppSpecValueType
{
    AppSpecValueType_None = 0x00,
    AppSpecValueType_Ellipsis = 0x01,
    AppSpecValueType_Boolean = 0x02,
    AppSpecValueType_Integer = 0x03,
    AppSpecValueType_Float = 0x04,
    AppSpecValueType_String = 0x05,

} AppSpecValueType;

#ifdef ASP_FEATURE_CLASS

typedef enum AppSpecClassEntryType
{
    AppSpecClassEntryType_Start = 0x8,

} AppSpecClassEntryType;

typedef enum AppSpecFunctionEntryFlag
{
    AppSpecFunctionEntryFlag_Qualified = 0x1,

} AppSpecFunctionEntryType;

#endif

#ifdef __cplusplus
}
#endif

#endif
