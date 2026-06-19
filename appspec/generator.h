//
// Asp application specification generator definitions.
//

#ifndef GENERATOR_H
#define GENERATOR_H

#include "lexer.h"
#include "asp.h"

#ifdef __cplusplus
#include "statement.hpp"
#include "symbol.hpp"
#include <iostream>
#include <stack>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <memory>
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
DECLARE_TYPE(Literal)
DECLARE_TYPE(Statement)
DECLARE_TYPE(ParameterList)
DECLARE_TYPE(Parameter)
DECLARE_TYPE(NameList)

#ifndef __cplusplus
DECLARE_TYPE(Generator)
#else
extern "C" {

class Generator
{
    public:

        // Constructor, destructor.
        Generator
            (std::ostream &errorStream,
             const std::string &fileBaseName);

        // Generator methods.
        void AddModule(const std::string &);
        std::pair
            <std::string, // Module name
             std::list
                <std::pair
                    <std::string, // Import name
                     SourceElement> > >
            NextModule();
        void SetSourceLocation(const SourceLocation &);
        unsigned ErrorCount() const;
        bool Finalize();

        // Source file methods.
        void CurrentSource
            (const std::string &fileName,
             bool newFile = true, bool isLibrary = false,
             const SourceLocation & = SourceLocation());
        bool IsLibrary() const;
        const std::string &CurrentSourceFileName() const;
        const std::string &CurrentModuleName() const;
        SourceLocation CurrentSourceLocation() const;

        // Output methods.
        void WriteCompilerSpec(std::ostream &);
        void WriteApplicationHeader(std::ostream &) const;
        void WriteApplicationCode(std::ostream &) const;

#endif

    /* Statements. */
    DECLARE_METHOD
        (DeclareAsLibrary, NonTerminal *, Token *)
    DECLARE_METHOD
        (IncludeHeader, NonTerminal *, Token *)
    DECLARE_METHOD
        (ImportModule, NonTerminal *, Token *, Token *)
    DECLARE_METHOD
        (UpdateFeatures, NonTerminal *, Token *, int)
    DECLARE_METHOD
        (MakeAssignment, NonTerminal *, Token *, Literal *)
    DECLARE_METHOD
        (MakeFunction, NonTerminal *, Token *, ParameterList *, Token *)
    #ifdef ASP_FEATURE_CLASS
    DECLARE_METHOD
        (StartClass, NonTerminal *, Token *)
    #endif
    DECLARE_METHOD
        (DeleteDefinition, NonTerminal *, NameList *)
    DECLARE_METHOD
        (EndBlock, NonTerminal *, int)

    /* Parameters. */
    DECLARE_METHOD
        (MakeEmptyParameterList, ParameterList *, int)
    DECLARE_METHOD
        (AddParameterToList, ParameterList *,
         ParameterList *, Parameter *)
    DECLARE_METHOD
        (MakeParameter, Parameter *, Token *)
    DECLARE_METHOD
        (MakeDefaultedParameter, Parameter *, Token *, Literal *)
    DECLARE_METHOD
        (MakeTupleGroupParameter, Parameter *, Token *)
    DECLARE_METHOD
        (MakeDictionaryGroupParameter, Parameter *, Token *)

    /* Names. */
    DECLARE_METHOD
        (MakeEmptyNameList, NameList *, int)
    DECLARE_METHOD
        (AddNameToList, NameList *, NameList *, Token *)

    /* Literals. */
    DECLARE_METHOD
        (MakeLiteral, Literal *, Token *)

    /* Miscellaneous methods. */
    DECLARE_METHOD(FreeNonTerminal, void, NonTerminal *)
    DECLARE_METHOD(FreeToken, void, Token *)
    DECLARE_METHOD(ReportWarning, void, const char *)
    DECLARE_METHOD(ReportError, void, const char *)

#ifdef __cplusplus

    protected:

        // Copy prevention.
        Generator(const Generator &) = delete;
        Generator &operator =(const Generator &) = delete;

        // Internal utility methods.
        void ClearDefinition
            (const std::string &, const SourceElement &, bool warn = true);
        bool CheckReservedNameError(const Token &);

        // Diagnostic reporting methods.
        void ReportWarning(const std::string &) const;
        void ReportWarning(const std::string &, const SourceElement &) const;
        void ReportWarning(const std::string &, const SourceLocation &) const;
        void ReportError(const std::string &);
        void ReportError(const std::string &, const SourceElement &);
        void ReportError(const std::string &, const SourceLocation &);
        void ReportMessage
            (const std::string &, const SourceLocation &, bool error) const;

        // Check value computation methods.
        std::uint32_t CheckValue() const;
        std::uint32_t ComputeCheckValue() const;

        // Modules and definitions navigation helper methods.
        void ForEachModuleAndDefinition
            (void (Generator::*moduleStartAction)
                (const std::set<std::string> &key, const std::string &name,
                 void *arg) const,
             void (Generator::*definitionStartAction)
                (const std::string &localName,
                 const std::string &qualifiedName,
                 const SourceElement &definition,
                 void *arg) const,
             void (Generator::*definitionEndAction)
                (const std::string &localName,
                 const std::string &qualifiedName,
                 const SourceElement &definition,
                 void *arg) const,
             void (Generator::*moduleEndAction)
                (const std::set<std::string> &key, const std::string &name,
                 void *arg) const,
             void *arg) const;
        void AssignDefinitionSymbols
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void AssignQualifiedNameSymbol
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void WriteApplicationHeaderDefinition
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void WriteApplicationDispatchCodeModuleStart
            (const std::set<std::string> &key, const std::string &name,
             void *arg) const;
        void WriteApplicationDispatchCodeDefinition
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void WriteApplicationDispatchCodeModuleEnd
            (const std::set<std::string> &key, const std::string &name,
             void *arg) const;
        void WriteApplicationAppSpecCodeModule
            (const std::set<std::string> &key, const std::string &name,
             void *arg) const;
        void WriteApplicationAppSpecCodeDefinitionStart
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void WriteApplicationAppSpecCodeDefinitionEnd
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;
        void ComputeCheckValueModule
            (const std::set<std::string> &key, const std::string &name,
             void *arg) const;
        void ComputeCheckValueDefinition
            (const std::string &localName, const std::string &qualifiedName,
             const SourceElement &definition,
             void *) const;

        // Internal data structures.
        struct NameInfo
        {
             NameInfo(unsigned useCount = 0) :
                 useCount(useCount)
             {
             }

             unsigned useCount;
             std::list<SourceElement> sourceElements;
        };
        struct ModuleDefinitionsInfo
        {
            ModuleDefinitionsInfo
                (const std::string &moduleName,
                 std::shared_ptr<DefinitionMap> definitions) :
                moduleName(moduleName),
                definitions(definitions)
            {
            }

            std::string moduleName;
            std::shared_ptr<DefinitionMap> definitions;
        };

    private:

        // Error reporting data.
        std::ostream &errorStream;
        unsigned errorCount = 0;

        // Source code processing data.
        std::string fileBaseName, variableBaseName;
        uint8_t compilerAppSpecVersion = 1u;
        uint8_t engineAppSpecVersion = 0;
        bool newFile = true, isLibrary = false;
        SourceLocation currentSourceLocation;
        std::string currentSourceFileName, currentModuleName;
        std::shared_ptr<DefinitionMap> currentDefinitions;
        std::stack<std::shared_ptr<DefinitionMap> > definitionsStack;
        std::set<std::string> moduleNames;
        std::deque<std::string> moduleNamesToImport;
        std::map
            <std::string, // Import name
             std::pair
                <std::string, // Module name
                 NameInfo> >
            imports;
        std::map
            <std::string, // Module name
             std::map
                <std::string, // Import name
                 NameInfo> >
            importedModules;
        std::map
            <std::string, // Module name
             std::shared_ptr<DefinitionMap> >
            definitionsByModuleName;

        // Code generation data.
        SymbolTable moduleIdTable, symbolTable;
        std::int32_t lastLocalSymbol;
        std::map
            <std::set<std::string>, // Module key
             ModuleDefinitionsInfo>
            definitionsByModuleKey;
        bool finalized = false, symbolsAssigned = false;
        std::uint32_t checkValue = 0;

        // Feature data.
        AspFeatureBits featureBits = 0;
};

} // extern "C"

#endif

#endif
