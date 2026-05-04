//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "app.h"
#include "asp.h"
#include "function.hpp"
#include "reserved.h"
#include "grammar.hpp"
#include <iostream>
#include <sstream>
#include <cctype>

using namespace std;

Generator::Generator
    (ostream &errorStream,
     const string &fileBaseName) :
    errorStream(errorStream),
    fileBaseName(fileBaseName),
    variableBaseName(fileBaseName),
    featureBits(featureBits)
{
    // Reserve module ID zero for the system module.
    moduleIdTable.ReserveSystemSymbol(0, "");

    // Deal with invalid variable name characters in the file name.
    for (string::iterator si = variableBaseName.begin();
         si != variableBaseName.end();
         si++)
    {
        auto c = *si;
        if (!isalnum(c) && c != '_')
            *si = '_';
    }
}

void Generator::AddModule(const string &moduleName)
{
    // Add the module only if it has never been added before.
    auto iter = moduleNames.find(moduleName);
    if (iter == moduleNames.end())
    {
        moduleNames.insert(moduleName);
        moduleNamesToImport.push_back(moduleName);
    }
}

pair<string, list<pair<string, SourceElement> > >
Generator::NextModule()
{
    // Check if there are any more modules to import.
    if (moduleNamesToImport.empty())
    {
        currentModuleName.clear();
        return make_pair
            ("", list<pair<string, SourceElement> >());
    }

    // Switch to the next module.
    auto moduleName = moduleNamesToImport.front();
    currentModuleName = definitionsByModuleName.empty() ?  "" : moduleName;
    moduleNamesToImport.pop_front();
    currentModuleDefinitions = definitionsByModuleName.emplace
        (currentModuleName, new map<string, shared_ptr<SourceElement> >)
        .first->second;

    // Gather all the import source locations that reference the module.
    list<pair<string, SourceElement> > sourceReferences;
    auto importedModuleIter = importedModules.find(currentModuleName);
    if (importedModuleIter != importedModules.end())
    {
        for (const auto &importEntry: importedModuleIter->second)
        {
            const auto &importName = importEntry.first;

            for (const auto &sourceElement: importEntry.second.sourceElements)
                sourceReferences.emplace_back
                    (make_pair(importName, sourceElement));
        }
    }

    return make_pair(moduleName, sourceReferences);
}

void Generator::SetSourceLocation(const SourceLocation &sourceLocation)
{
    currentSourceLocation = sourceLocation;
}

unsigned Generator::ErrorCount() const
{
    return errorCount;
}

void Generator::Finalize()
{
    // Discard module names as they are no longer needed.
    moduleNames.clear();

    // Reserve system symbols.
    symbolTable.ReserveSystemSymbols(featureBits);

    // Prepare to write the feature bits to the app spec file if applicable.
    if (featureBits != 0)
        compilerAppSpecVersion = 3u;

    // Reorganize modules into a well-defined order that does not depend on
    // the module names, but rather the set of associated import names,
    // dropping any modules that have no remaining imports due to deletions or
    // replacements.
    for (const auto &moduleEntry: definitionsByModuleName)
    {
        const auto &moduleName = moduleEntry.first;

        const auto importedModuleIter = importedModules.find(moduleName);
        size_t keyCount =
            importedModuleIter == importedModules.end() ?
            0 : importedModuleIter->second.size();
        if (keyCount == 0 && !moduleName.empty())
            continue;

        // Create a unique module key comprised of all its import names.
        set<string> moduleKey;
        if (keyCount != 0)
        {
            for (const auto &importEntry: importedModuleIter->second)
                moduleKey.emplace(importEntry.first);
        }

        // Copy the module's definitions.
        definitionsByModuleKey.emplace
            (moduleKey, ModuleDefinitionsInfo(moduleName, moduleEntry.second));
    }

    // Set the app spec format versions to support app modules if required.
    if (definitionsByModuleKey.size() > 1u)
    {
        if (compilerAppSpecVersion < 2u)
            compilerAppSpecVersion = 2u;
        if (engineAppSpecVersion < 1u)
            engineAppSpecVersion = 1u;
    }

    // Perform a final pass over all the modules and their definitions.
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        // Assign a module identifier.
        moduleIdTable.Symbol(moduleEntry.second.moduleName);

        // Set the required engine spec format to support functions with a
        // large number of parameters if necessary.
        if (engineAppSpecVersion < 1u)
        {
            for (const auto &definitionEntry: *moduleEntry.second.definitions)
            {
                const auto definition = definitionEntry.second.get();
                const auto functionDefinition =
                    dynamic_cast<const FunctionDefinition *>(definition);
                if (functionDefinition != nullptr &&
                    functionDefinition->Parameters().ParametersSize()
                    > AppSpecPrefix_MaxFunctionParameterCount)
                {
                    engineAppSpecVersion = 1u;
                    break;
                }
            }
        }
    }

    // Discard data that is no longer needed.
    importedModules.clear();
    definitionsByModuleName.clear();

    // Compute the CRC.
    checkValue = ComputeCheckValue();

    finalized = true;
}

void Generator::CurrentSource
    (const string &sourceFileName,
     bool newFile, bool isLibrary,
     const SourceLocation &sourceLocation)
{
    this->newFile = newFile;
    this->isLibrary = isLibrary;
    currentSourceFileName = sourceFileName;
    currentSourceLocation = sourceLocation;
}

bool Generator::IsLibrary() const
{
    return isLibrary;
}

const string &Generator::CurrentSourceFileName() const
{
    return currentSourceFileName;
}

const string &Generator::CurrentModuleName() const
{
    return currentModuleName;
}

SourceLocation Generator::CurrentSourceLocation() const
{
    return currentSourceLocation;
}

#define DEFINE_ACTION(...) DEFINE_ACTION_N(__VA_ARGS__, \
    DEFINE_ACTION_3, ~, \
    DEFINE_ACTION_2, ~, \
    DEFINE_ACTION_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_ACTION_N(_1, _2, _3, _4, _5, _6, _7, _8, arg, ...) arg
#define DEFINE_ACTION_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Generator *generator, type param) \
    {DO_ACTION(generator->action(param));} \
    ReturnType Generator::action(type param)
#define DEFINE_ACTION_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2) \
    {DO_ACTION(generator->action(p1, p2));} \
    ReturnType Generator::action \
        (t1 p1, t2 p2)
#define DEFINE_ACTION_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2, t3 p3) \
    {DO_ACTION(generator->action(p1, p2, p3));} \
    ReturnType Generator::action \
        (t1 p1, t2 p2, t3 p3)
#define DO_ACTION(action) \
    auto result = (action); \
    if (result && result->sourceLocation.line != 0) \
        generator->currentSourceLocation = result->sourceLocation; \
    return result;

#define DEFINE_UTIL(...) DEFINE_UTIL_N(__VA_ARGS__, \
    DEFINE_UTIL_3, ~, \
    DEFINE_UTIL_2, ~, \
    DEFINE_UTIL_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_UTIL_N(_1, _2, _3, _4, _5, _6, _7, _8, arg, ...) arg
#define DEFINE_UTIL_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Generator *generator, type param) \
    {generator->action(param);} \
    ReturnType Generator::action(type param)
#define DEFINE_UTIL_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2) \
    {generator->action(p1, p2);} \
    ReturnType Generator::action \
        (t1 p1, t2 p2)
#define DEFINE_UTIL_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2, t3 p3) \
    {generator->action(p1, p2, p3);} \
    ReturnType Generator::action \
        (t1 p1, t2 p2, t3 p3)

DEFINE_ACTION
    (DeclareAsLibrary, NonTerminal *, Token *, commandToken)
{
    // Allow library declaration only as the first statement.
    if (!newFile)
    {
        ReportError("lib must be the first statement", *commandToken);
        return nullptr;
    }
    newFile = false;

    isLibrary = true;

    // Clean up grammar elements.
    delete commandToken;

    return nullptr;
}

DEFINE_ACTION
    (IncludeHeader, NonTerminal *, Token *, includeNameToken)
{
    newFile = false;

    if (includeNameToken->s.empty())
    {
        ReportError("Include name cannot be empty", *includeNameToken);
        return nullptr;
    }

    auto newSourceFileName = includeNameToken->s + ".asps";
    if (newSourceFileName == currentSourceFileName)
    {
        ostringstream oss;
        oss << "Source file cannot include itself: " << newSourceFileName;
        ReportError(oss.str(), *includeNameToken);
        return nullptr;
    }

    // Switch to the new source file.
    currentSourceFileName = includeNameToken->s + ".asps";
    currentSourceLocation = includeNameToken->sourceLocation;

    // Clean up grammar elements.
    delete includeNameToken;

    return nullptr;
}

DEFINE_ACTION
    (ImportModule, NonTerminal *,
     Token *, moduleNameToken, Token *, asNameToken)
{
    newFile = false;

    if (moduleNameToken->s.empty())
    {
        ReportError("Module name cannot be empty", *moduleNameToken);
        return nullptr;
    }
    if (CheckReservedNameError(*asNameToken))
        return nullptr;

    // Ensure the import name has not been previously associated with a
    // different module.
    auto findImportIter = imports.find(asNameToken->s);
    if (findImportIter != imports.end() &&
        moduleNameToken->s != findImportIter->second.first)
    {
        // Report the error on the line on which it occurs.
        ostringstream oss;
        oss
            << "Cannot import module '" << moduleNameToken->s
            << "' as '" << asNameToken->s << "\' ...";
        ReportError(oss.str(), *asNameToken);

        // Report all the places where the name was previously defined.
        const auto &previousModuleName = findImportIter->second.first;
        const auto sourceElements =
            findImportIter->second.second.sourceElements;
        for (const auto &sourceElement: sourceElements)
        {
            ostringstream oss;
            oss
                << "... Module '" << previousModuleName
                << "' was previously imported as '" << asNameToken->s
                << "' here";
            ReportError(oss.str(), sourceElement);
        }

        return nullptr;
    }

    // Add the import to the global collection of import names, associating it
    // with the referenced module, or update its use count.
    auto globalImportIter = imports.emplace
        (asNameToken->s, make_pair(moduleNameToken->s, 0)).first;
    globalImportIter->second.second.useCount++;
    globalImportIter->second.second.sourceElements.push_back(*asNameToken);

    // Add the import to the collection of imports for the referenced module,
    // or update its use count.
    auto importedModuleIter = importedModules.emplace
        (moduleNameToken->s, map<string, NameInfo>()).first;
    auto moduleImportIter = importedModuleIter->second.emplace
        (asNameToken->s, 0).first;
    moduleImportIter->second.useCount++;
    moduleImportIter->second.sourceElements.push_back(*asNameToken);

    // Add the module.
    AddModule(moduleNameToken->s);

    currentSourceLocation = asNameToken->sourceLocation;

    // Clean up grammar elements.
    if (asNameToken != moduleNameToken)
        delete asNameToken;
    delete moduleNameToken;

    return nullptr;
}

DEFINE_ACTION
    (UpdateFeatures, NonTerminal *, Token *, featureToken, int, add)
{
    AspFeatureBits featureBit = 0;
    #ifdef ASP_FEATURE_CLASS
    if (featureToken->s == "class")
        featureBit = AspFeatureBit_Class;
    #endif
    if (featureBit == 0)
    {
        ostringstream oss;
        oss << "Unknown feature '" << featureToken->s << '\'';
        ReportError(oss.str(), *featureToken);
    }

    if (add)
        featureBits |= featureBit;
    else
        featureBits &= ~featureBit;

    delete featureToken;

    return nullptr;
}

DEFINE_ACTION
    (MakeAssignment, NonTerminal *,
     Token *, nameToken, Literal *, value)
{
    newFile = false;

    if (value != nullptr && CheckReservedNameError(*nameToken))
        return nullptr;

    // Replace any previous definition having the same name with this one.
    ClearDefinition(nameToken->s, *nameToken);

    // Add the assignment definition to the current module.
    currentModuleDefinitions->emplace
        (nameToken->s, new Assignment(*nameToken, value));

    currentSourceLocation = nameToken->sourceLocation;

    // Clean up grammar elements.
    delete nameToken;

    return nullptr;
}

DEFINE_ACTION
    (MakeFunction, NonTerminal *,
     Token *, nameToken, ParameterList *, parameterList,
     Token *, internalNameToken)
{
    newFile = false;

    if (CheckReservedNameError(*nameToken))
        return nullptr;

    // Ensure the validity of the order of parameter types.
    int position = 1;
    ValidFunctionDefinition validFunctionDefinition;
    for (auto iter = parameterList->ParametersBegin();
         validFunctionDefinition.IsValid() &&
         iter != parameterList->ParametersEnd();
         iter++, position++)
    {
        const auto &parameter = **iter;

        ValidFunctionDefinition::ParameterType type;
        bool typeValid = true;
        switch (parameter.GetType())
        {
            default:
                typeValid = false;
                break;
            case Parameter::Type::Positional:
                type = ValidFunctionDefinition::ParameterType::Positional;
                break;
            case Parameter::Type::TupleGroup:
                type = ValidFunctionDefinition::ParameterType::TupleGroup;
                break;
            case Parameter::Type::DictionaryGroup:
                type = ValidFunctionDefinition::ParameterType::DictionaryGroup;
                break;
        }
        if (!typeValid)
        {
            ReportError("Internal error: Unknown parameter type", parameter);
            break;
        }

        string error = validFunctionDefinition.AddParameter
            (parameter.Name(), type, parameter.DefaultValue() != nullptr);
        if (!error.empty())
            ReportError(error, parameter);
    }

    // Replace any previous definition having the same name with this one.
    ClearDefinition(nameToken->s, *nameToken);

    // Add the function definition to the current module.
    currentModuleDefinitions->emplace
        (nameToken->s, new FunctionDefinition
            (*nameToken, isLibrary,
             *internalNameToken, parameterList));

    currentSourceLocation = nameToken->sourceLocation;

    // Clean up grammar elements.
    delete nameToken;
    delete internalNameToken;

    return nullptr;
}

DEFINE_ACTION
    (DeleteDefinition, NonTerminal *, NameList *, nameList)
{
    for (auto iter = nameList->NamesBegin();
         iter != nameList->NamesEnd();
         iter++)
    {
        const auto &name = *iter;

        // Ensure the name exists in the current module.
        const auto findIter = currentModuleDefinitions->find(name);
        if (findIter == currentModuleDefinitions->end())
        {
            ostringstream oss;
            oss << "Cannot delete '" << name << '\'' << "; not found";
            ReportError(oss.str(), *nameList);
            continue;
        }

        // Drop the definition from the current module.
        ClearDefinition(name, *nameList, false);
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeEmptyParameterList, ParameterList *, int, _)
{
    return new ParameterList;
}

DEFINE_ACTION
    (AddParameterToList, ParameterList *,
     ParameterList *, parameterList, Parameter *, parameter)
{
    parameterList->Add(parameter);
    return parameterList;
}

DEFINE_ACTION
    (MakeParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDefaultedParameter, Parameter *,
     Token *, nameToken, Literal *, defaultValue)
{
    auto result = new Parameter(*nameToken, defaultValue);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeTupleGroupParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken, Parameter::Type::TupleGroup);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDictionaryGroupParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken, Parameter::Type::DictionaryGroup);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeEmptyNameList, NameList *, int, _)
{
    return new NameList;
}

DEFINE_ACTION
    (AddNameToList, NameList *, NameList *, nameList, Token *, nameToken)
{
    nameList->Add(*nameToken);
    delete nameToken;
    return nameList;
}

DEFINE_ACTION
    (MakeLiteral, Literal *, Token *, token)
{
    try
    {
        auto result = new Literal(*token);
        delete token;
        return result;
    }
    catch (const pair<SourceElement, string> &e)
    {
        ReportError(e.second, e.first);
    }
    catch (const string &e)
    {
        ReportError(e, *token);
    }

    return nullptr;
}

DEFINE_UTIL(FreeNonTerminal, void, NonTerminal *, nt)
{
    delete nt;
}

DEFINE_UTIL(FreeToken, void, Token *, token)
{
    delete token;
}

DEFINE_UTIL(ReportWarning, void, const char *, error)
{
     ReportWarning(string(error));
}

DEFINE_UTIL(ReportError, void, const char *, error)
{
     ReportError(string(error));
}

void Generator::ClearDefinition
    (const string &name, const SourceElement &sourceElement, bool warn)
{
    // Determine whether the name is already defined.
    auto findDefinitionIter = currentModuleDefinitions->find(name);
    if (findDefinitionIter == currentModuleDefinitions->end())
        return;

    // Issue a warning if applicable.
    if (warn)
    {
        ostringstream oss;
        oss << "Name '" << name << "' redefined";
        ReportWarning(oss.str(), sourceElement);
    }

    // Drop the definition from the current module.
    currentModuleDefinitions->erase(findDefinitionIter);
}

bool Generator::CheckReservedNameError(const Token &nameToken)
{
    /* Prevent certain reserved names from being used. */
    int32_t reservedSymbols[] =
    {
        AspReservedSymbol_SystemModule,
        AspReservedSymbol_SystemArguments,
        AspReservedSymbol_MainModule,
    };
    for (auto symbol: reservedSymbols)
    {
        if (nameToken.s == AspReservedName(symbol))
        {
            ostringstream oss;
            oss << "Cannot redefine reserved name '" << nameToken.s << '\'';
            ReportError(oss.str(), nameToken);
            return true;
        }
    }

    return false;
}

void Generator::ReportWarning(const string &message) const
{
    ReportWarning(message, currentSourceLocation);
}

void Generator::ReportWarning
    (const string &message, const SourceElement &sourceElement) const
{
    ReportWarning(message, sourceElement.sourceLocation);
}

void Generator::ReportWarning
    (const string &message, const SourceLocation &sourceLocation) const
{
    ReportMessage(message, sourceLocation, false);
}

void Generator::ReportError(const string &message)
{
    ReportError(message, currentSourceLocation);
}

void Generator::ReportError
    (const string &message, const SourceElement &sourceElement)
{
    ReportError(message, sourceElement.sourceLocation);
}

void Generator::ReportError
    (const string &message, const SourceLocation &sourceLocation)
{
    ReportMessage(message, sourceLocation, true);
    errorCount++;
}

void Generator::ReportMessage
    (const string &message, const SourceLocation &sourceLocation, bool error)
    const
{
    if (sourceLocation.Defined())
        errorStream
            << sourceLocation.fileName << ':'
            << sourceLocation.line << ':'
            << sourceLocation.column << ": ";
    errorStream
        << (error ? "Error" : "Warning") << ": " << message << endl;
}
