//
// Asp code generation routines from all classes.
//

#include "statement.hpp"
#include "expression.hpp"
#include "symbol.hpp"
#include "instruction.hpp"
#include "asp.h"
#include "opcode.h"
#include <map>
#include <sstream>
#include <iomanip>

using namespace std;

void Block::Emit(Executable &executable) const
{
    for (auto iter = statements.begin(); iter != statements.end(); iter++)
        (*iter)->Emit(executable);
}

void ExpressionStatement::Emit(Executable &executable) const
{
    expression->Emit(executable);
    executable.Insert
        (new PopInstruction("Pop unused value"),
         sourceLocation);
}

void AssignmentStatement::Emit(Executable &executable) const
{
    Emit1(executable, true);
}

void AssignmentStatement::Emit1(Executable &executable, bool top) const
{
    if (assignmentTokenType != TOKEN_ASSIGN)
        targetExpression->Emit(executable, Expression::EmitType::Value);

    if (valueAssignmentStatement != nullptr)
        valueAssignmentStatement->Emit1(executable, false);
    else
        valueExpression->Emit(executable);

    if (assignmentTokenType != TOKEN_ASSIGN)
    {
        static map<int, uint8_t> opCodes =
        {
            {TOKEN_BIT_OR_ASSIGN, OpCode_OR},
            {TOKEN_BIT_XOR_ASSIGN, OpCode_XOR},
            {TOKEN_BIT_AND_ASSIGN, OpCode_AND},
            {TOKEN_LEFT_SHIFT_ASSIGN, OpCode_LSH},
            {TOKEN_RIGHT_SHIFT_ASSIGN, OpCode_RSH},
            {TOKEN_PLUS_ASSIGN, OpCode_ADD},
            {TOKEN_MINUS_ASSIGN, OpCode_SUB},
            {TOKEN_TIMES_ASSIGN, OpCode_MUL},
            {TOKEN_DIVIDE_ASSIGN, OpCode_DIV},
            {TOKEN_FLOOR_DIVIDE_ASSIGN, OpCode_FDIV},
            {TOKEN_MODULO_ASSIGN, OpCode_MOD},
            {TOKEN_POWER_ASSIGN, OpCode_POW},
        };

        auto iter = opCodes.find(assignmentTokenType);
        if (iter == opCodes.end())
        {
            ostringstream oss;
            oss
                << "Internal error:"
                   " Cannot find op code for augmented assignment "
                << assignmentTokenType;
            ThrowError(oss.str());
        }
        ostringstream oss;
        oss
            << "Perform binary operation 0x"
            << hex << uppercase << setfill('0')
            << setw(2) << static_cast<unsigned>(iter->second);
        executable.Insert
            (new BinaryInstruction(iter->second, oss.str()),
             sourceLocation);
    }

    targetExpression->Emit(executable, Expression::EmitType::Address);
    executable.Insert
        (new SetInstruction
            (top,
             top ? "Assign with pop" : "Assign, leave value on stack"),
         sourceLocation);
}

void InsertionStatement::Emit(Executable &executable) const
{
    Emit1(executable, true);
}

void InsertionStatement::Emit1(Executable &executable, bool top) const
{
    if (containerInsertionStatement != nullptr)
        containerInsertionStatement->Emit1(executable, false);
    else
        containerExpression->Emit(executable);

    if (keyValuePair != nullptr)
        keyValuePair->Emit(executable);
    else
        itemExpression->Emit(executable);

    executable.Insert
        (new InsertInstruction
            (top,
             top ? "Insert with pop" : "Insert, leave container on stack"),
         sourceLocation);
}

void BreakStatement::Emit(Executable &executable) const
{
    auto loopStatement = ParentLoop();
    if (loopStatement == nullptr)
        ThrowError("break outside loop");

    executable.Insert
        (new JumpInstruction
            (loopStatement->EndLocation(), "Jump out of loop"),
         sourceLocation);
}

void ContinueStatement::Emit(Executable &executable) const
{
    auto loopStatement = ParentLoop();
    if (loopStatement == nullptr)
        ThrowError("continue outside loop");

    executable.Insert
        (new JumpInstruction
            (loopStatement->ContinueLocation(), "Jump to loop iteration"),
         sourceLocation);
}

void PassStatement::Emit(Executable &executable) const
{
    // Nothing to emit.
}

void ImportStatement::Emit(Executable &executable) const
{
    for (auto iter = moduleNameList->NamesBegin();
         iter != moduleNameList->NamesEnd(); iter++)
    {
        auto importName = *iter;
        auto moduleName = importName->Name();
        auto moduleSymbol = executable.Symbol(moduleName);

        {
            ostringstream oss;
            oss << "Load module " << moduleName;
            executable.Insert
                (new LoadModuleInstruction(moduleSymbol, oss.str()),
                 sourceLocation);
        }

        if (memberNameList == nullptr)
        {
            auto asName = importName->AsName();
            auto asNameSymbol = executable.Symbol(asName);

            {
                ostringstream oss;
                oss << "Push module " << moduleName;
                executable.Insert
                    (new PushModuleInstruction(moduleSymbol, oss.str()),
                     sourceLocation);
            }
            {
                ostringstream oss;
                oss << "Push address of variable " << asName;
                executable.Insert
                    (new LoadInstruction(asNameSymbol, true, oss.str()),
                     sourceLocation);
            }
            executable.Insert(new SetInstruction(true), sourceLocation);
        }
        else
        {
            for (auto iter = memberNameList->NamesBegin();
                 iter != memberNameList->NamesEnd(); iter++)
            {
                auto memberName = *iter;
                auto name = memberName->Name();
                auto nameSymbol = executable.Symbol(name);
                auto asName = memberName->AsName();
                auto asNameSymbol = executable.Symbol(asName);

                if (name == "*")
                    ThrowError("Wildcard form of from...import not permitted");

                {
                    ostringstream oss;
                    oss << "Push module " << moduleName;
                    executable.Insert
                        (new PushModuleInstruction(moduleSymbol, oss.str()),
                         sourceLocation);
                }
                {
                    ostringstream oss;
                    oss << "Look up member variable " << name;
                    executable.Insert
                        (new MemberInstruction(nameSymbol, false, oss.str()),
                         sourceLocation);
                }
                {
                    ostringstream oss;
                    oss << "Load address of variable " << asName;
                    executable.Insert
                        (new LoadInstruction(asNameSymbol, true, oss.str()),
                         sourceLocation);
                }
                executable.Insert(new SetInstruction(true), sourceLocation);
            }
        }
    }
}

void GlobalStatement::Emit(Executable &executable) const
{
    bool isLocal = ParentDef() != nullptr;
    if (!isLocal)
        ThrowError("global outside function");

    for (auto iter = variableList->NamesBegin();
         iter != variableList->NamesEnd(); iter++)
    {
        auto name = *iter;
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss << "Enable global override for variable " << name;
        executable.Insert
            (new GlobalInstruction(symbol, false, oss.str()),
             sourceLocation);
    }
}

void LocalStatement::Emit(Executable &executable) const
{
    bool isLocal = ParentDef() != nullptr;
    if (!isLocal)
        ThrowError("local outside function");

    for (auto iter = variableList->NamesBegin();
         iter != variableList->NamesEnd(); iter++)
    {
        auto name = *iter;
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss << "Disable global override for variable " << name;
        executable.Insert
            (new GlobalInstruction(symbol, true, oss.str()),
             sourceLocation);
    }
}

void DelStatement::Emit(Executable &executable) const
{
    Emit1(executable, expression);
}

void DelStatement::Emit1(Executable &executable, Expression *expression) const
{
    auto tupleExpression = dynamic_cast<TupleExpression *>(expression);
    auto elementExpression = dynamic_cast<ElementExpression *>(expression);
    auto memberExpression = dynamic_cast<MemberExpression *>(expression);
    auto variableExpression = dynamic_cast<VariableExpression *>(expression);
    if (tupleExpression != nullptr)
    {
        for (auto iter = tupleExpression->ExpressionsBegin();
             iter != tupleExpression->ExpressionsEnd(); iter++)
        {
            auto elementExpression = *iter;
            Emit1(executable, elementExpression);
        }
    }
    else if (elementExpression != nullptr)
    {
        elementExpression->Emit(executable, Expression::EmitType::Delete);
        executable.Insert
            (new EraseInstruction("Erase element"),
             sourceLocation);
    }
    else if (memberExpression != nullptr)
    {
        memberExpression->Emit(executable, Expression::EmitType::Delete);
        executable.Insert
            (new EraseInstruction("Erase member"),
             sourceLocation);
    }
    else if (variableExpression != nullptr)
    {
        if (variableExpression->HasSymbol())
            ThrowError("Cannot delete temporary variable");

        auto name = variableExpression->Name();
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss << "Delete variable " << name;
        executable.Insert
            (new DeleteInstruction(symbol, oss.str()),
             sourceLocation);
    }
    else
        ThrowError("Invalid type for del");
}

void ReturnStatement::Emit(Executable &executable) const
{
    // Ensure the return statement is within a function definition.
    auto isLocal = ParentDef() != nullptr;
    if (!isLocal)
        ThrowError("return outside function");

    if (expression != nullptr)
        expression->Emit(executable);
    else
    {
        // Use None as the return value.
        Expression *noneExpression = new ConstantExpression
            (Token(sourceLocation, TOKEN_NONE));
        noneExpression->Parent(this);
        noneExpression->Emit(executable);
        delete noneExpression;
    }

    executable.Insert(new ReturnInstruction, sourceLocation);
}

void AssertStatement::Emit(Executable &executable) const
{
    auto *constantExpression = dynamic_cast<const ConstantExpression *>
        (expression);
    if (constantExpression != nullptr && !constantExpression->IsTrue())
    {
        executable.Insert(new AbortInstruction, sourceLocation);
    }
    else
    {
        expression->Emit(executable);
        auto endLocation = executable.Insert
            (new NullInstruction, sourceLocation);

        executable.PushLocation(endLocation);
        executable.Insert
            (new ConditionalJumpInstruction
                (true, endLocation, "Jump if true to end"),
             sourceLocation);
        executable.Insert(new AbortInstruction, sourceLocation);
        executable.PopLocation();
    }
}

void IfStatement::Emit(Executable &executable) const
{
    conditionExpression->Emit(executable);

    auto elseLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    auto endLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    executable.PushLocation(elseLocation);
    executable.Insert
        (new ConditionalJumpInstruction
            (false, elseLocation, "Jump if false to else"),
         sourceLocation);
    trueBlock->Emit(executable);
    if (falseBlock != nullptr || elsePart != nullptr)
        executable.Insert
            (new JumpInstruction(endLocation, "Jump to end"),
             sourceLocation);
    executable.PopLocation();

    executable.PushLocation(endLocation);
    if (falseBlock != nullptr)
        falseBlock->Emit(executable);
    else if (elsePart != nullptr)
        elsePart->Emit(executable);
    executable.PopLocation();
}

void WhileStatement::Emit(Executable &executable) const
{
    int loopedVariableSymbol = 0;
    if (falseBlock != nullptr)
    {
        loopedVariableSymbol = executable.TemporarySymbol();

        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto falseExpression = new ConstantExpression
            (Token(sourceLocation, TOKEN_FALSE));
        AssignmentStatement initStatement
            (Token(sourceLocation, TOKEN_ASSIGN),
             loopedExpression, falseExpression);
        loopedExpression->Parent(&initStatement);
        falseExpression->Parent(&initStatement);
        initStatement.Parent(Parent());
        initStatement.Emit(executable);
    }

    continueLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    conditionExpression->Emit(executable);

    auto elseLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    endLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    executable.PushLocation(elseLocation);
    executable.Insert
        (new ConditionalJumpInstruction
            (false, elseLocation, "Jump if false to else"),
         sourceLocation);
    if (falseBlock != nullptr)
    {
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto trueExpression = new ConstantExpression
            (Token(sourceLocation, TOKEN_TRUE));
        AssignmentStatement signalStatement
            (Token(sourceLocation, TOKEN_ASSIGN),
             loopedExpression, trueExpression);
        loopedExpression->Parent(&signalStatement);
        trueExpression->Parent(&signalStatement);
        signalStatement.Parent(Parent());
        signalStatement.Emit(executable);
    }
    trueBlock->Emit(executable);
    executable.Insert
        (new JumpInstruction(continueLocation, "Jump to continue"),
         sourceLocation);
    executable.PopLocation();

    if (falseBlock != nullptr)
    {
        executable.PushLocation(endLocation);
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        try
        {
            loopedExpression->Parent(this);
            loopedExpression->Emit
                (executable, Expression::EmitType::Value);
        }
        catch (...)
        {
            delete loopedExpression;
            throw;
        }
        delete loopedExpression;
        executable.Insert
            (new ConditionalJumpInstruction
                (true, endLocation, "Jump if true to end"),
             sourceLocation);
        falseBlock->Emit(executable);
        executable.PopLocation();
    }
}

void ForStatement::Emit(Executable &executable) const
{
    int loopedVariableSymbol = 0;
    if (falseBlock != nullptr)
    {
        loopedVariableSymbol = executable.TemporarySymbol();

        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto falseExpression = new ConstantExpression
            (Token(sourceLocation, TOKEN_FALSE));
        AssignmentStatement initStatement
            (Token(sourceLocation, TOKEN_ASSIGN),
             loopedExpression, falseExpression);
        loopedExpression->Parent(&initStatement);
        falseExpression->Parent(&initStatement);
        initStatement.Parent(Parent());
        initStatement.Emit(executable);
    }

    iterableExpression->Emit(executable);
    executable.Insert(new StartIteratorInstruction, sourceLocation);

    auto testLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    continueLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    auto elseLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    endLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    executable.PushLocation(continueLocation);
    executable.Insert(new TestIteratorInstruction, sourceLocation);
    executable.Insert
        (new ConditionalJumpInstruction
            (false, elseLocation, "Jump if false to else"),
         sourceLocation);
    if (falseBlock != nullptr)
    {
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto trueExpression = new ConstantExpression
            (Token(sourceLocation, TOKEN_TRUE));
        AssignmentStatement signalStatement
            (Token(sourceLocation, TOKEN_ASSIGN),
             loopedExpression, trueExpression);
        loopedExpression->Parent(&signalStatement);
        trueExpression->Parent(&signalStatement);
        signalStatement.Parent(Parent());
        signalStatement.Emit(executable);
    }
    executable.Insert(new DereferenceIteratorInstruction, sourceLocation);
    targetExpression->Emit
        (executable, Expression::EmitType::Address);

    executable.Insert(new SetInstruction(true), sourceLocation);
    trueBlock->Emit(executable);
    executable.PopLocation();

    executable.PushLocation(elseLocation);
    executable.Insert(new AdvanceIteratorInstruction, sourceLocation);
    executable.Insert
        (new JumpInstruction(testLocation, "Jump to test"),
         sourceLocation);
    executable.PopLocation();

    executable.PushLocation(endLocation);
    if (falseBlock != nullptr)
    {
        executable.PushLocation(endLocation);
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        try
        {
            loopedExpression->Parent(this);
            loopedExpression->Emit
                (executable, Expression::EmitType::Value);
        }
        catch (...)
        {
            delete loopedExpression;
            throw;
        }
        delete loopedExpression;
        executable.Insert
            (new ConditionalJumpInstruction
                (true, endLocation, "Jump if true to end"),
             sourceLocation);
        falseBlock->Emit(executable);
        executable.PopLocation();
    }
    executable.PopLocation();

    executable.Insert(new PopInstruction, sourceLocation);
}

void Parameter::Emit(Executable &executable) const
{
    if (defaultExpression != nullptr)
        defaultExpression->Emit(executable);
    auto symbol = executable.Symbol(name);
    ostringstream oss;
    oss << "Make";
    if (type == Type::TupleGroup)
        oss << " tuple group";
    else if (type == Type::DictionaryGroup)
        oss << " dictionary group";
    oss << " parameter " << name;
    if (defaultExpression != nullptr)
        oss << " with default value";
    executable.Insert
        (new MakeParameterInstruction
            (symbol,
             type == Type::TupleGroup ?
                MakeParameterInstruction::Type::TupleGroup :
             type == Type::DictionaryGroup ?
                MakeParameterInstruction::Type::DictionaryGroup :
             HasDefault() ?
                MakeParameterInstruction::Type::Defaulted :
                MakeParameterInstruction::Type::Positional,
             oss.str()),
         sourceLocation);
}

void ParameterList::Emit(Executable &executable) const
{
    executable.Insert
        (new PushParameterListInstruction("Push empty parameter list"),
         sourceLocation);
    for (auto iter = parameters.begin();
         iter != parameters.end(); iter++)
    {
        auto parameter = *iter;
        parameter->Emit(executable);
        executable.Insert
            (new BuildInstruction("Add parameter to parameter list"),
             sourceLocation);
    }
}

void DefStatement::Emit(Executable &executable) const
{
    auto entryLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    auto defineLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    executable.PushLocation(entryLocation);
    executable.Insert
        (new JumpInstruction(defineLocation, "Jump around code"),
         sourceLocation);
    executable.PopLocation();

    executable.PushLocation(defineLocation);
    try
    {
        block->Emit(executable);

        // Emit a final return statement if needed.
        auto finalReturnStatement = dynamic_cast<const ReturnStatement *>
            (block->FinalStatement());
        if (finalReturnStatement == nullptr)
        {
            executable.Insert
                (new PushNoneInstruction("Push default return value)"),
                 sourceLocation);
            executable.Insert(new ReturnInstruction, sourceLocation);
        }
    }
    catch (...)
    {
        executable.PopLocation();
        throw;
    }
    executable.PopLocation();

    parameterList->Emit(executable);
    executable.Insert
        (new PushCodeAddressInstruction(entryLocation, "Push code address"),
         sourceLocation);
    executable.Insert(new MakeFunctionInstruction, sourceLocation);

    auto variableExpression = new VariableExpression
        (Token(sourceLocation, TOKEN_NAME, name));
    variableExpression->Parent(this);
    variableExpression->Emit
        (executable, Expression::EmitType::Address);
    delete variableExpression;
    executable.Insert(new SetInstruction(true), sourceLocation);
}

void ConditionalExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete value expression");

    conditionExpression->Emit(executable);

    auto falseLocation = executable.Insert
        (new NullInstruction, sourceLocation);
    auto endLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    executable.PushLocation(falseLocation);
    executable.Insert
        (new ConditionalJumpInstruction
            (false, falseLocation, "Jump if false to false expression"),
         sourceLocation);
    trueExpression->Emit(executable);
    executable.Insert
        (new JumpInstruction(endLocation, "Jump to end"),
         sourceLocation);
    executable.PopLocation();

    executable.PushLocation(endLocation);
    falseExpression->Emit(executable);
    executable.PopLocation();
}

void ShortCircuitLogicalExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete value expression");

    auto expressionIter = expressions.begin();
    auto leftExpression = *expressionIter;
    leftExpression->Emit(executable);
    expressionIter++;

    auto endLocation = executable.Insert
        (new NullInstruction, sourceLocation);

    static map<int, uint8_t> opCodes =
    {
        {TOKEN_OR, OpCode_LOR},
        {TOKEN_AND, OpCode_LAND},
    };
    auto iter = opCodes.find(operatorTokenType);
    if (iter == opCodes.end())
    {
        ostringstream oss;
        oss
            << "Internal error: Cannot find op code for binary operator "
            << operatorTokenType;
        ThrowError(oss.str());
    }
    ostringstream oss;
    oss
        << "Perform short-circuit logical operation 0x"
        << hex << uppercase << setfill('0')
        << setw(2) << static_cast<unsigned>(iter->second);

    executable.PushLocation(endLocation);
    for (; expressionIter != expressions.end(); expressionIter++)
    {
        auto expression = *expressionIter;

        executable.Insert
            (new LogicalInstruction(iter->second, endLocation, oss.str()),
             sourceLocation);
        expression->Emit(executable);
    }
    executable.PopLocation();
}

void BinaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete value expression");

    leftExpression->Emit(executable);
    rightExpression->Emit(executable);
    static map<int, uint8_t> opCodes =
    {
        {TOKEN_BAR, OpCode_OR},
        {TOKEN_CARET, OpCode_XOR},
        {TOKEN_AMPERSAND, OpCode_AND},
        {TOKEN_LEFT_SHIFT, OpCode_LSH},
        {TOKEN_RIGHT_SHIFT, OpCode_RSH},
        {TOKEN_PLUS, OpCode_ADD},
        {TOKEN_MINUS, OpCode_SUB},
        {TOKEN_ASTERISK, OpCode_MUL},
        {TOKEN_SLASH, OpCode_DIV},
        {TOKEN_FLOOR_DIVIDE, OpCode_FDIV},
        {TOKEN_PERCENT, OpCode_MOD},
        {TOKEN_DOUBLE_ASTERISK, OpCode_POW},

        {TOKEN_NE, OpCode_NE},
        {TOKEN_EQ, OpCode_EQ},
        {TOKEN_LT, OpCode_LT},
        {TOKEN_LE, OpCode_LE},
        {TOKEN_GT, OpCode_GT},
        {TOKEN_GE, OpCode_GE},
        {TOKEN_NOT_IN, OpCode_NIN},
        {TOKEN_IN, OpCode_IN},
        {TOKEN_IS_NOT, OpCode_NIS},
        {TOKEN_IS, OpCode_IS},
    };
    auto iter = opCodes.find(operatorTokenType);
    if (iter == opCodes.end())
    {
        ostringstream oss;
        oss
            << "Internal error: Cannot find op code for binary operator "
            << operatorTokenType;
        ThrowError(oss.str());
    }
    ostringstream oss;
    oss
        << "Perform binary operation 0x"
        << hex << uppercase << setfill('0')
        << setw(2) << static_cast<unsigned>(iter->second);
    executable.Insert
        (new BinaryInstruction(iter->second, oss.str()),
         sourceLocation);
}

void UnaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete value expression");

    expression->Emit(executable);

    static map<int, uint8_t> opCodes =
    {
        {TOKEN_NOT, OpCode_LNOT},
        {TOKEN_PLUS, OpCode_POS},
        {TOKEN_MINUS, OpCode_NEG},
        {TOKEN_TILDE, OpCode_NOT},
    };
    auto iter = opCodes.find(operatorTokenType);
    if (iter == opCodes.end())
    {
        ostringstream oss;
        oss
            << "Internal error: Cannot find op code for unary operator "
            << operatorTokenType;
        ThrowError(oss.str());
    }
    ostringstream oss;
    oss
        << "Perform unary operation 0x"
        << hex << uppercase << setfill('0')
        << setw(2) << static_cast<unsigned>(iter->second);
    executable.Insert
        (new UnaryInstruction(iter->second, oss.str()),
         sourceLocation);
}

void TargetExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType != EmitType::Address)
        ThrowError("Unexpected use of target expression");

    if (!name.empty())
    {
        if (!targetExpressions.empty())
            ThrowError("Internal error: Invalid target expression");

        auto symbol = executable.Symbol(name);
        ostringstream oss;
        oss << "Push address of variable " << name;
        executable.Insert
            (new LoadInstruction(symbol, true, oss.str()),
             sourceLocation);
    }
    else
    {
        executable.Insert
            (new PushTupleInstruction("Create empty tuple"),
             sourceLocation);

        for (auto iter = targetExpressions.begin();
             iter != targetExpressions.end(); iter++)
        {
            auto targetExpression = *iter;
            targetExpression->Emit(executable, emitType);
            executable.Insert
                (new BuildInstruction("Add item to tuple"),
                 sourceLocation);
        }
    }
}

void Argument::Emit(Executable &executable) const
{
    valueExpression->Emit(executable);
    if (!name.empty())
    {
        auto symbol = executable.Symbol(name);
        ostringstream oss;
        oss << "Make argument with name " << name;
        executable.Insert
            (new MakeArgumentInstruction(symbol, oss.str()),
             sourceLocation);
    }
    else
    {
        ostringstream oss;
        oss << "Make";
        if (type == Type::IterableGroup)
            oss << " iterable group";
        else if (type == Type::DictionaryGroup)
            oss << " dictionary group";
        else if (HasName())
            oss << " named";
        else
            oss << " positional";
        oss << " argument";
        executable.Insert
            (new MakeArgumentInstruction
                (type == Type::IterableGroup ?
                    MakeArgumentInstruction::Type::IterableGroup :
                 type == Type::DictionaryGroup ?
                    MakeArgumentInstruction::Type::DictionaryGroup :
                 HasName() ?
                    MakeArgumentInstruction::Type::Named :
                    MakeArgumentInstruction::Type::Positional,
                 oss.str()),
             sourceLocation);
    }
}

void ArgumentList::Emit(Executable &executable) const
{
    executable.Insert
        (new PushArgumentListInstruction("Push empty argument list"),
         sourceLocation);
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        auto argument = *iter;
        argument->Emit(executable);
        executable.Insert
            (new BuildInstruction("Add argument to argument list"),
             sourceLocation);
    }
}

void CallExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of function call");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete function call");

    argumentList->Emit(executable);
    functionExpression->Emit(executable);
    executable.Insert(new CallInstruction, sourceLocation);
}

void ElementExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    sequenceExpression->Emit(executable);
    indexExpression->Emit(executable);

    if (emitType == EmitType::Delete)
        return;

    ostringstream oss;
    oss
        << "Get " << (emitType == EmitType::Address ? "address" : "value")
        << " of element";
    executable.Insert
        (new IndexInstruction(emitType == EmitType::Address, oss.str()),
         sourceLocation);
}

void MemberExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    expression->Emit(executable);
    auto symbol = executable.Symbol(name);

    if (emitType == EmitType::Delete)
    {
        ostringstream oss;
        oss << "Push symbol of variable " << name;
        executable.Insert
            (new PushIntegerInstruction(symbol, oss.str()),
             sourceLocation);
        return;
    }

    ostringstream oss;
    oss
        << "Lookup " << (emitType == EmitType::Address ? "address" : "value")
        << " of member " << name;
    executable.Insert
        (new MemberInstruction
            (symbol, emitType == EmitType::Address, oss.str()),
         sourceLocation);
}

void VariableExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Delete)
        return;

    auto symbol = name.empty() ? this->symbol : executable.Symbol(name);

    ostringstream oss;
    oss
        << "Push " << (emitType == EmitType::Address ? "address" : "value")
        << " of variable " << name;
    executable.Insert
        (new LoadInstruction
            (symbol, emitType == EmitType::Address, oss.str()),
         sourceLocation);
}

void SymbolExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of symbol expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete symbol expression");

    auto nameSymbol = executable.Symbol(name);

    ostringstream oss;
    oss << "Push symbol of variable " << name;
    executable.Insert
        (new PushSymbolInstruction(nameSymbol, oss.str()),
         sourceLocation);
}

void KeyValuePair::Emit(Executable &executable) const
{
    valueExpression->Emit(executable);
    keyExpression->Emit(executable);
    executable.Insert(new MakeKeyValuePairInstruction, sourceLocation);
}

void DictionaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of dictionary expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete dictionary expression");

    executable.Insert
        (new PushDictionaryInstruction("Create empty dictionary"),
         sourceLocation);
    for (auto iter = entries.begin(); iter != entries.end(); iter++)
    {
        auto entry = *iter;
        entry->Emit(executable);
        executable.Insert
            (new BuildInstruction("Add entry to dictionary"),
             sourceLocation);
    }
}

void SetExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of set expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete set expression");

    executable.Insert
        (new PushSetInstruction("Create empty set"),
         sourceLocation);
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable);
        executable.Insert
            (new BuildInstruction("Add item to set"),
             sourceLocation);
    }
}

void ListExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Delete)
        ThrowError("Cannot delete list expression");

    executable.Insert
        (new PushListInstruction("Create empty list"),
         sourceLocation);
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable, emitType);
        executable.Insert
            (new BuildInstruction("Add item to list"),
             sourceLocation);
    }
}

void TupleExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Delete)
        ThrowError("Cannot delete tuple expression");

    executable.Insert
        (new PushTupleInstruction("Create empty tuple"),
         sourceLocation);
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable, emitType);
        executable.Insert
            (new BuildInstruction("Add item to tuple"),
             sourceLocation);
    }
}

void RangeExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of range expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete range expression");

    list<Expression *> partExpressions =
    {
        startExpression, endExpression, stepExpression
    };
    unsigned partCount = 0;
    for (auto iter = partExpressions.rbegin();
         iter != partExpressions.rend(); iter++)
    {
        auto partExpression = *iter;
        if (partExpression != nullptr)
        {
            partExpression->Emit(executable);
            partCount++;
        }
    }
    ostringstream oss;
    oss
        << "Make range of pattern "
        << (startExpression != nullptr ? "S" : "") << ".."
        << (endExpression != nullptr ? "E" : "") << ':'
        << (stepExpression != nullptr ? "T" : "");
    executable.Insert
        (new MakeRangeInstruction
            (startExpression != nullptr,
             endExpression != nullptr,
             stepExpression != nullptr,
             oss.str()),
         sourceLocation);
}

void ConstantExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        ThrowError("Cannot take address of constant expression");
    else if (emitType == EmitType::Delete)
        ThrowError("Cannot delete constant expression");

    switch (type)
    {
        case Type::None:
            executable.Insert(new PushNoneInstruction, sourceLocation);
            break;
        case Type::Ellipsis:
            executable.Insert(new PushEllipsisInstruction, sourceLocation);
            break;
        case Type::Boolean:
            executable.Insert(new PushBooleanInstruction(b), sourceLocation);
            break;
        case Type::Integer:
            executable.Insert(new PushIntegerInstruction(i), sourceLocation);
            break;
        case Type::NegatedMinInteger:
            ThrowError("Integer constant out of range");
        case Type::Float:
            executable.Insert(new PushFloatInstruction(f), sourceLocation);
            break;
        case Type::String:
            executable.Insert(new PushStringInstruction(s), sourceLocation);
            break;
    }
}
