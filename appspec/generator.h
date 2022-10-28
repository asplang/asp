//
// Asp application specification generator definitions.
//

#ifndef GENERATOR_H
#define GENERATOR_H

#include "lexer.h"

#ifdef __cplusplus
#include "statement.hpp"
#include <iostream>
#include <set>
#include <vector>
#include <string>
class SymbolTable;
#endif

#ifdef __cplusplus
#define DECLARE_TYPE(T) class T;
#define DECLARE_METHOD(name, ReturnType, ...) \
    friend ReturnType name(Generator *, __VA_ARGS__); \
    ReturnType name(__VA_ARGS__);
#else
#define DECLARE_TYPE(T) typedef struct T T;
#define DECLARE_METHOD(name, ReturnType, ...) \
    ReturnType name(Generator *, __VA_ARGS__);
#define ACTION(name, ...) (name(generator, __VA_ARGS__))
#define FREE_NT(name) (FreeNonTerminal(generator, name))
#define FREE_TOKEN(name) (FreeToken(generator, name))
#endif

DECLARE_TYPE(SourceElement)
DECLARE_TYPE(NonTerminal)
DECLARE_TYPE(Statement)
DECLARE_TYPE(ParameterList)
DECLARE_TYPE(Parameter)

#ifndef __cplusplus
DECLARE_TYPE(Generator);
#else
extern "C" {

class Generator
{
    public:

        // Constructor, destructor.
        Generator
            (std::ostream &errorStream, SymbolTable &,
             const std::string &baseFileName);
        ~Generator();

        // Generator methods.
        unsigned ErrorCount() const;

        // Output methods.
        void WriteCompilerSpec(std::ostream &);
        void WriteApplicationHeader(std::ostream &);
        void WriteApplicationCode(std::ostream &);

#endif

    /* Statements. */
    DECLARE_METHOD
        (IncludeHeader, NonTerminal *, Token *);
    DECLARE_METHOD
        (MakeFunction, NonTerminal *, Token *, ParameterList *, Token *);

    /* Parameters. */
    DECLARE_METHOD
        (MakeEmptyParameterList, ParameterList *, int);
    DECLARE_METHOD
        (AddParameterToList, ParameterList *,
         ParameterList *, Parameter *);
    DECLARE_METHOD
        (MakeParameter, Parameter *, Token *);
    DECLARE_METHOD
        (MakeGroupParameter, Parameter *, Token *);

    /* Miscellaneous methods. */
    DECLARE_METHOD(FreeNonTerminal, void, NonTerminal *);
    DECLARE_METHOD(FreeToken, void, Token *);
    DECLARE_METHOD(ReportError, void, const char *);

#ifdef __cplusplus

    protected:

        // Copy prevention.
        Generator(const Generator &) = delete;
        Generator &operator =(const Generator &) = delete;

        void ReportError(const std::string &);
        std::uint32_t CheckValue();
        void ComputeCheckValue();

    private:

        // Error reporting data.
        std::ostream &errorStream;
        unsigned errorCount = 0;

        // Code generation data.
        SourceLocation currentSourceLocation;
        SymbolTable &symbolTable;
        std::string baseFileName;

        // Spec data.
        std::set<FunctionDefinition> functionDefinitions;
        bool checkValueComputed = false;
        std::uint32_t checkValue = 0;
};

} // extern "C"

#endif

#endif
