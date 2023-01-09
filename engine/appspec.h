/*
 * Asp application specification definitions.
 */

#ifndef ASP_ASPEC_H
#define ASP_ASPEC_H

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
