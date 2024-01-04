/*
 * API macro definitions.
 *
 * Copyright (c) 2024 Canadensys Aerospace Corporation.
 * See LICENSE.txt at https://bitbucket.org/asplang/asp for details.
 */

#ifndef ASP_API
    #if USING_SHARED_LIBS
        #if defined _WIN32 && !defined __CYGWIN__
            #ifdef ASP_EXPORT_API
                #define ASP_API __declspec(dllexport)
            #else
                #define ASP_API __declspec(dllimport)
            #endif
        #else
            #ifdef ASP_EXPORT_API
                #define ASP_API __attribute__((visibility("default")))
            #else
                #define ASP_API
            #endif
        #endif
    #else
        #define ASP_API
    #endif
#endif

#ifndef ASP_LIB_API
    #if USING_SHARED_LIBS
        #if defined _WIN32 && !defined __CYGWIN__
            #ifdef ASP_EXPORT_LIB_API
                #define ASP_LIB_API __declspec(dllexport)
            #else
                #define ASP_LIB_API __declspec(dllimport)
            #endif
        #else
            #ifdef ASP_EXPORT_LIB_API
                #define ASP_LIB_API __attribute__((visibility("default")))
            #else
                #define ASP_LIB_API
            #endif
        #endif
    #else
        #define ASP_LIB_API
    #endif
#endif
