/*
 * Asp engine member lookup definitions.
 */

#ifndef ASP_MEMBER_H
#define ASP_MEMBER_H

#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *member;
} AspMemberResult;

AspMemberResult AspFindMember
    (AspEngine *, AspDataEntry *container, int32_t symbol, bool address);

#ifdef __cplusplus
}
#endif

#endif
