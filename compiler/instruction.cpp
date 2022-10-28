//
// Asp instruction implementation.
//

#include "instruction.hpp"
#include "opcode.h"
#include <map>
#include <algorithm>
#include <iomanip>

using namespace std;

static inline char Byte(uint64_t value, unsigned index)
{
    return (value >> (index << 3)) & 0xFF;
}

Instruction::Instruction(uint8_t opCode, const string &comment) :
    offset(0), targetOffset(0),
    opCode(opCode),
    comment(comment),
    targetLocationDefined(false),
    fixed(true)
{
}

Instruction::Instruction
    (uint8_t opCode, const Executable::Location &targetLocation,
     const string &comment) :
    offset(0), targetOffset(0),
    opCode(opCode),
    comment(comment),
    targetLocationDefined(true), fixed(false),
    targetLocation(targetLocation)
{
}

Instruction::~Instruction()
{
}

void Instruction::Offset(uint32_t offset)
{
    this->offset = offset;
}

uint32_t Instruction::Offset() const
{
    return offset;
}

Executable::Location Instruction::TargetLocation() const
{
    return targetLocation;
}

bool Instruction::Fixed() const
{
    return fixed;
}

void Instruction::Fix(uint32_t targetOffset)
{
    this->targetOffset = targetOffset;
    fixed = true;
}

unsigned Instruction::Size() const
{
    return
        sizeof opCode + OperandsSize() +
        (targetLocationDefined ? sizeof targetOffset : 0);
}

void Instruction::Write(ostream &os) const
{
    os.write(reinterpret_cast<const char *>(&opCode), 1);
    WriteOperands(os);
    if (targetLocationDefined)
        WriteField(os, targetOffset, 4);
}

void Instruction::Print(ostream &os) const
{
    auto oldFlags = os.flags();
    auto oldFill = os.fill();
    os << "0x" << hex << setfill('0') << setw(7) << offset << ": ";
    os.flags(oldFlags);
    os.fill(oldFill);

    PrintCode(os);

    if (targetLocationDefined)
    {
        oldFlags = os.flags();
        oldFill = os.fill();
        os << " 0x" << hex << setfill('0') << setw(7) << targetOffset;
        os.flags(oldFlags);
        os.fill(oldFill);
    }

    if (!comment.empty())
        os << "; " << comment;
}

unsigned Instruction::OperandsSize() const
{
    return 0;
}

void Instruction::WriteOperands(ostream &os) const
{
}

unsigned Instruction::OperandSize(uint32_t value)
{
    return
        value == 0 ? 0 :
        value <= 0xFF ? 1 :
        value <= 0xFFFF ? 2 : 4;
}

unsigned Instruction::OperandSize(int32_t value)
{
    return
        value == 0 ? 0 :
        value >= -128 && value <= 127 ? 1 :
        value >= -32768 && value <= 32767 ? 2 : 4;
}

void Instruction::WriteField(ostream &os, uint64_t value, unsigned size)
{
    unsigned i = size;
    while (i--)
        os << Byte(value, i);
}

uint8_t Instruction::OpCode() const
{
    return opCode;
}

NullInstruction::NullInstruction() :
    Instruction(0)
{
}

unsigned NullInstruction::Size() const
{
    return 0;
}

void NullInstruction::Write(ostream &) const
{
}

void NullInstruction::Print(ostream &) const
{
}

void NullInstruction::PrintCode(ostream &) const
{
}

SimpleInstruction::SimpleInstruction
    (uint8_t opCode, const string &comment) :
    Instruction(opCode, comment)
{
}

SimpleInstruction::SimpleInstruction
    (uint8_t opCode, const Executable::Location &location,
     const string &comment) :
    Instruction(opCode, location, comment)
{
}

void SimpleInstruction::PrintCode(ostream &os) const
{
    static map<uint8_t, string> mnemonics =
    {
        {OpCode_PUSHN, "PUSHN"},
        {OpCode_PUSHE, "PUSHE"},
        {OpCode_PUSHF, "PUSHF"},
        {OpCode_PUSHT, "PUSHT"},
        {OpCode_PUSHTU, "PUSHTU"},
        {OpCode_PUSHLI, "PUSHLI"},
        {OpCode_PUSHSE, "PUSHSE"},
        {OpCode_PUSHDI, "PUSHDI"},
        {OpCode_PUSHAL, "PUSHAL"},
        {OpCode_PUSHPL, "PUSHPL"},
        {OpCode_PUSHCA, "PUSHCA"},
        {OpCode_POP, "POP"},
        {OpCode_LNOT, "LNOT"},
        {OpCode_NEG, "NEG"},
        {OpCode_NOT, "NOT"},
        {OpCode_OR, "OR"},
        {OpCode_XOR, "XOR"},
        {OpCode_AND, "AND"},
        {OpCode_LSH, "LSH"},
        {OpCode_RSH, "RSH"},
        {OpCode_ADD, "ADD"},
        {OpCode_SUB, "SUB"},
        {OpCode_MUL, "MUL"},
        {OpCode_DIV, "DIV"},
        {OpCode_FDIV, "FDIV"},
        {OpCode_MOD, "MOD"},
        {OpCode_POW, "POW"},
        {OpCode_NE, "NE"},
        {OpCode_EQ, "EQ"},
        {OpCode_LT, "LT"},
        {OpCode_LE, "LE"},
        {OpCode_GT, "GT"},
        {OpCode_GE, "GE"},
        {OpCode_NIN, "NIN"},
        {OpCode_IN, "IN"},
        {OpCode_NIS, "NIS"},
        {OpCode_IS, "IS"},
        {OpCode_SET, "SET"},
        {OpCode_SETP, "SETP"},
        {OpCode_ERASE, "ERASE"},
        {OpCode_SITER, "SITER"},
        {OpCode_TITER, "TITER"},
        {OpCode_NITER, "NITER"},
        {OpCode_DITER, "DITER"},
        {OpCode_NOOP, "NOOP"},
        {OpCode_JMPF, "JMPF"},
        {OpCode_JMPT, "JMPT"},
        {OpCode_JMP, "JMP"},
        {OpCode_LOR, "LOR"},
        {OpCode_LAND, "LAND"},
        {OpCode_CALL, "CALL"},
        {OpCode_RET, "RET"},
        {OpCode_XMOD, "XMOD"},
        {OpCode_MKFUN, "MKFUN"},
        {OpCode_MKKVP, "MKKVP"},
        {OpCode_MKR0, "MKR0"},
        {OpCode_MKRS, "MKRS"},
        {OpCode_MKRE, "MKRE"},
        {OpCode_MKRSE, "MKRSE"},
        {OpCode_MKRT, "MKRT"},
        {OpCode_MKRST, "MKRST"},
        {OpCode_MKRET, "MKRET"},
        {OpCode_MKR, "MKR"},
        {OpCode_INS, "INS"},
        {OpCode_INSP, "INSP"},
        {OpCode_BLD, "BLD"},
        {OpCode_IDX, "IDX"},
        {OpCode_IDXA, "IDXA"},
        {OpCode_END, "END"},
    };
    auto iter = mnemonics.find(OpCode());
    os << (iter != mnemonics.end() ? iter->second : "???");
}

PushNoneInstruction::PushNoneInstruction(const string &comment) :
    SimpleInstruction(OpCode_PUSHN, comment)
{
}

PushEllipsisInstruction::PushEllipsisInstruction(const string &comment) :
    SimpleInstruction(OpCode_PUSHE, comment)
{
}

PushBooleanInstruction::PushBooleanInstruction
    (bool value, const string &comment) :
    SimpleInstruction(value ? OpCode_PUSHT : OpCode_PUSHF, comment)
{
}

PushIntegerInstruction::PushIntegerInstruction
    (int32_t value, const string &comment) :
    Instruction
        (OperandSize(value) == 0 ? OpCode_PUSHI0 :
         OperandSize(value) == 1 ? OpCode_PUSHI1 :
         OperandSize(value) == 2 ? OpCode_PUSHI2 : OpCode_PUSHI4,
         comment),
    value(value)
{
}

unsigned PushIntegerInstruction::OperandsSize() const
{
    return OperandSize(value);
}

void PushIntegerInstruction::WriteOperands(ostream &os) const
{
    uint32_t uValue = *reinterpret_cast<const uint32_t *>(&value);
    WriteField(os, uValue, OperandSize(value));
}

void PushIntegerInstruction::PrintCode(ostream &os) const
{
    os << "PUSHI " << value;
}

PushFloatInstruction::PushFloatInstruction
    (double value, const string &comment) :
    Instruction(OpCode_PUSHD, comment),
    value(value)
{
}

unsigned PushFloatInstruction::OperandsSize() const
{
    return 8;
}

void PushFloatInstruction::WriteOperands(ostream &os) const
{
    uint64_t uValue = *reinterpret_cast<const uint64_t *>(&value);
    WriteField(os, uValue, 8);
}

void PushFloatInstruction::PrintCode(ostream &os) const
{
    os << "PUSHD " << value;
}

PushStringInstruction::PushStringInstruction
    (const string &s, const string &comment) :
    Instruction
        (OperandSize(static_cast<uint32_t>(s.size())) == 0 ? OpCode_PUSHS0 :
         OperandSize(static_cast<uint32_t>(s.size())) == 1 ? OpCode_PUSHS1 :
         OperandSize(static_cast<uint32_t>(s.size())) == 2 ?
         OpCode_PUSHS2 : OpCode_PUSHS4,
         comment),
    s(s)
{
}

unsigned PushStringInstruction::OperandsSize() const
{
    return
        OperandSize(static_cast<uint32_t>(s.size())) +
        static_cast<unsigned>(s.size());
}

void PushStringInstruction::WriteOperands(ostream &os) const
{
    WriteField
        (os,
         static_cast<uint32_t>(s.size()),
         OperandSize(static_cast<uint32_t>(s.size())));
    os.write(s.c_str(), s.size());
}

void PushStringInstruction::PrintCode(ostream &os) const
{
    os << "PUSHS " << s.size() << ", '" << s << '\'';
}

PushTupleInstruction::PushTupleInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHTU, comment)
{
}

PushListInstruction::PushListInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHLI, comment)
{
}

PushSetInstruction::PushSetInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHSE, comment)
{
}

PushDictionaryInstruction::PushDictionaryInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHDI, comment)
{
}

PushArgumentListInstruction::PushArgumentListInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHAL, comment)
{
}

PushParameterListInstruction::PushParameterListInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_PUSHPL, comment)
{
}

PushCodeAddressInstruction::PushCodeAddressInstruction
    (const Executable::Location &location, const string &comment) :
    SimpleInstruction(OpCode_PUSHCA, location, comment)
{
}

PushModuleInstruction::PushModuleInstruction
    (int32_t symbol, const string &comment) :
    Instruction
        (OperandSize(symbol) <= 1 ? OpCode_PUSHM1 :
         OperandSize(symbol) == 2 ? OpCode_PUSHM2 : OpCode_PUSHM4,
         comment),
    symbol(symbol)
{
}

unsigned PushModuleInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void PushModuleInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void PushModuleInstruction::PrintCode(ostream &os) const
{
    os << "PUSHM " << symbol;
}

PopInstruction::PopInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_POP, comment)
{
}

UnaryInstruction::UnaryInstruction
    (uint8_t opCode, const string &comment) :
    SimpleInstruction(opCode, comment)
{
}

BinaryInstruction::BinaryInstruction
    (uint8_t opCode, const string &comment) :
    SimpleInstruction(opCode, comment)
{
}

LogicalInstruction::LogicalInstruction
    (uint8_t opCode, const Executable::Location &location,
     const string &comment) :
    SimpleInstruction(opCode, location, comment)
{
}

LoadInstruction::LoadInstruction
    (int32_t symbol, bool address, const string &comment) :
    Instruction
        (address ?
            (OperandSize(symbol) <= 1 ? OpCode_LDA1 :
             OperandSize(symbol) == 2 ? OpCode_LDA2 : OpCode_LDA4) :
            (OperandSize(symbol) <= 1 ? OpCode_LD1 :
             OperandSize(symbol) == 2 ? OpCode_LD2 : OpCode_LD4),
         comment),
    symbol(symbol)
{
}

unsigned LoadInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void LoadInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void LoadInstruction::PrintCode(ostream &os) const
{
    bool address =
        OpCode() == OpCode_LDA1 ||
        OpCode() == OpCode_LDA2 ||
        OpCode() == OpCode_LDA4;
    os << (address ? "LDA" : "LD") << ' ' << symbol;
}

SetInstruction::SetInstruction(bool pop, const string &comment) :
    SimpleInstruction(pop ? OpCode_SETP : OpCode_SET, comment)
{
}

DeleteInstruction::DeleteInstruction
    (int32_t symbol, const string &comment) :
    Instruction
        (OperandSize(symbol) <= 1 ? OpCode_DEL1 :
         OperandSize(symbol) == 2 ? OpCode_DEL2 : OpCode_DEL4,
         comment),
    symbol(symbol)
{
}

unsigned DeleteInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void DeleteInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void DeleteInstruction::PrintCode(ostream &os) const
{
    os << "DEL " << symbol;
}

EraseInstruction::EraseInstruction(const string &comment) :
    SimpleInstruction(OpCode_ERASE, comment)
{
}

GlobalInstruction::GlobalInstruction
    (int32_t symbol, bool local, const string &comment) :
    Instruction
        (local ?
            (OperandSize(symbol) <= 1 ? OpCode_LOC1 :
             OperandSize(symbol) == 2 ? OpCode_LOC2 : OpCode_LOC4) :
            (OperandSize(symbol) <= 1 ? OpCode_GLOB1 :
             OperandSize(symbol) == 2 ? OpCode_GLOB2 : OpCode_GLOB4),
         comment),
    symbol(symbol)
{
}

unsigned GlobalInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void GlobalInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void GlobalInstruction::PrintCode(ostream &os) const
{
    bool local =
        OpCode() == OpCode_LOC1 ||
        OpCode() == OpCode_LOC2 ||
        OpCode() == OpCode_LOC4;
    os << (local ? "LOC" : "GLOB") << ' ' << symbol;
}

StartIteratorInstruction::StartIteratorInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_SITER, comment)
{
}

TestIteratorInstruction::TestIteratorInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_TITER, comment)
{
}

AdvanceIteratorInstruction::AdvanceIteratorInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_NITER, comment)
{
}

DereferenceIteratorInstruction::DereferenceIteratorInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_DITER, comment)
{
}

ConditionalJumpInstruction::ConditionalJumpInstruction
    (bool condition, const Executable::Location &targetLocation,
     const string &comment) :
    SimpleInstruction
        (condition ? OpCode_JMPT : OpCode_JMPF,
         targetLocation, comment)
{
}

JumpInstruction::JumpInstruction
    (const Executable::Location &targetLocation,
     const string &comment) :
    SimpleInstruction(OpCode_JMP, targetLocation, comment)
{
}

CallInstruction::CallInstruction(const string &comment) :
    SimpleInstruction(OpCode_CALL, comment)
{
}

ReturnInstruction::ReturnInstruction(const string &comment) :
    SimpleInstruction(OpCode_RET, comment)
{
}

AddModuleInstruction::AddModuleInstruction
    (int32_t symbol, const Executable::Location &targetLocation,
     const string &comment) :
    Instruction
        (OperandSize(symbol) <= 1 ? OpCode_ADDMOD1 :
         OperandSize(symbol) == 2 ? OpCode_ADDMOD2 : OpCode_ADDMOD4,
         targetLocation,
         comment),
    symbol(symbol)
{
}

unsigned AddModuleInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void AddModuleInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void AddModuleInstruction::PrintCode(ostream &os) const
{
    os << "ADDMOD " << symbol;
}

ExitModuleInstruction::ExitModuleInstruction(const string &comment) :
    SimpleInstruction(OpCode_XMOD, comment)
{
}

LoadModuleInstruction::LoadModuleInstruction
    (int32_t symbol, const string &comment) :
    Instruction
        (OperandSize(symbol) <= 1 ? OpCode_LDMOD1 :
         OperandSize(symbol) == 2 ? OpCode_LDMOD2 : OpCode_LDMOD4,
         comment),
    symbol(symbol)
{
}

unsigned LoadModuleInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void LoadModuleInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void LoadModuleInstruction::PrintCode(ostream &os) const
{
    os << "LDMOD " << symbol;
}

MakeArgumentInstruction::MakeArgumentInstruction
    (bool isGroup, const string &comment) :
    Instruction(isGroup ? OpCode_MKGARG : OpCode_MKARG, comment),
    symbol(0)
{
}

MakeArgumentInstruction::MakeArgumentInstruction
    (int32_t symbol, const string &comment) :
    Instruction
        (OperandSize(symbol) <= 1 ? OpCode_MKNARG1 :
         OperandSize(symbol) == 2 ? OpCode_MKNARG2 : OpCode_MKNARG4,
         comment),
    symbol(symbol)
{
}

unsigned MakeArgumentInstruction::OperandsSize() const
{
    return
        OpCode() == OpCode_MKARG || OpCode() == OpCode_MKGARG ?
        0 : max(1U, OperandSize(symbol));
}

void MakeArgumentInstruction::WriteOperands(ostream &os) const
{
    if (OpCode() != OpCode_MKARG && OpCode() != OpCode_MKGARG)
    {
        uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
        WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
    }
}

void MakeArgumentInstruction::PrintCode(ostream &os) const
{
    os << "MK";
    if (OpCode() == OpCode_MKGARG)
        os << 'G';
    else if (OpCode() != OpCode_MKARG)
        os << 'N';
    os << "ARG";
    if (OpCode() != OpCode_MKARG && OpCode() != OpCode_MKGARG)
        os << ' ' << symbol;
}

MakeParameterInstruction::MakeParameterInstruction
    (int32_t symbol, bool withDefault, bool isGroup, const string &comment) :
    Instruction
        (withDefault ?
            (OperandSize(symbol) <= 1 ? OpCode_MKDPAR1 :
             OperandSize(symbol) == 2 ? OpCode_MKDPAR2 : OpCode_MKDPAR4) :
             isGroup ?
                (OperandSize(symbol) <= 1 ? OpCode_MKGPAR1 :
                 OperandSize(symbol) == 2 ? OpCode_MKGPAR2 : OpCode_MKGPAR4) :
                (OperandSize(symbol) <= 1 ? OpCode_MKPAR1 :
                 OperandSize(symbol) == 2 ? OpCode_MKPAR2 : OpCode_MKPAR4),
         comment),
    symbol(symbol)
{
}

unsigned MakeParameterInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void MakeParameterInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void MakeParameterInstruction::PrintCode(ostream &os) const
{
    os << "MK";
    if (OpCode() == OpCode_MKGPAR1 ||
        OpCode() == OpCode_MKGPAR2 ||
        OpCode() == OpCode_MKGPAR4)
        os << 'G';
    if (OpCode() == OpCode_MKDPAR1 ||
        OpCode() == OpCode_MKDPAR2 ||
        OpCode() == OpCode_MKDPAR4)
        os << 'D';
    os << "PAR " << symbol;
}

MakeFunctionInstruction::MakeFunctionInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_MKFUN, comment)
{
}

MakeKeyValuePairInstruction::MakeKeyValuePairInstruction
    (const string &comment) :
    SimpleInstruction(OpCode_MKKVP, comment)
{
}

InsertInstruction::InsertInstruction(bool pop, const string &comment) :
    SimpleInstruction(pop ? OpCode_INSP : OpCode_INS, comment)
{
}

BuildInstruction::BuildInstruction(const string &comment) :
    SimpleInstruction(OpCode_BLD, comment)
{
}

MakeRangeInstruction::MakeRangeInstruction
    (bool hasStart, bool hasEnd, bool hasStep,
     const string &comment) :
    SimpleInstruction
        (hasStart ? hasEnd ? hasStep ? OpCode_MKR : OpCode_MKRSE :
         hasStep ? OpCode_MKRST : OpCode_MKRS :
         hasEnd ? hasStep ? OpCode_MKRET : OpCode_MKRE :
         hasStep ? OpCode_MKRT : OpCode_MKR0,
         comment)
{
}

IndexInstruction::IndexInstruction
    (bool address, const string &comment) :
    SimpleInstruction(address ? OpCode_IDXA : OpCode_IDX, comment)
{
}

MemberInstruction::MemberInstruction
    (int32_t symbol, bool address, const string &comment) :
    Instruction
        (address ?
            (OperandSize(symbol) <= 1 ? OpCode_MEMA1 :
             OperandSize(symbol) == 2 ? OpCode_MEMA2 : OpCode_MEMA4) :
            (OperandSize(symbol) <= 1 ? OpCode_MEM1 :
             OperandSize(symbol) == 2 ? OpCode_MEM2 : OpCode_MEM4),
         comment),
    symbol(symbol)
{
}

unsigned MemberInstruction::OperandsSize() const
{
    return max(1U, OperandSize(symbol));
}

void MemberInstruction::WriteOperands(ostream &os) const
{
    uint32_t uSymbol = *reinterpret_cast<const uint32_t *>(&symbol);
    WriteField(os, uSymbol, max(1U, OperandSize(symbol)));
}

void MemberInstruction::PrintCode(ostream &os) const
{
    bool address =
        OpCode() == OpCode_MEMA1 ||
        OpCode() == OpCode_MEMA2 ||
        OpCode() == OpCode_MEMA4;
    os << (address ? "MEMA" : "MEM") << ' ' << symbol;
}

EndInstruction::EndInstruction(const string &comment) :
    SimpleInstruction(OpCode_END, comment)
{
}
