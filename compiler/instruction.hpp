//
// Asp instruction definitions.
//

#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "executable.hpp"
#include <iostream>
#include <string>
#include <cstdint>

class Instruction
{
    protected:

        // Base constructors.
        explicit Instruction
            (std::uint8_t opCode, const std::string &comment = "");
        Instruction
            (std::uint8_t opCode, const Executable::Location &,
             const std::string &comment = "");

    public:

        // Destructor.
        virtual ~Instruction();

        // Address methods.
        void Offset(std::uint32_t);
        std::uint32_t Offset() const;
        Executable::Location TargetLocation() const;
        bool Fixed() const;
        void Fix(std::uint32_t targetOffset);

        // Code generation methods.
        virtual unsigned Size() const;
        virtual void Write(std::ostream &) const;

        // Listing methods.
        virtual void Print(std::ostream &) const;

    protected:

        // Internal methods.
        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const = 0;
        static unsigned OperandSize(std::uint32_t value);
        static unsigned OperandSize(std::int32_t value);
        static void WriteField
            (std::ostream &, std::uint64_t value, unsigned size);
        std::uint8_t OpCode() const;

    private:

        // Data.
        std::uint8_t opCode;
        std::uint32_t offset, targetOffset;
        std::string comment;
        bool targetLocationDefined, fixed;
        Executable::Location targetLocation;
};

class NullInstruction : public Instruction
{
    public:

        NullInstruction();

        virtual unsigned Size() const;
        virtual void Write(std::ostream &) const;
        virtual void Print(std::ostream &) const;

    protected:

        virtual void PrintCode(std::ostream &) const;
};

class SimpleInstruction : public Instruction
{
    public:

        explicit SimpleInstruction
            (std::uint8_t opCode, const std::string &comment = "");
        SimpleInstruction
            (std::uint8_t opCode, const Executable::Location &,
             const std::string &comment = "");

    protected:

        virtual void PrintCode(std::ostream &) const;
};

class PushNoneInstruction : public SimpleInstruction
{
    public:

        explicit PushNoneInstruction
            (const std::string &comment = "");
};

class PushEllipsisInstruction : public SimpleInstruction
{
    public:

        explicit PushEllipsisInstruction
            (const std::string &comment = "");
};

class PushBooleanInstruction : public SimpleInstruction
{
    public:

        explicit PushBooleanInstruction
            (bool, const std::string &comment = "");
};

class PushIntegerInstruction : public Instruction
{
    public:

        explicit PushIntegerInstruction
            (std::int32_t, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t value;
};

class PushFloatInstruction : public Instruction
{
    public:

        explicit PushFloatInstruction
            (double, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        double value;
};

class PushStringInstruction : public Instruction
{
    public:

        explicit PushStringInstruction
            (const std::string &s, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::string s;
};

class PushTupleInstruction : public SimpleInstruction
{
    public:

        explicit PushTupleInstruction
            (const std::string &comment = "");
};

class PushListInstruction : public SimpleInstruction
{
    public:

        explicit PushListInstruction
            (const std::string &comment = "");
};

class PushSetInstruction : public SimpleInstruction
{
    public:

        explicit PushSetInstruction
            (const std::string &comment = "");
};

class PushDictionaryInstruction : public SimpleInstruction
{
    public:

        explicit PushDictionaryInstruction
            (const std::string &comment = "");
};

class PushArgumentListInstruction : public SimpleInstruction
{
    public:

        explicit PushArgumentListInstruction
            (const std::string &comment = "");
};

class PushParameterListInstruction : public SimpleInstruction
{
    public:

        explicit PushParameterListInstruction
            (const std::string &comment = "");
};

class PushCodeAddressInstruction : public SimpleInstruction
{
    public:

        explicit PushCodeAddressInstruction
            (const Executable::Location &, const std::string &comment = "");
};

class PushModuleInstruction : public Instruction
{
    public:

        PushModuleInstruction
            (std::int32_t symbol, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class PopInstruction : public SimpleInstruction
{
    public:

        PopInstruction(const std::string &comment = "");
};

class UnaryInstruction : public SimpleInstruction
{
    public:

        UnaryInstruction
            (std::uint8_t opCode, const std::string &comment = "");
};

class BinaryInstruction : public SimpleInstruction
{
    public:

        BinaryInstruction
            (std::uint8_t opCode, const std::string &comment = "");
};

class LogicalInstruction : public SimpleInstruction
{
    public:

        LogicalInstruction
            (std::uint8_t opCode, const Executable::Location &,
             const std::string &comment = "");
};

class LoadInstruction : public Instruction
{
    public:

        LoadInstruction
            (std::int32_t symbol, bool address,
             const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class SetInstruction : public SimpleInstruction
{
    public:

        explicit SetInstruction
            (bool pop, const std::string &comment = "");
};

class DeleteInstruction : public Instruction
{
    public:

        explicit DeleteInstruction
            (std::int32_t symbol, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class EraseInstruction : public SimpleInstruction
{
    public:

        explicit EraseInstruction
            (const std::string &comment = "");
};

class GlobalInstruction : public Instruction
{
    public:

        explicit GlobalInstruction
            (std::int32_t symbol, bool local,
             const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};


class StartIteratorInstruction : public SimpleInstruction
{
    public:

        explicit StartIteratorInstruction
            (const std::string &comment = "");
};

class TestIteratorInstruction : public SimpleInstruction
{
    public:

        explicit TestIteratorInstruction
            (const std::string &comment = "");
};

class AdvanceIteratorInstruction : public SimpleInstruction
{
    public:

        explicit AdvanceIteratorInstruction
            (const std::string &comment = "");
};


class DereferenceIteratorInstruction : public SimpleInstruction
{
    public:

        explicit DereferenceIteratorInstruction
            (const std::string &comment = "");
};

class ConditionalJumpInstruction : public SimpleInstruction
{
    public:

        explicit ConditionalJumpInstruction
            (bool condition, const Executable::Location &,
             const std::string &comment = "");
};


class JumpInstruction : public SimpleInstruction
{
    public:

        explicit JumpInstruction
            (const Executable::Location &,
             const std::string &comment = "");
};

class CallInstruction : public SimpleInstruction
{
    public:

        explicit CallInstruction
            (const std::string &comment = "");
};

class ReturnInstruction : public SimpleInstruction
{
    public:

        explicit ReturnInstruction
            (const std::string &comment = "");
};

class AddModuleInstruction : public Instruction
{
    public:

        AddModuleInstruction
            (std::int32_t symbol, const Executable::Location &,
             const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class ExitModuleInstruction : public SimpleInstruction
{
    public:

        explicit ExitModuleInstruction
            (const std::string &comment = "");
};

class LoadModuleInstruction : public Instruction
{
    public:

        explicit LoadModuleInstruction
            (std::int32_t symbol, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class MakeArgumentInstruction : public Instruction
{
    public:

        explicit MakeArgumentInstruction
            (bool isGroup, const std::string &comment = "");
        explicit MakeArgumentInstruction
            (std::int32_t symbol, const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class MakeParameterInstruction : public Instruction
{
    public:

        MakeParameterInstruction
            (std::int32_t symbol, bool withDefault, bool isGroup,
             const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class MakeFunctionInstruction : public SimpleInstruction
{
    public:

        explicit MakeFunctionInstruction
            (const std::string &comment = "");
};

class MakeKeyValuePairInstruction : public SimpleInstruction
{
    public:

        explicit MakeKeyValuePairInstruction
            (const std::string &comment = "");
};

class MakeRangeInstruction : public SimpleInstruction
{
    public:

        MakeRangeInstruction
            (bool hasStart, bool hasEnd, bool hasStep,
             const std::string &comment = "");
};

class InsertInstruction : public SimpleInstruction
{
    public:

        explicit InsertInstruction
            (bool pop, const std::string &comment = "");
};

class BuildInstruction : public SimpleInstruction
{
    public:

        explicit BuildInstruction
            (const std::string &comment = "");
};

class IndexInstruction : public SimpleInstruction
{
    public:

        IndexInstruction
            (bool address, const std::string &comment = "");
};

class MemberInstruction : public Instruction
{
    public:

        MemberInstruction
            (std::int32_t symbol, bool address,
             const std::string &comment = "");

    protected:

        virtual unsigned OperandsSize() const;
        virtual void WriteOperands(std::ostream &) const;
        virtual void PrintCode(std::ostream &) const;

    private:

        std::int32_t symbol;
};

class AbortInstruction : public SimpleInstruction
{
    public:

        explicit AbortInstruction
            (const std::string &comment = "");
};

class EndInstruction : public SimpleInstruction
{
    public:

        explicit EndInstruction
            (const std::string &comment = "");
};

#endif
