/*
 * Version header generation main.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Syntax: %s major minor patch tweak\n", argv[0]);
        return 1;
    }

    /* Compute the 32-bit version from the four components. */
    unsigned long version = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        unsigned argi = 1 + i;

        char *p;
        unsigned long v = strtoul(argv[argi], &p, 0);
        if (*p != '\0')
        {
            fprintf(stderr,
                "Error: Version component %u, '%s', invalid\n",
                i, argv[argi]);
            return 2;
        }
        if (v > 255)
        {
            fprintf(stderr,
                "Error: Version component %u exceeds 255\n", i);
            return 2;
        }

        version <<= 8;
        version |= v;
    }

    /* Write the header content. */
    printf
        ("/* Asp engine version. */\n"
         "#ifndef ASP_VERSION\n"
         "#define ASP_VERSION 0x%8.8lX\n"
         "#endif\n",
         version);

    return 0;
}
