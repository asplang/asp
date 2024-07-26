/*
 * Standalone Asp application script context.
 */

#ifndef ASPS_CONTEXT_H
#define ASPS_CONTEXT_H

#include <ctime>

typedef struct
{
    bool sleeping;
    clock_t expiry;
} StandaloneAspContext;

#endif
