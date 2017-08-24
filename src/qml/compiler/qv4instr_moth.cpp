/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4instr_moth_p.h"

using namespace QV4;
using namespace QV4::Moth;

int Instr::size(Type type)
{
#define MOTH_RETURN_INSTR_SIZE(I) case Type::I: return InstrMeta<(int)Type::I>::Size;
    switch (type) {
    FOR_EACH_MOTH_INSTR(MOTH_RETURN_INSTR_SIZE)
    }
#undef MOTH_RETURN_INSTR_SIZE
}

static QByteArray alignedNumber(int n) {
    QByteArray number = QByteArray::number(n);
    while (number.size() < 12)
        number.prepend(' ');
    return number;
}

static QByteArray alignedLineNumber(int line) {
    if (line > 0)
        return alignedNumber(static_cast<int>(line));
    return QByteArray("            ");
}

static QString toString(QV4::ReturnedValue v)
{
#ifdef V4_BOOTSTRAP
    return QStringLiteral("string-const(%1)").arg(v);
#else // !V4_BOOTSTRAP
    Value val = Value::fromReturnedValue(v);
    QString result;
    if (val.isInt32())
        result = QLatin1String("int ");
    else if (val.isDouble())
        result = QLatin1String("double ");
    if (val.isEmpty())
        result += QLatin1String("empty");
    else
        result += val.toQStringNoThrow();
    return result;
#endif // V4_BOOTSTRAP
}

template<typename T>
size_t absoluteInstructionOffset(const char *codeStart, const T &instr)
{
    return reinterpret_cast<const char *>(&instr) - codeStart + offsetof(T, offset) + instr.offset;
}

#define MOTH_BEGIN_INSTR(I) \
    case Instr::Type::I: {\
    const InstrMeta<int(Instr::Type::I)>::DataType &instr = InstrMeta<int(Instr::Type::I)>::data(*genericInstr); \
    Q_UNUSED(instr); \
    QDebug d = qDebug(); \
    d.noquote(); \
    d.nospace(); \
    d << alignedLineNumber(line) << alignedNumber(int(code - start)).constData() << ":    " << #I << " "; \
    code += InstrMeta<int(Instr::Type::I)>::Size; \

#define MOTH_END_INSTR(I) } break;

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace Moth {

void dumpConstantTable(const Value *constants, uint count)
{
    QDebug d = qDebug();
    d.nospace();
    for (uint i = 0; i < count; ++i)
        d << alignedNumber(i).constData() << ":    "
          << toString(constants[i].asReturnedValue()).toUtf8().constData() << "\n";
}

void dumpBytecode(const char *code, int len, int nLocals, int nFormals, int startLine, const QVector<int> &lineNumberMapping)
{
    int lastLine = -1;
    const char *start = code;
    const char *end = code + len;
    while (code < end) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        int line = startLine + ((code == start) ? 0 : lineNumberMapping.lastIndexOf(static_cast<uint>(code - start)) + 1);
        if (line > lastLine)
            lastLine = line;
        else
            line = -1;
        switch (genericInstr->Common.instructionType) {

        MOTH_BEGIN_INSTR(LoadReg)
            d << StackSlot::createRegister(instr.reg).dump(nFormals);
        MOTH_END_INSTR(LoadReg)

        MOTH_BEGIN_INSTR(StoreReg)
            d << StackSlot::createRegister(instr.reg).dump(nFormals);
        MOTH_END_INSTR(StoreReg)

        MOTH_BEGIN_INSTR(MoveReg)
            d << StackSlot::createRegister(instr.destReg).dump(nFormals) << ", " << StackSlot::createRegister(instr.srcReg).dump(nFormals);
        MOTH_END_INSTR(MoveReg)

        MOTH_BEGIN_INSTR(LoadConst)
            d << "C" << instr.index;
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
            d << instr.value;
        MOTH_END_INSTR(LoadInt)

        MOTH_BEGIN_INSTR(MoveConst)
            d << StackSlot::createRegister(instr.destTemp).dump(nFormals) << ", C" << instr.constIndex;
        MOTH_END_INSTR(MoveConst)

        MOTH_BEGIN_INSTR(LoadScopedLocal)
            if (instr.index < nLocals)
                d << "l" << instr.index << "@" << instr.scope;
            else
                d << "a" << (instr.index - nLocals) << "@" << instr.scope;
        MOTH_END_INSTR(LoadScopedLocal)

        MOTH_BEGIN_INSTR(StoreScopedLocal)
            if (instr.index < nLocals)
                d << ", " << "l" << instr.index << "@" << instr.scope;
            else
                d << ", " << "a" << (instr.index - nLocals) << "@" << instr.scope;
        MOTH_END_INSTR(StoreScopedLocal)

        MOTH_BEGIN_INSTR(LoadRuntimeString)
            d << instr.stringId;
        MOTH_END_INSTR(LoadRuntimeString)

        MOTH_BEGIN_INSTR(LoadRegExp)
            d << instr.regExpId;
        MOTH_END_INSTR(LoadRegExp)

        MOTH_BEGIN_INSTR(LoadClosure)
            d << instr.value;
        MOTH_END_INSTR(LoadClosure)

        MOTH_BEGIN_INSTR(LoadName)
            d << instr.name;
        MOTH_END_INSTR(LoadName)

        MOTH_BEGIN_INSTR(LoadGlobalLookup)
            d << instr.index;
        MOTH_END_INSTR(LoadGlobalLookup)

        MOTH_BEGIN_INSTR(StoreNameSloppy)
            d << instr.name;
        MOTH_END_INSTR(StoreNameSloppy)

        MOTH_BEGIN_INSTR(StoreNameStrict)
            d << instr.name;
        MOTH_END_INSTR(StoreNameStrict)

        MOTH_BEGIN_INSTR(LoadElement)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << StackSlot::createRegister(instr.index).dump(nFormals) << "]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(LoadElementA)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[acc]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(StoreElement)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << StackSlot::createRegister(instr.index).dump(nFormals) << "]";
        MOTH_END_INSTR(StoreElement)

        MOTH_BEGIN_INSTR(LoadProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.name << "]";
        MOTH_END_INSTR(LoadProperty)

        MOTH_BEGIN_INSTR(LoadPropertyA)
            d << "acc[" << instr.name << "]";
        MOTH_END_INSTR(LoadElementA)

        MOTH_BEGIN_INSTR(GetLookup)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "(" << instr.index << ")";
        MOTH_END_INSTR(GetLookup)

        MOTH_BEGIN_INSTR(GetLookupA)
            d << "acc(" << instr.index << ")";
        MOTH_END_INSTR(GetLookupA)

        MOTH_BEGIN_INSTR(StoreProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.name<< "]";
        MOTH_END_INSTR(StoreProperty)

        MOTH_BEGIN_INSTR(SetLookup)
            d << StackSlot::createRegister(instr.base).dump(nFormals);
        MOTH_END_INSTR(SetLookup)

        MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(StoreScopeObjectProperty)

        MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadScopeObjectProperty)

        MOTH_BEGIN_INSTR(StoreContextObjectProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(StoreContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadContextObjectProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadIdObject)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.index << "]";
        MOTH_END_INSTR(LoadIdObject)

        MOTH_BEGIN_INSTR(CallValue)
            d << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallValue)

        MOTH_BEGIN_INSTR(CallProperty)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "." << instr.name << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallProperty)

        MOTH_BEGIN_INSTR(CallPropertyLookup)
            d << instr.lookupIndex << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallPropertyLookup)

        MOTH_BEGIN_INSTR(CallElement)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << StackSlot::createRegister(instr.index).dump(nFormals) << "]" << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallElement)

        MOTH_BEGIN_INSTR(CallName)
            d << instr.name << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallName)

        MOTH_BEGIN_INSTR(CallPossiblyDirectEval)
            d << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallPossiblyDirectEval)

        MOTH_BEGIN_INSTR(CallGlobalLookup)
            d << instr.index << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(CallGlobalLookup)

        MOTH_BEGIN_INSTR(SetExceptionHandler)
            if (instr.offset)
                d << absoluteInstructionOffset(start, instr);
            else
                d << "<null>";
        MOTH_END_INSTR(SetExceptionHandler)

        MOTH_BEGIN_INSTR(ThrowException)
        MOTH_END_INSTR(ThrowException)

        MOTH_BEGIN_INSTR(GetException)
        MOTH_END_INSTR(HasException)

        MOTH_BEGIN_INSTR(SetException)
        MOTH_END_INSTR(SetExceptionFlag)

        MOTH_BEGIN_INSTR(UnwindException)
        MOTH_END_INSTR(UnwindException)

        MOTH_BEGIN_INSTR(PushCatchContext)
            d << StackSlot::createRegister(instr.reg).dump(nFormals) << ", " << instr.name;
        MOTH_END_INSTR(PushCatchContext)

        MOTH_BEGIN_INSTR(PushWithContext)
            d << StackSlot::createRegister(instr.reg).dump(nFormals);
        MOTH_END_INSTR(PushWithContext)

        MOTH_BEGIN_INSTR(PopContext)
            d << StackSlot::createRegister(instr.reg).dump(nFormals);
        MOTH_END_INSTR(PopContext)

        MOTH_BEGIN_INSTR(ForeachIteratorObject)
        MOTH_END_INSTR(ForeachIteratorObject)

        MOTH_BEGIN_INSTR(ForeachNextPropertyName)
        MOTH_END_INSTR(ForeachNextPropertyName)

        MOTH_BEGIN_INSTR(DeleteMember)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << instr.member << "]";
        MOTH_END_INSTR(DeleteMember)

        MOTH_BEGIN_INSTR(DeleteSubscript)
            d << StackSlot::createRegister(instr.base).dump(nFormals) << "[" << StackSlot::createRegister(instr.index).dump(nFormals) << "]";
        MOTH_END_INSTR(DeleteSubscript)

        MOTH_BEGIN_INSTR(DeleteName)
            d << instr.name;
        MOTH_END_INSTR(DeleteName)

        MOTH_BEGIN_INSTR(TypeofName)
            d << instr.name;
        MOTH_END_INSTR(TypeofName)

        MOTH_BEGIN_INSTR(TypeofValue)
        MOTH_END_INSTR(TypeofValue)

        MOTH_BEGIN_INSTR(DeclareVar)
            d << instr.isDeletable << ", " << instr.varName;
        MOTH_END_INSTR(DeclareVar)

        MOTH_BEGIN_INSTR(DefineArray)
            d << StackSlot::createRegister(instr.args).dump(nFormals) << ", " << instr.argc;
        MOTH_END_INSTR(DefineArray)

        MOTH_BEGIN_INSTR(DefineObjectLiteral)
            d << StackSlot::createRegister(instr.args).dump(nFormals)
              << ", " << instr.internalClassId
              << ", " << instr.arrayValueCount
              << ", " << instr.arrayGetterSetterCountAndFlags;
        MOTH_END_INSTR(DefineObjectLiteral)

        MOTH_BEGIN_INSTR(CreateMappedArgumentsObject)
        MOTH_END_INSTR(CreateMappedArgumentsObject)

        MOTH_BEGIN_INSTR(CreateUnmappedArgumentsObject)
        MOTH_END_INSTR(CreateUnmappedArgumentsObject)

        MOTH_BEGIN_INSTR(ConvertThisToObject)
        MOTH_END_INSTR(ConvertThisToObject)

        MOTH_BEGIN_INSTR(Construct)
            d << "new" << StackSlot::createRegister(instr.func).dump(nFormals) << "(" << StackSlot::createRegister(instr.callData).dump(nFormals) << ")";
        MOTH_END_INSTR(Construct)

        MOTH_BEGIN_INSTR(Jump)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(Jump)

        MOTH_BEGIN_INSTR(JumpEq)
            d << "acc  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpEq)

        MOTH_BEGIN_INSTR(JumpNe)
            d << "acc  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpNe)

        MOTH_BEGIN_INSTR(CmpJmpEqNull)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpEqNull)

        MOTH_BEGIN_INSTR(CmpJmpNeNull)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpNeNull)

        MOTH_BEGIN_INSTR(CmpJmpEqInt)
            d << instr.lhs << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpEq)

        MOTH_BEGIN_INSTR(CmpJmpNeInt)
                d << instr.lhs << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpNe)


        MOTH_BEGIN_INSTR(CmpJmpEq)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpEq)

        MOTH_BEGIN_INSTR(CmpJmpNe)
                d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpNe)

        MOTH_BEGIN_INSTR(CmpJmpGt)
                d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpGt)

        MOTH_BEGIN_INSTR(CmpJmpGe)
                d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpGe)

        MOTH_BEGIN_INSTR(CmpJmpLt)
                d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpLt)

        MOTH_BEGIN_INSTR(CmpJmpLe)
                d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpLe)

        MOTH_BEGIN_INSTR(JumpStrictEqual)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictEqual)

        MOTH_BEGIN_INSTR(JumpStrictNotEqual)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictNotEqual)

        MOTH_BEGIN_INSTR(JumpStrictEqualStackSlotInt)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << instr.rhs << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictEqualStackSlotInt)

        MOTH_BEGIN_INSTR(JumpStrictNotEqualStackSlotInt)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", " << instr.rhs << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

        MOTH_BEGIN_INSTR(UNot)
        MOTH_END_INSTR(UNot)

        MOTH_BEGIN_INSTR(UPlus)
        MOTH_END_INSTR(UPlus)

        MOTH_BEGIN_INSTR(UMinus)
        MOTH_END_INSTR(UMinus)

        MOTH_BEGIN_INSTR(UCompl)
        MOTH_END_INSTR(UCompl)

        MOTH_BEGIN_INSTR(Increment)
        MOTH_END_INSTR(PreIncrement)

        MOTH_BEGIN_INSTR(Decrement)
        MOTH_END_INSTR(PreDecrement)

        MOTH_BEGIN_INSTR(Binop)
            d << instr.alu << ", " << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Binop)

        MOTH_BEGIN_INSTR(Add)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Add)

        MOTH_BEGIN_INSTR(BitAnd)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOr)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXor)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(Shr)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Shr)

        MOTH_BEGIN_INSTR(Shl)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Shl)

        MOTH_BEGIN_INSTR(BitAndConst)
            d << "acc, " << instr.rhs;
        MOTH_END_INSTR(BitAndConst)

        MOTH_BEGIN_INSTR(BitOrConst)
            d << "acc, " << instr.rhs;
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXorConst)
            d << "acc, " << instr.rhs;
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(ShrConst)
            d << "acc, " << instr.rhs;
        MOTH_END_INSTR(ShrConst)

        MOTH_BEGIN_INSTR(ShlConst)
            d << "acc, " << instr.rhs;
        MOTH_END_INSTR(ShlConst)

        MOTH_BEGIN_INSTR(Mul)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Mul)

        MOTH_BEGIN_INSTR(Sub)
            d << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(BinopContext)
            d << instr.alu << " " << StackSlot::createRegister(instr.lhs).dump(nFormals) << ", acc";
        MOTH_END_INSTR(BinopContext)

        MOTH_BEGIN_INSTR(Ret)
        MOTH_END_INSTR(Ret)

#ifndef QT_NO_QML_DEBUGGER
        MOTH_BEGIN_INSTR(Debug)
        MOTH_END_INSTR(Debug)
#endif // QT_NO_QML_DEBUGGER

        MOTH_BEGIN_INSTR(LoadQmlContext)
            d << StackSlot::createRegister(instr.result).dump(nFormals);
        MOTH_END_INSTR(LoadQmlContext)

        MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
            d << StackSlot::createRegister(instr.result).dump(nFormals);
        MOTH_END_INSTR(LoadQmlImportedScripts)

        MOTH_BEGIN_INSTR(LoadQmlSingleton)
            d << instr.name;
        MOTH_END_INSTR(LoadQmlSingleton)

        default:
            Q_UNREACHABLE();
        }
    }
}

}
}
QT_END_NAMESPACE
