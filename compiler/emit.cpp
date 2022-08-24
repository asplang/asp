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
    executable.Insert(new PopInstruction("Pop unused value"));
}

void AssignmentStatement::Emit(Executable &executable) const
{
    Emit1(executable, true);
}

void AssignmentStatement::Emit1(Executable &executable, bool top) const
{
    if (valueAssignmentStatement != 0)
        valueAssignmentStatement->Emit1(executable, false);
    else
        valueExpression->Emit(executable);

    if (assignmentTokenType != TOKEN_ASSIGN)
    {
        targetExpression->Emit(executable, Expression::EmitType::Value);
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
            throw oss.str();
        }
        ostringstream oss;
        oss
            << "Perform binary operation 0x"
            << hex << setfill('0') << setw(2)
            << static_cast<unsigned>(iter->second);
        executable.Insert(new BinaryInstruction(iter->second, oss.str()));
    }

    targetExpression->Emit(executable, Expression::EmitType::Address);
    executable.Insert(new SetInstruction(top,
        top ? "Assign with pop" : "Assign, leave value on stack"));
}

void InsertionStatement::Emit(Executable &executable) const
{
    Emit1(executable, true);
}

void InsertionStatement::Emit1(Executable &executable, bool top) const
{
    if (containerInsertionStatement != 0)
        containerInsertionStatement->Emit1(executable, false);
    else
        containerExpression->Emit(executable);

    if (keyValuePair != 0)
        keyValuePair->Emit(executable);
    else
        itemExpression->Emit(executable);

    executable.Insert(new InsertInstruction(top,
        top ? "Insert with pop" : "Insert, leave container on stack"));
}

void BreakStatement::Emit(Executable &executable) const
{
    auto loopStatement = ParentLoop();
    if (loopStatement == 0)
        throw string("break outside loop");

    executable.Insert(new JumpInstruction
        (loopStatement->EndLocation(), "Jump out of loop"));
}

void ContinueStatement::Emit(Executable &executable) const
{
    auto loopStatement = ParentLoop();
    if (loopStatement == 0)
        throw string("continue outside loop");

    executable.Insert(new JumpInstruction
        (loopStatement->ContinueLocation(), "Jump to loop iteration"));
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
            oss
                << "Load module " << moduleName
                << " (" << moduleSymbol << ')';
            executable.Insert(new LoadModuleInstruction
                (moduleSymbol, oss.str()));
        }

        if (memberNameList == 0)
        {
            auto asName = importName->AsName();
            auto asNameSymbol = executable.Symbol(asName);

            {
                ostringstream oss;
                oss
                    << "Push module " << moduleName
                    << " (" << moduleSymbol << ')';
                executable.Insert(new PushModuleInstruction
                    (moduleSymbol, oss.str()));
            }
            {
                ostringstream oss;
                oss
                    << "Push address of variable " << asName
                    << " (" << asNameSymbol << ')';
                executable.Insert(new LoadInstruction
                    (asNameSymbol, true, oss.str()));
            }
            executable.Insert(new SetInstruction(true));
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

                {
                    ostringstream oss;
                    oss
                        << "Push module " << moduleName
                        << " (" << moduleSymbol << ')';
                    executable.Insert(new PushModuleInstruction
                        (moduleSymbol, oss.str()));
                }
                {
                    ostringstream oss;
                    oss
                        << "Look up member variable " << name
                        << " (" << nameSymbol << ')';
                    executable.Insert(new MemberInstruction
                        (nameSymbol, false, oss.str()));
                }
                {
                    ostringstream oss;
                    oss
                        << "Load address of variable " << asName
                        << " (" << asNameSymbol << ')';
                    executable.Insert(new LoadInstruction
                        (asNameSymbol, true, oss.str()));
                }
                executable.Insert(new SetInstruction(true));
            }
        }
    }
}

void VariableList::Emit(Executable &executable) const
{
    executable.Insert(new PushTupleInstruction("Push a new tuple"));
    for (auto iter = names.begin(); iter != names.end(); iter++)
    {
        auto &name = *iter;
        auto symbol = executable.Symbol(name);
        ostringstream oss;
        oss
            << "Load address of variable " << name
            << " (" << symbol << ')';
        executable.Insert(new LoadInstruction(symbol, true, oss.str()));
        executable.Insert(new BuildInstruction("Add address to tuple"));
    }
}

void GlobalStatement::Emit(Executable &executable) const
{
    bool isLocal = ParentDef() != 0;
    if (!isLocal)
        throw string("global outside function");

    for (auto iter = variableList->NamesBegin();
         iter != variableList->NamesEnd(); iter++)
    {
        auto name = *iter;
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss
            << "Enable global override for variable " << name
            << " (" << symbol << ')';
        executable.Insert(new GlobalInstruction(symbol, false, oss.str()));
    }
}

void LocalStatement::Emit(Executable &executable) const
{
    bool isLocal = ParentDef() != 0;
    if (!isLocal)
        throw string("local outside function");

    for (auto iter = variableList->NamesBegin();
         iter != variableList->NamesEnd(); iter++)
    {
        auto name = *iter;
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss
            << "Disable global override for variable " << name
            << " (" << symbol << ')';
        executable.Insert(new GlobalInstruction(symbol, true, oss.str()));
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
    if (tupleExpression != 0)
    {
        for (auto iter = tupleExpression->ExpressionsBegin();
             iter != tupleExpression->ExpressionsEnd(); iter++)
        {
            auto elementExpression = *iter;
            Emit1(executable, elementExpression);
        }
    }
    else if (elementExpression != 0)
    {
        elementExpression->Emit(executable, Expression::EmitType::Delete);
        executable.Insert(new EraseInstruction("Erase element"));
    }
    else if (memberExpression != 0)
    {
        memberExpression->Emit(executable, Expression::EmitType::Delete);
        executable.Insert(new EraseInstruction("Erase member"));
    }
    else if (variableExpression != 0)
    {
        if (variableExpression->HasSymbol())
            throw string("Cannot delete temporary variable");

        auto name = variableExpression->Name();
        auto symbol = executable.Symbol(name);

        ostringstream oss;
        oss
            << "Delete variable " << name
            << " (" << symbol << ')';
        executable.Insert(new DeleteInstruction(symbol, oss.str()));
    }
    else
        throw string("Invalid type for del");
}

void ReturnStatement::Emit(Executable &executable) const
{
    // Ensure the return statement is within a function definition.
    auto isLocal = ParentDef() != 0;
    if (!isLocal)
        throw string("return outside function");

    if (expression != 0)
        expression->Emit(executable);
    else
    {
        // Use None as the return value.
        Expression *noneExpression = new ConstantExpression
            (Token((SourceLocation &)*this, TOKEN_NONE));
        noneExpression->Parent(this);
        noneExpression->Emit(executable);
        delete noneExpression;
    }

    executable.Insert(new ReturnInstruction);
}

void IfStatement::Emit(Executable &executable) const
{
    conditionExpression->Emit(executable);

    auto elseLocation = executable.Insert(new NullInstruction);
    auto endLocation = executable.Insert(new NullInstruction);

    executable.PushLocation(elseLocation);
    executable.Insert(new ConditionalJumpInstruction
        (false, elseLocation, "Jump if false to else"));
    trueBlock->Emit(executable);
    if (falseBlock != 0 || elsePart != 0)
        executable.Insert(new JumpInstruction
            (endLocation, "Jump to end"));
    executable.PopLocation();

    executable.PushLocation(endLocation);
    if (falseBlock != 0)
        falseBlock->Emit(executable);
    else if (elsePart != 0)
        elsePart->Emit(executable);
    executable.PopLocation();
}

void WhileStatement::Emit(Executable &executable) const
{
    int loopedVariableSymbol = 0;
    if (falseBlock != 0)
    {
        loopedVariableSymbol = executable.TemporarySymbol();

        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto falseExpression = new ConstantExpression
            (Token((SourceLocation &)*this, TOKEN_FALSE));
        AssignmentStatement initStatement
            (Token((SourceLocation &)*this, TOKEN_ASSIGN),
             loopedExpression, falseExpression);
        loopedExpression->Parent(&initStatement);
        falseExpression->Parent(&initStatement);
        initStatement.Parent(Parent());
        initStatement.Emit(executable);
    }

    continueLocation = executable.Insert(new NullInstruction);
    conditionExpression->Emit(executable);

    auto elseLocation = executable.Insert(new NullInstruction);
    endLocation = executable.Insert(new NullInstruction);

    executable.PushLocation(elseLocation);
    executable.Insert(new ConditionalJumpInstruction
        (false, elseLocation, "Jump if false to else"));
    if (falseBlock != 0)
    {
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto trueExpression = new ConstantExpression
            (Token((SourceLocation &)*this, TOKEN_TRUE));
        AssignmentStatement signalStatement
            (Token((SourceLocation &)*this, TOKEN_ASSIGN),
             loopedExpression, trueExpression);
        loopedExpression->Parent(&signalStatement);
        trueExpression->Parent(&signalStatement);
        signalStatement.Parent(Parent());
        signalStatement.Emit(executable);
    }
    trueBlock->Emit(executable);
    executable.Insert(new JumpInstruction
        (continueLocation, "Jump to continue"));
    executable.PopLocation();

    if (falseBlock != 0)
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
        executable.Insert(new ConditionalJumpInstruction
            (true, endLocation, "Jump if true to end"));
        falseBlock->Emit(executable);
        executable.PopLocation();
    }
}

void ForStatement::Emit(Executable &executable) const
{
    int loopedVariableSymbol = 0;
    if (falseBlock != 0)
    {
        loopedVariableSymbol = executable.TemporarySymbol();

        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto falseExpression = new ConstantExpression
            (Token((SourceLocation &)*this, TOKEN_FALSE));
        AssignmentStatement initStatement
            (Token((SourceLocation &)*this, TOKEN_ASSIGN),
             loopedExpression, falseExpression);
        loopedExpression->Parent(&initStatement);
        falseExpression->Parent(&initStatement);
        initStatement.Parent(Parent());
        initStatement.Emit(executable);
    }

    iterableExpression->Emit(executable);
    executable.Insert(new StartIteratorInstruction);

    string targetName;
    int targetSymbol = 0;
    Expression *targetExpression = 0;
    if (variableList->NamesSize() == 1)
    {
        targetName = *variableList->NamesBegin();
        targetSymbol = executable.Symbol(targetName);
    }
    else
    {
        targetSymbol = executable.TemporarySymbol();
        variableList->Emit(executable);
        targetExpression = new VariableExpression
            ((SourceElement &)*this, targetSymbol);
        targetExpression->Parent(this);
        targetExpression->Emit
            (executable, Expression::EmitType::Address);
        executable.Insert(new SetInstruction(true));
    }

    auto testLocation = executable.Insert(new NullInstruction);
    continueLocation = executable.Insert(new NullInstruction);
    auto elseLocation = executable.Insert(new NullInstruction);
    endLocation = executable.Insert(new NullInstruction);

    executable.PushLocation(continueLocation);
    executable.Insert(new TestIteratorInstruction);
    executable.Insert(new ConditionalJumpInstruction
        (false, elseLocation, "Jump if false to else"));
    if (falseBlock != 0)
    {
        auto loopedExpression = new VariableExpression
            ((SourceElement &)*this, loopedVariableSymbol);
        auto trueExpression = new ConstantExpression
            (Token((SourceLocation &)*this, TOKEN_TRUE));
        AssignmentStatement signalStatement
            (Token((SourceLocation &)*this, TOKEN_ASSIGN),
             loopedExpression, trueExpression);
        loopedExpression->Parent(&signalStatement);
        trueExpression->Parent(&signalStatement);
        signalStatement.Parent(Parent());
        signalStatement.Emit(executable);
    }
    executable.Insert(new DereferenceIteratorInstruction
        ("Dereference iterator"));
    if (variableList->NamesSize() == 1)
    {
        ostringstream oss;
        oss
            << "Push address of variable " << targetName
            << " (" << targetSymbol << ')';
        executable.Insert(new LoadInstruction
            (targetSymbol, true, oss.str()));
    }
    else
        variableList->Emit(executable);
    executable.Insert(new SetInstruction(true));
    trueBlock->Emit(executable);
    executable.PopLocation();

    executable.PushLocation(elseLocation);
    executable.Insert(new AdvanceIteratorInstruction);
    executable.Insert(new JumpInstruction
        (testLocation, "Jump to test"));
    executable.PopLocation();

    executable.PushLocation(endLocation);
    if (falseBlock != 0)
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
        executable.Insert(new ConditionalJumpInstruction
            (true, endLocation, "Jump if true to end"));
        falseBlock->Emit(executable);
        executable.PopLocation();
    }
    executable.PopLocation();

    executable.Insert(new PopInstruction);
}

void Parameter::Emit(Executable &executable) const
{
    if (defaultExpression != 0)
        defaultExpression->Emit(executable);
    auto symbol = executable.Symbol(name);
    ostringstream oss;
    oss
        << "Make parameter " << name
        << " (" << symbol << ')';
    executable.Insert(new MakeParameterInstruction
        (symbol, defaultExpression != 0, oss.str()));
}

void ParameterList::Emit(Executable &executable) const
{
    executable.Insert(new PushParameterListInstruction
        ("Push empty parameter list"));
    for (auto iter = parameters.begin();
         iter != parameters.end(); iter++)
    {
        auto parameter = *iter;
        parameter->Emit(executable);
        executable.Insert(new BuildInstruction
            ("Add parameter to parameter list"));
    }
}

void DefStatement::Emit(Executable &executable) const
{
    auto entryLocation = executable.Insert(new NullInstruction);
    auto defineLocation = executable.Insert(new NullInstruction);

    executable.PushLocation(entryLocation);
    executable.Insert(new JumpInstruction
        (defineLocation, "Jump around code"));
    executable.PopLocation();

    executable.PushLocation(defineLocation);
    try
    {
        block->Emit(executable);

        // Emit a final return statement if needed.
        auto finalReturnStatement = dynamic_cast<const ReturnStatement *>
            (block->FinalStatement());
        if (finalReturnStatement == 0)
        {
            executable.Insert(new PushNoneInstruction
                ("Push default return value)"));
            executable.Insert(new ReturnInstruction);
        }
    }
    catch (...)
    {
        executable.PopLocation();
        throw;
    }
    executable.PopLocation();

    parameterList->Emit(executable);
    executable.Insert(new PushCodeAddressInstruction
        (entryLocation, "Push code address"));
    executable.Insert(new MakeFunctionInstruction);

    auto variableExpression = new VariableExpression
        (Token((SourceLocation &)*this, TOKEN_NAME, name));
    variableExpression->Parent(this);
    variableExpression->Emit
        (executable, Expression::EmitType::Address);
    delete variableExpression;
    executable.Insert(new SetInstruction(true));
}

void TernaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete value expression");

    falseExpression->Emit(executable);
    trueExpression->Emit(executable);
    conditionExpression->Emit(executable);
    uint8_t opCode = OpCode_COND;
    ostringstream oss;
    oss
        << "Perform ternary operation 0x"
        << hex << setfill('0') << setw(2)
        << static_cast<unsigned>(opCode);
    executable.Insert(new TernaryInstruction(opCode, oss.str()));
}

void BinaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete value expression");

    rightExpression->Emit(executable);
    leftExpression->Emit(executable);
    static map<int, uint8_t> opCodes =
    {
        {TOKEN_OR, OpCode_LOR},
        {TOKEN_AND, OpCode_LAND},
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
        {TOKEN_POWER, OpCode_POW},

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
        throw oss.str();
    }
    ostringstream oss;
    oss
        << "Perform binary operation 0x"
        << hex << setfill('0') << setw(2)
        << static_cast<unsigned>(iter->second);
    executable.Insert(new BinaryInstruction(iter->second, oss.str()));
}

void UnaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of value expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete value expression");

    expression->Emit(executable);
    static map<int, uint8_t> opCodes =
    {
        {TOKEN_NOT, OpCode_LNOT},
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
        throw oss.str();
    }
    ostringstream oss;
    oss
        << "Perform unary operation 0x"
        << hex << setfill('0') << setw(2)
        << static_cast<unsigned>(iter->second);
    executable.Insert(new UnaryInstruction(iter->second, oss.str()));
}

void Argument::Emit(Executable &executable) const
{
    valueExpression->Emit(executable);
    if (!name.empty())
    {
        auto symbol = executable.Symbol(name);
        ostringstream oss;
        oss
            << "Make argument with name " << name
            << " (" << symbol << ')';
        executable.Insert(new MakeArgumentInstruction(symbol, oss.str()));
    }
    else
        executable.Insert(new MakeArgumentInstruction
            ("Make positional argument"));
}

void ArgumentList::Emit(Executable &executable) const
{
    executable.Insert(new PushArgumentListInstruction
        ("Push empty argument list"));
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        auto argument = *iter;
        argument->Emit(executable);
        executable.Insert(new BuildInstruction
            ("Add argument to argument list"));
    }
}

void CallExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of function call");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete function call");

    argumentList->Emit(executable);
    functionExpression->Emit(executable);
    executable.Insert(new CallInstruction);
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
    executable.Insert(new IndexInstruction
        (emitType == EmitType::Address, oss.str()));
}

void MemberExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    expression->Emit(executable);

    if (emitType == EmitType::Delete)
        return;

    auto symbol = executable.Symbol(name);
    ostringstream oss;
    oss
        << "Lookup " << (emitType == EmitType::Address ? "address" : "value")
        << " of member";
    executable.Insert(new MemberInstruction
        (symbol, emitType == EmitType::Address, oss.str()));
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
        << " of variable " << name << " (" << symbol << ')';
    executable.Insert(new LoadInstruction
        (symbol, emitType == EmitType::Address, oss.str()));
}

void KeyValuePair::Emit(Executable &executable) const
{
    valueExpression->Emit(executable);
    keyExpression->Emit(executable);
    executable.Insert(new MakeKeyValuePairInstruction);
}

void DictionaryExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of dictionary expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete dictionary expression");

    executable.Insert(new PushDictionaryInstruction
        ("Create empty dictionary"));
    for (auto iter = entries.begin(); iter != entries.end(); iter++)
    {
        auto entry = *iter;
        entry->Emit(executable);
        executable.Insert(new BuildInstruction("Add entry to dictionary"));
    }
}

void SetExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of set expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete set expression");

    executable.Insert(new PushSetInstruction
        ("Create empty set"));
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable);
        executable.Insert(new BuildInstruction("Add item to set"));
    }
}

void ListExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of list expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete list expression");

    executable.Insert(new PushListInstruction
        ("Create empty list"));
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable);
        executable.Insert(new BuildInstruction("Add item to list"));
    }
}

void TupleExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Delete)
        throw string("Cannot delete tuple expression");

    executable.Insert(new PushTupleInstruction
        ("Create empty tuple"));
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Emit(executable, emitType);
        executable.Insert(new BuildInstruction("Add item to tuple"));
    }
}

void RangeExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of range expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete range expression");

    list<Expression *> partExpressions =
    {
        startExpression, endExpression, stepExpression
    };
    unsigned partCount = 0;
    for (auto iter = partExpressions.rbegin();
         iter != partExpressions.rend(); iter++)
    {
        auto partExpression = *iter;
        if (partExpression != 0)
        {
            partExpression->Emit(executable);
            partCount++;
        }
    }
    ostringstream oss;
    oss
        << "Make range of pattern "
        << (startExpression ? "S" : "") << ".."
        << (endExpression ? "E" : "") << ':'
        << (stepExpression ? "T" : "");
    executable.Insert(new MakeRangeInstruction
        (startExpression != 0, endExpression != 0, stepExpression != 0,
         oss.str()));
}

void ConstantExpression::Emit
    (Executable &executable, EmitType emitType) const
{
    if (emitType == EmitType::Address)
        throw string("Cannot take address of constant expression");
    else if (emitType == EmitType::Delete)
        throw string("Cannot delete constant expression");

    switch (type)
    {
        case Type::None:
            executable.Insert(new PushNoneInstruction);
            break;
        case Type::Ellipsis:
            executable.Insert(new PushEllipsisInstruction);
            break;
        case Type::Boolean:
            executable.Insert(new PushBooleanInstruction(b));
            break;
        case Type::Integer:
            executable.Insert(new PushIntegerInstruction(i));
            break;
        case Type::Float:
            executable.Insert(new PushFloatInstruction(f));
            break;
        case Type::String:
            executable.Insert(new PushStringInstruction(s));
            break;
    }
}
