/*
 * Asp engine op code definitions.
 */

#ifndef ASP_OPCODE_H
#define ASP_OPCODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OpCode
{
    /* Generic stack operations. */
    OpCode_PUSHN = 0x00, /* None */
    OpCode_PUSHE = 0x01, /* ... */
    OpCode_PUSHF = 0x02, /* False */
    OpCode_PUSHT = 0x03, /* True */
    OpCode_PUSHI0 = 0x04, /* int 0 */
    OpCode_PUSHI1 = 0x05, /* 1-byte integer */
    OpCode_PUSHI2 = 0x06, /* 2-byte integer */
    OpCode_PUSHI4 = 0x07, /* 4-byte integer */
    OpCode_PUSHD = 0x08, /* double-precision floating-point */
    /* Opcode_PUSHCX = 0x09, double-precision complex, placeholder */
    OpCode_PUSHY1 = 0x0D, /* 1-byte variable symbol */
    OpCode_PUSHY2 = 0x0E, /* 2-byte variable symbol */
    OpCode_PUSHY4 = 0x0F, /* 4-byte variable symbol */
    OpCode_PUSHS0 = 0x10, /* empty string */
    OpCode_PUSHS1 = 0x11, /* 1-byte length string */
    OpCode_PUSHS2 = 0x12, /* 2-byte length string */
    OpCode_PUSHS4 = 0x13, /* 4-byte length string */
    OpCode_PUSHTU = 0x14, /* empty tuple */
    OpCode_PUSHLI = 0x15, /* empty list */
    OpCode_PUSHSE = 0x16, /* empty set */
    OpCode_PUSHDI = 0x17, /* empty dictionary */
    OpCode_PUSHAL = 0x18, /* argument list */
    OpCode_PUSHPL = 0x19, /* parameter list */
    OpCode_PUSHCA = 0x1C, /* 4-byte code address */
    OpCode_PUSHM1 = 0x1D, /* 1-byte module symbol */
    OpCode_PUSHM2 = 0x1E, /* 2-byte module symbol */
    OpCode_PUSHM4 = 0x1F, /* 4-byte module symbol */
    OpCode_POP = 0x20, /* pop single entry */
    OpCode_POP1 = 0x21, /* pop N entries with 1-byte count */

    /* Unary operations. */
    OpCode_LNOT = 0x40, /* logical not */
    OpCode_POS = 0x48, /* positive value */
    OpCode_NEG = 0x49, /* negate */
    OpCode_NOT = 0x4F, /* bitwise not */

    /* Binary logical and arithmetic operations. */
    OpCode_OR = 0x53, /* bitwise or */
    OpCode_XOR = 0x54, /* bitwise exclusive or */
    OpCode_AND = 0x55, /* bitwise and */
    OpCode_LSH = 0x56, /* left shift */
    OpCode_RSH = 0x57, /* right shift */
    OpCode_ADD = 0x58, /* add */
    OpCode_SUB = 0x59, /* subtract */
    OpCode_MUL = 0x5A, /* multiply */
    OpCode_DIV = 0x5B, /* divide */
    OpCode_FDIV = 0x5C, /* floor divide */
    OpCode_MOD = 0x5D, /* modulo */
    OpCode_POW = 0x5E, /* power */

    /* Binary comparison operations. */
    OpCode_NE = 0x60, /* != */
    OpCode_EQ = 0x61, /* == */
    OpCode_LT = 0x62, /* < */
    OpCode_LE = 0x63, /* <= */
    OpCode_GT = 0x64, /* > */
    OpCode_GE = 0x65, /* >= */
    OpCode_NIN = 0x66, /* not in */
    OpCode_IN = 0x67, /* in */
    OpCode_NIS = 0x68, /* is not */
    OpCode_IS = 0x69, /* is */
    OpCode_ORDER = 0x6C, /* object order */

    /* Load operations. */
    OpCode_LD1 = 0x81, /* load variable's value with 1-byte symbol */
    OpCode_LD2 = 0x82, /* load variable's value with 2-byte symbol */
    OpCode_LD4 = 0x83, /* load variable's value with 4-byte symbol */
    OpCode_LDA1 = 0x85, /* load variable's address with 1-byte symbol */
    OpCode_LDA2 = 0x86, /* load variable's address with 2-byte symbol */
    OpCode_LDA4 = 0x87, /* load variable's address with 4-byte symbol */

    /* Assignment and deletion operations. */
    OpCode_SET = 0x88, /* assign variable (no pop) */
    OpCode_SETP = 0x89, /* assign variable with pop */
    OpCode_ERASE = 0x8C, /* delete element or slice */
    OpCode_DEL1 = 0x8D, /* delete variable with 1-byte symbol */
    OpCode_DEL2 = 0x8E, /* delete variable with 2-byte symbol */
    OpCode_DEL4 = 0x8F, /* delete variable with 4-byte symbol */

    /* Global override operations. */
    OpCode_GLOB1 = 0x91, /* 1-byte symbol global override */
    OpCode_GLOB2 = 0x92, /* 2-byte symbol global override */
    OpCode_GLOB4 = 0x93, /* 4-byte symbol global override */
    OpCode_LOC1 = 0x95, /* cancel 1-byte symbol global override */
    OpCode_LOC2 = 0x96, /* cancel 2-byte symbol global override */
    OpCode_LOC4 = 0x97, /* cancel 4-byte symbol global override */

    /* Iterator operations. */
    OpCode_SITER = 0xA0, /* start iterator */
    OpCode_TITER = 0xA1, /* test iterator */
    OpCode_NITER = 0xA2, /* advance iterator to next */
    OpCode_DITER = 0xA3, /* dereference iterator */

    /* Jump operations. */
    OpCode_NOOP = 0xB0, /* (never jump) */
    OpCode_JMPF = 0xB1, /* jump false */
    OpCode_JMPT = 0xB2, /* jump true */
    OpCode_JMP = 0xB3, /* unconditional jump */
    OpCode_LOR = 0xB4, /* short-cut logical or */
    OpCode_LAND = 0xB5, /* short-cut logical and */

    /* Function call/return operations. */
    OpCode_CALL = 0xB6, /* call function */
    OpCode_RET = 0xB7, /* return from function */

    /* Module operations. */
    OpCode_ADDMOD1 = 0xB9, /* add module with 1-byte symbol to engine */
    OpCode_ADDMOD2 = 0xBA, /* add module with 2-byte symbol to engine */
    OpCode_ADDMOD4 = 0xBB, /* add module with 4-byte symbol to engine */
    OpCode_XMOD = 0xBC, /* exit module */
    OpCode_LDMOD1 = 0xBD, /* load/call module with 1-byte symbol */
    OpCode_LDMOD2 = 0xBE, /* load/call module with 2-byte symbol */
    OpCode_LDMOD4 = 0xBF, /* load/call module with 4-byte symbol */

    /* Function argument operations. */
    OpCode_MKARG = 0xC0, /* make positional arg */
    OpCode_MKNARG1 = 0xC1, /* make named argument with 1-byte symbol */
    OpCode_MKNARG2 = 0xC2, /* make named argument with 2-byte symbol */
    OpCode_MKNARG4 = 0xC3, /* make named argument with 4-byte symbol */
    OpCode_MKIGARG = 0xC4, /* make iteratable group argument */
    OpCode_MKDGARG = 0xC5, /* make dictionary group argument */

    /* Function parameter operations. */
    OpCode_MKPAR1 = 0xD1, /* make parameter with 1-byte symbol */
    OpCode_MKPAR2 = 0xD2, /* make parameter with 2-byte symbol */
    OpCode_MKPAR4 = 0xD3, /* make parameter with 4-byte symbol */
    OpCode_MKDPAR1 = 0xD5, /* make parameter with default with 1-byte symbol */
    OpCode_MKDPAR2 = 0xD6, /* make parameter with default with 2-byte symbol */
    OpCode_MKDPAR4 = 0xD7, /* make parameter with default with 4-byte symbol */
    OpCode_MKTGPAR1 = 0xD9, /* make tuple group parameter with 1-byte symbol */
    OpCode_MKTGPAR2 = 0xDA, /* make tuple group parameter with 2-byte symbol */
    OpCode_MKTGPAR4 = 0xDB, /* make tuple group parameter with 4-byte symbol */
    OpCode_MKDGPAR1 = 0xDD, /* make dict group parameter with 1-byte symbol */
    OpCode_MKDGPAR2 = 0xDE, /* make dict group parameter with 1-byte symbol */
    OpCode_MKDGPAR4 = 0xDF, /* make dict group parameter with 1-byte symbol */

    /* Function definition operations. */
    OpCode_MKFUN = 0xE0, /* make function */

    /* Container entry operations. */
    OpCode_MKKVP = 0xE2, /* make key/value pair entry */

    /* Range operations. */
    OpCode_MKR0 = 0xE4, /* make range .. */
    OpCode_MKRS = 0xE5, /* make range start.. */
    OpCode_MKRE = 0xE6, /* make range ..end */
    OpCode_MKRSE = 0xE7, /* make range start..end */
    OpCode_MKRT = 0xE8, /* make range ..:step */
    OpCode_MKRST = 0xE9, /* make range start..:step */
    OpCode_MKRET = 0xEA, /* make range ..end:step */
    OpCode_MKR = 0xEB, /* make range s..end:step */

    /* Insert operations. */
    OpCode_INS = 0xEC, /* insert (list, set, dictionary) */
    OpCode_INSP = 0xED, /* insert with pop */
    OpCode_BLD = 0xEE, /* build (tuple, list, set, dictionary, args, params) */

    /* Indexing operations. */
    OpCode_IDX = 0xF0, /* index (or range) value */
    OpCode_IDXA = 0xF1, /* index (or range) address */

    /* Member look-up operations. */
    OpCode_MEM1 = 0xF4, /* 1-byte member lookup value by symbol */
    OpCode_MEM2 = 0xF5, /* 2-byte member lookup value by symbol */
    OpCode_MEM4 = 0xF6, /* 4-byte member lookup value by symbol */
    OpCode_MEMA1 = 0xF8, /* 1-byte member lookup address by symbol */
    OpCode_MEMA2 = 0xF9, /* 2-byte member lookup address by symbol */
    OpCode_MEMA4 = 0xFA, /* 4-byte member lookup address by symbol */

    /* End operations. */
    OpCode_ABORT = 0xFE, /* abnormal exit (assertion failure) */
    OpCode_END = 0xFF, /* normal exit, stack must be empty */

} OpCode;

#ifdef __cplusplus
}
#endif

#endif
