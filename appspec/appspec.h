/*
 * Asp application specification definitions.
 */

#ifndef ASP_ASPEC_H
#define ASP_ASPEC_H

typedef enum AppSpecPrefix
{
    AppSpecPrefix_Symbol = 0xFE,
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

#endif
