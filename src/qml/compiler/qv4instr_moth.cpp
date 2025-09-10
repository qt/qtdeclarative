// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4instr_moth_p.h"
#include <private/qv4compileddata_p.h>
#include <private/qv4calldata_p.h>

#include <QtCore/qdebug.h>

using namespace QV4;
using namespace QV4::Moth;

int InstrInfo::size(Instr::Type type)
{
#define MOTH_RETURN_INSTR_SIZE(I) case Instr::Type::I: case Instr::Type::I##_Wide: return InstrMeta<int(Instr::Type::I)>::Size;
    switch (type) {
    FOR_EACH_MOTH_INSTR_ALL(MOTH_RETURN_INSTR_SIZE)
    }
#undef MOTH_RETURN_INSTR_SIZE
    Q_UNREACHABLE();
}

static QByteArray alignedNumber(int n) {
    QByteArray number = QByteArray::number(n);
    return number.prepend(8 - number.size(), ' ');
}

static QByteArray alignedLineNumber(int line) {
    if (line > 0)
        return alignedNumber(static_cast<int>(line));
    return QByteArray("        ");
}

static QByteArray rawBytes(const char *data, int n)
{
    QByteArray ba;
    while (n) {
        uint num = *reinterpret_cast<const uchar *>(data);
        if (num < 16)
            ba += '0';
        ba += QByteArray::number(num, 16) + " ";
        ++data;
        --n;
    }
    while (ba.size() < 25)
        ba += ' ';
    return ba;
}

#define ABSOLUTE_OFFSET() \
    (code + beginOffset - start + offset)

#define MOTH_BEGIN_INSTR(instr) \
    { \
        INSTR_##instr(MOTH_DECODE_WITH_BASE) \
        if (static_cast<int>(Instr::Type::instr) >= 0x100) \
            --base_ptr; \
        s << alignedLineNumber(line) << alignedNumber(beginOffset + codeOffset).constData() << ": " \
          << rawBytes(base_ptr, int(code - base_ptr)) << #instr << " ";

#define MOTH_END_INSTR(instr) \
        s << "\n"; \
        continue; \
    }

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace Moth {

const int InstrInfo::argumentCount[] = {
    FOR_EACH_MOTH_INSTR_ALL(MOTH_COLLECT_NARGS)
};

QString dumpRegister(int reg, int nFormals)
{
    Q_STATIC_ASSERT(offsetof(CallData, function) == 0);
    Q_STATIC_ASSERT(offsetof(CallData, context) == sizeof(StaticValue));
    Q_STATIC_ASSERT(offsetof(CallData, accumulator) == 2*sizeof(StaticValue));
    Q_STATIC_ASSERT(offsetof(CallData, thisObject) == 3*sizeof(StaticValue));
    if (reg == CallData::Function)
        return QStringLiteral("(function)");
    else if (reg == CallData::Context)
        return QStringLiteral("(context)");
    else if (reg == CallData::Accumulator)
        return QStringLiteral("(accumulator)");
    else if (reg == CallData::NewTarget)
        return QStringLiteral("(new.target)");
    else if (reg == CallData::This)
        return QStringLiteral("(this)");
    else if (reg == CallData::Argc)
        return QStringLiteral("(argc)");
    reg -= CallData::HeaderSize();
    if (reg < nFormals)
        return QStringLiteral("a%1").arg(reg);
    reg -= nFormals;
    return QStringLiteral("r%1").arg(reg);

}

QString dumpArguments(int argc, int argv, int nFormals)
{
    if (!argc)
        return QStringLiteral("()");
    return QStringLiteral("(") + dumpRegister(argv, nFormals) + QStringLiteral(", ") + QString::number(argc) + QStringLiteral(")");
}

QString dumpBytecode(
        const char *code, int len, int nLocals, int nFormals, int /*startLine*/,
        const QVector<CompiledData::CodeOffsetToLineAndStatement> &lineAndStatementNumberMapping)
{
    return dumpBytecode(code, len, nLocals, nFormals, 0, len - 1, lineAndStatementNumberMapping);
}

QString dumpBytecode(
        const char *code, int len, int nLocals, int nFormals, int beginOffset, int endOffset,
        const QVector<CompiledData::CodeOffsetToLineAndStatement> &lineAndStatementNumberMapping)
{
    Q_ASSERT(beginOffset <= endOffset && 0 <= beginOffset && endOffset <= len);

    MOTH_JUMP_TABLE;

    auto findLine = [](const CompiledData::CodeOffsetToLineAndStatement &entry, uint offset) {
        return entry.codeOffset < offset;
    };

    QString output;
    QTextStream s{ &output };

    int lastLine = -1;
    code += beginOffset;
    const char *start = code;
    const char *end = code + (endOffset - beginOffset) + 1;
    while (code < end) {
        const auto codeToLine = std::lower_bound(
                    lineAndStatementNumberMapping.constBegin(),
                    lineAndStatementNumberMapping.constEnd(),
                    static_cast<uint>(code - start + beginOffset) + 1, findLine) - 1;
        int line = int(codeToLine->line);
        if (line != lastLine)
            lastLine = line;
        else
            line = -1;

        int codeOffset = int(code - start);

        MOTH_DISPATCH()

        MOTH_BEGIN_INSTR(LoadReg)
            s << dumpRegister(reg, nFormals);
        MOTH_END_INSTR(LoadReg)

        MOTH_BEGIN_INSTR(StoreReg)
            s << dumpRegister(reg, nFormals);
        MOTH_END_INSTR(StoreReg)

        MOTH_BEGIN_INSTR(MoveReg)
            s << dumpRegister(srcReg, nFormals) << ", " << dumpRegister(destReg, nFormals);
        MOTH_END_INSTR(MoveReg)

        MOTH_BEGIN_INSTR(LoadImport)
            s << "i" << index;
        MOTH_END_INSTR(LoadImport)

        MOTH_BEGIN_INSTR(LoadConst)
            s << "C" << index;
        MOTH_END_INSTR(LoadConst)

        MOTH_BEGIN_INSTR(LoadNull)
        MOTH_END_INSTR(LoadNull)

        MOTH_BEGIN_INSTR(LoadZero)
        MOTH_END_INSTR(LoadZero)

        MOTH_BEGIN_INSTR(LoadTrue)
        MOTH_END_INSTR(LoadTrue)

        MOTH_BEGIN_INSTR(LoadFalse)
        MOTH_END_INSTR(LoadFalse)

        MOTH_BEGIN_INSTR(LoadUndefined)
        MOTH_END_INSTR(LoadUndefined)

        MOTH_BEGIN_INSTR(LoadInt)
            s << value;
        MOTH_END_INSTR(LoadInt)

        MOTH_BEGIN_INSTR(MoveConst)
            s << "C" << constIndex << ", " << dumpRegister(destTemp, nFormals);
        MOTH_END_INSTR(MoveConst)

        MOTH_BEGIN_INSTR(LoadLocal)
            if (index < nLocals)
                s << "l" << index;
            else
                s << "a" << (index - nLocals);
        MOTH_END_INSTR(LoadLocal)

        MOTH_BEGIN_INSTR(StoreLocal)
            if (index < nLocals)
                s << "l" << index;
            else
                s << "a" << (index - nLocals);
        MOTH_END_INSTR(StoreLocal)

        MOTH_BEGIN_INSTR(LoadScopedLocal)
            if (index < nLocals)
                s << "l" << index << "@" << scope;
            else
                s << "a" << (index - nLocals) << "@" << scope;
        MOTH_END_INSTR(LoadScopedLocal)

        MOTH_BEGIN_INSTR(StoreScopedLocal)
            if (index < nLocals)
                s << ", " << "l" << index << "@" << scope;
            else
                s << ", " << "a" << (index - nLocals) << "@" << scope;
        MOTH_END_INSTR(StoreScopedLocal)

        MOTH_BEGIN_INSTR(LoadRuntimeString)
            s << stringId;
        MOTH_END_INSTR(LoadRuntimeString)

        MOTH_BEGIN_INSTR(MoveRegExp)
            s << regExpId << ", " << dumpRegister(destReg, nFormals);
        MOTH_END_INSTR(MoveRegExp)

        MOTH_BEGIN_INSTR(LoadClosure)
            s << value;
        MOTH_END_INSTR(LoadClosure)

        MOTH_BEGIN_INSTR(LoadName)
            s << name;
        MOTH_END_INSTR(LoadName)

        MOTH_BEGIN_INSTR(LoadGlobalLookup)
            s << index;
        MOTH_END_INSTR(LoadGlobalLookup)

        MOTH_BEGIN_INSTR(LoadQmlContextPropertyLookup)
            s << index;
        MOTH_END_INSTR(LoadQmlContextPropertyLookup)

        MOTH_BEGIN_INSTR(StoreNameSloppy)
            s << name;
        MOTH_END_INSTR(StoreNameSloppy)

        MOTH_BEGIN_INSTR(StoreNameStrict)
            s << name;
        MOTH_END_INSTR(StoreNameStrict)

        MOTH_BEGIN_INSTR(LoadElement)
            s << dumpRegister(base, nFormals) << "[acc]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(StoreElement)
            s << dumpRegister(base, nFormals) << "[" << dumpRegister(index, nFormals) << "]";
        MOTH_END_INSTR(StoreElement)

        MOTH_BEGIN_INSTR(LoadProperty)
            s << "acc[" << name << "]";
        MOTH_END_INSTR(LoadProperty)

        MOTH_BEGIN_INSTR(LoadOptionalProperty)
            s << "acc[" << name << "], jump(" << ABSOLUTE_OFFSET() << ")";
        MOTH_END_INSTR(LoadOptionalProperty)

        MOTH_BEGIN_INSTR(GetLookup)
            s << "acc(" << index << ")";
        MOTH_END_INSTR(GetLookup)

        MOTH_BEGIN_INSTR(GetOptionalLookup)
            s << "acc(" << index << "), jump(" << ABSOLUTE_OFFSET() << ")";
        MOTH_END_INSTR(GetOptionalLookup)

        MOTH_BEGIN_INSTR(StoreProperty)
            s << dumpRegister(base, nFormals) << "[" << name<< "]";
        MOTH_END_INSTR(StoreProperty)

        MOTH_BEGIN_INSTR(SetLookup)
            s << dumpRegister(base, nFormals) << "(" << index << ")";
        MOTH_END_INSTR(SetLookup)

        MOTH_BEGIN_INSTR(LoadSuperProperty)
            s << dumpRegister(property, nFormals);
        MOTH_END_INSTR(LoadSuperProperty)

        MOTH_BEGIN_INSTR(StoreSuperProperty)
            s << dumpRegister(property, nFormals);
        MOTH_END_INSTR(StoreSuperProperty)

        MOTH_BEGIN_INSTR(Yield)
        MOTH_END_INSTR(Yield)

        MOTH_BEGIN_INSTR(YieldStar)
        MOTH_END_INSTR(YieldStar)

        MOTH_BEGIN_INSTR(Resume)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(Resume)

        MOTH_BEGIN_INSTR(CallValue)
            s << dumpRegister(name, nFormals) << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallValue)

        MOTH_BEGIN_INSTR(CallWithReceiver)
            s << dumpRegister(name, nFormals) << dumpRegister(thisObject, nFormals)
              << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallWithReceiver)

        MOTH_BEGIN_INSTR(CallProperty)
            s << dumpRegister(base, nFormals) << "." << name << dumpArguments(argc, argv, nFormals)
             ;
        MOTH_END_INSTR(CallProperty)

        MOTH_BEGIN_INSTR(CallPropertyLookup)
            s << dumpRegister(base, nFormals) << "." << lookupIndex
              << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallPropertyLookup)

        MOTH_BEGIN_INSTR(CallName)
            s << name << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallName)

        MOTH_BEGIN_INSTR(CallPossiblyDirectEval)
            s << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallPossiblyDirectEval)

        MOTH_BEGIN_INSTR(CallGlobalLookup)
            s << index << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallGlobalLookup)

        MOTH_BEGIN_INSTR(CallQmlContextPropertyLookup)
            s << index << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallQmlContextPropertyLookup)

        MOTH_BEGIN_INSTR(CallWithSpread)
            s << "new " << dumpRegister(func, nFormals) << dumpRegister(thisObject, nFormals)
              << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(CallWithSpread)

        MOTH_BEGIN_INSTR(Construct)
            s << "new " << dumpRegister(func, nFormals) << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(Construct)

        MOTH_BEGIN_INSTR(ConstructWithSpread)
            s << "new " << dumpRegister(func, nFormals) << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(ConstructWithSpread)

        MOTH_BEGIN_INSTR(SetUnwindHandler)
            if (offset)
                s << ABSOLUTE_OFFSET();
            else
                s << "<null>";
        MOTH_END_INSTR(SetUnwindHandler)

        MOTH_BEGIN_INSTR(UnwindDispatch)
        MOTH_END_INSTR(UnwindDispatch)

        MOTH_BEGIN_INSTR(UnwindToLabel)
                s << "(" << level << ") " << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(UnwindToLabel)

        MOTH_BEGIN_INSTR(DeadTemporalZoneCheck)
                s << name;
        MOTH_END_INSTR(DeadTemporalZoneCheck)

        MOTH_BEGIN_INSTR(ThrowException)
        MOTH_END_INSTR(ThrowException)

        MOTH_BEGIN_INSTR(GetException)
        MOTH_END_INSTR(HasException)

        MOTH_BEGIN_INSTR(SetException)
        MOTH_END_INSTR(SetExceptionFlag)

        MOTH_BEGIN_INSTR(CreateCallContext)
        MOTH_END_INSTR(CreateCallContext)

        MOTH_BEGIN_INSTR(PushCatchContext)
            s << index << ", " << name;
        MOTH_END_INSTR(PushCatchContext)

        MOTH_BEGIN_INSTR(PushWithContext)
        MOTH_END_INSTR(PushWithContext)

        MOTH_BEGIN_INSTR(PushBlockContext)
            s << index;
        MOTH_END_INSTR(PushBlockContext)

        MOTH_BEGIN_INSTR(CloneBlockContext)
        MOTH_END_INSTR(CloneBlockContext)

        MOTH_BEGIN_INSTR(PushScriptContext)
            s << index;
        MOTH_END_INSTR(PushScriptContext)

        MOTH_BEGIN_INSTR(PopScriptContext)
        MOTH_END_INSTR(PopScriptContext)

        MOTH_BEGIN_INSTR(PopContext)
        MOTH_END_INSTR(PopContext)

        MOTH_BEGIN_INSTR(GetIterator)
            s << iterator;
        MOTH_END_INSTR(GetIterator)

        MOTH_BEGIN_INSTR(IteratorNext)
            s << dumpRegister(value, nFormals) << ", " << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(IteratorNext)

        MOTH_BEGIN_INSTR(IteratorNextForYieldStar)
            s << dumpRegister(iterator, nFormals) << ", " << dumpRegister(object, nFormals)
              << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(IteratorNextForYieldStar)

        MOTH_BEGIN_INSTR(IteratorClose)
        MOTH_END_INSTR(IteratorClose)

        MOTH_BEGIN_INSTR(DestructureRestElement)
        MOTH_END_INSTR(DestructureRestElement)

        MOTH_BEGIN_INSTR(DeleteProperty)
            s << dumpRegister(base, nFormals) << "[" << dumpRegister(index, nFormals) << "]";
        MOTH_END_INSTR(DeleteProperty)

        MOTH_BEGIN_INSTR(DeleteName)
            s << name;
        MOTH_END_INSTR(DeleteName)

        MOTH_BEGIN_INSTR(TypeofName)
            s << name;
        MOTH_END_INSTR(TypeofName)

        MOTH_BEGIN_INSTR(TypeofValue)
        MOTH_END_INSTR(TypeofValue)

        MOTH_BEGIN_INSTR(DeclareVar)
            s << isDeletable << ", " << varName;
        MOTH_END_INSTR(DeclareVar)

        MOTH_BEGIN_INSTR(DefineArray)
            s << dumpRegister(args, nFormals) << ", " << argc;
        MOTH_END_INSTR(DefineArray)

        MOTH_BEGIN_INSTR(DefineObjectLiteral)
            s << internalClassId
              << ", " << argc
              << ", " << dumpRegister(args, nFormals);
        MOTH_END_INSTR(DefineObjectLiteral)

        MOTH_BEGIN_INSTR(CreateClass)
            s << classIndex
              << ", " << dumpRegister(heritage, nFormals)
              << ", " << dumpRegister(computedNames, nFormals);
        MOTH_END_INSTR(CreateClass)

        MOTH_BEGIN_INSTR(CreateMappedArgumentsObject)
        MOTH_END_INSTR(CreateMappedArgumentsObject)

        MOTH_BEGIN_INSTR(CreateUnmappedArgumentsObject)
        MOTH_END_INSTR(CreateUnmappedArgumentsObject)

        MOTH_BEGIN_INSTR(CreateRestParameter)
            s << argIndex;
        MOTH_END_INSTR(CreateRestParameter)

        MOTH_BEGIN_INSTR(ConvertThisToObject)
        MOTH_END_INSTR(ConvertThisToObject)

        MOTH_BEGIN_INSTR(LoadSuperConstructor)
        MOTH_END_INSTR(LoadSuperConstructor)

        MOTH_BEGIN_INSTR(ToObject)
        MOTH_END_INSTR(ToObject)

        MOTH_BEGIN_INSTR(Jump)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(Jump)

        MOTH_BEGIN_INSTR(JumpTrue)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(JumpTrue)

        MOTH_BEGIN_INSTR(JumpFalse)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(JumpFalse)

        MOTH_BEGIN_INSTR(JumpNotUndefined)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(JumpNotUndefined)

        MOTH_BEGIN_INSTR(JumpNoException)
            s << ABSOLUTE_OFFSET();
        MOTH_END_INSTR(JumpNoException)

        MOTH_BEGIN_INSTR(CheckException)
        MOTH_END_INSTR(CheckException)

        MOTH_BEGIN_INSTR(CmpEqNull)
        MOTH_END_INSTR(CmpEqNull)

        MOTH_BEGIN_INSTR(CmpNeNull)
        MOTH_END_INSTR(CmpNeNull)

        MOTH_BEGIN_INSTR(CmpEqInt)
            s << lhs;
        MOTH_END_INSTR(CmpEq)

        MOTH_BEGIN_INSTR(CmpNeInt)
                s << lhs;
        MOTH_END_INSTR(CmpNeInt)

        MOTH_BEGIN_INSTR(CmpEq)
            s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpEq)

        MOTH_BEGIN_INSTR(CmpNe)
                s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpNe)

        MOTH_BEGIN_INSTR(CmpGt)
                s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpGt)

        MOTH_BEGIN_INSTR(CmpGe)
                s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpGe)

        MOTH_BEGIN_INSTR(CmpLt)
                s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpLt)

        MOTH_BEGIN_INSTR(CmpLe)
                s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpLe)

        MOTH_BEGIN_INSTR(CmpStrictEqual)
            s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpStrictEqual)

        MOTH_BEGIN_INSTR(CmpStrictNotEqual)
            s << dumpRegister(lhs, nFormals);
        MOTH_END_INSTR(CmpStrictNotEqual)

        MOTH_BEGIN_INSTR(UNot)
        MOTH_END_INSTR(UNot)

        MOTH_BEGIN_INSTR(UPlus)
        MOTH_END_INSTR(UPlus)

        MOTH_BEGIN_INSTR(UMinus)
        MOTH_END_INSTR(UMinus)

        MOTH_BEGIN_INSTR(UCompl)
        MOTH_END_INSTR(UCompl)

        MOTH_BEGIN_INSTR(Increment)
        MOTH_END_INSTR(Increment)

        MOTH_BEGIN_INSTR(Decrement)
        MOTH_END_INSTR(Decrement)

        MOTH_BEGIN_INSTR(Add)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Add)

        MOTH_BEGIN_INSTR(BitAnd)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOr)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXor)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(UShr)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(UShr)

        MOTH_BEGIN_INSTR(Shr)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Shr)

        MOTH_BEGIN_INSTR(Shl)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Shl)

        MOTH_BEGIN_INSTR(BitAndConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(BitAndConst)

        MOTH_BEGIN_INSTR(BitOrConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXorConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(UShrConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(UShrConst)

        MOTH_BEGIN_INSTR(ShrConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(ShrConst)

        MOTH_BEGIN_INSTR(ShlConst)
            s << "acc, " << rhs;
        MOTH_END_INSTR(ShlConst)

        MOTH_BEGIN_INSTR(Exp)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Exp)

        MOTH_BEGIN_INSTR(Mul)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Mul)

        MOTH_BEGIN_INSTR(Div)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Div)

        MOTH_BEGIN_INSTR(Mod)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Mod)

        MOTH_BEGIN_INSTR(Sub)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(As)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(CmpIn)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(CmpIn)

        MOTH_BEGIN_INSTR(CmpInstanceOf)
            s << dumpRegister(lhs, nFormals) << ", acc";
        MOTH_END_INSTR(CmpInstanceOf)

        MOTH_BEGIN_INSTR(Ret)
        MOTH_END_INSTR(Ret)

        MOTH_BEGIN_INSTR(Debug)
        MOTH_END_INSTR(Debug)

        MOTH_BEGIN_INSTR(InitializeBlockDeadTemporalZone)
            s << dumpRegister(firstReg, nFormals) << ", " << count;
        MOTH_END_INSTR(InitializeBlockDeadTemporalZone)

        MOTH_BEGIN_INSTR(ThrowOnNullOrUndefined)
        MOTH_END_INSTR(ThrowOnNullOrUndefined)

        MOTH_BEGIN_INSTR(GetTemplateObject)
            s << index;
        MOTH_END_INSTR(GetTemplateObject)

        MOTH_BEGIN_INSTR(TailCall)
            s << dumpRegister(func, nFormals) << dumpRegister(thisObject, nFormals) << dumpArguments(argc, argv, nFormals);
        MOTH_END_INSTR(TailCall)
    }
    return output;
}

}
}
QT_END_NAMESPACE
