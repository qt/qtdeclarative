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
#define MOTH_RETURN_INSTR_SIZE(I, FMT) case I: return InstrMeta<(int)I>::Size;
    switch (type) {
    FOR_EACH_MOTH_INSTR(MOTH_RETURN_INSTR_SIZE)
    default: return 0;
    }
#undef MOTH_RETURN_INSTR_SIZE
}

static QByteArray alignedNumber(int n) {
    QByteArray number = QByteArray::number(n);
    while (number.size() < 12)
        number.prepend(' ');
    return number;
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
    case Instr::I: {\
    const InstrMeta<int(Instr::I)>::DataType &instr = InstrMeta<int(Instr::I)>::data(*genericInstr); \
    Q_UNUSED(instr); \
    QDebug d = qDebug(); \
    d.noquote(); \
    d.nospace(); \
    d << alignedNumber(int(code - start)).constData() << ":    " << #I << " "; \
    code += InstrMeta<int(Instr::I)>::Size; \

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

void dumpBytecode(const char *code, int len, int nFormals)
{
    const char *start = code;
    const char *end = code + len;
    while (code < end) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {

        MOTH_BEGIN_INSTR(LoadReg)
            d << instr.reg.dump(nFormals);
        MOTH_END_INSTR(LoadReg)

        MOTH_BEGIN_INSTR(StoreReg)
            d << instr.reg.dump(nFormals);
        MOTH_END_INSTR(StoreReg)

        MOTH_BEGIN_INSTR(MoveReg)
            d << instr.destReg.dump(nFormals) << ", " << instr.srcReg.dump(nFormals);
        MOTH_END_INSTR(MoveReg)

        MOTH_BEGIN_INSTR(LoadConst)
            d << "C" << instr.index;
        MOTH_END_INSTR(LoadConst)

        MOTH_BEGIN_INSTR(MoveConst)
            d << instr.destTemp.dump(nFormals) << ", C" << instr.constIndex;
        MOTH_END_INSTR(MoveConst)

        MOTH_BEGIN_INSTR(LoadScopedLocal)
            d << "l" << instr.index << "@" << instr.scope;
        MOTH_END_INSTR(LoadScopedLocal)

        MOTH_BEGIN_INSTR(StoreScopedLocal)
            d << ", " << "l" << instr.index << "@" << instr.scope;
        MOTH_END_INSTR(StoreScopedLocal)

        MOTH_BEGIN_INSTR(LoadScopedArgument)
            d <<  "a" << instr.index << "@" << instr.scope;
        MOTH_END_INSTR(LoadScopedArgument)

        MOTH_BEGIN_INSTR(StoreScopedArgument)
            d << "a" << instr.index << "@" << instr.scope;
        MOTH_END_INSTR(StoreScopedArgument)

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
            d << instr.base.dump(nFormals) << "[" << instr.index.dump(nFormals) << "]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(LoadElementA)
            d << instr.base.dump(nFormals) << "[acc]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(StoreElement)
            d << instr.base.dump(nFormals) << "[" << instr.index.dump(nFormals) << "]";
        MOTH_END_INSTR(StoreElement)

        MOTH_BEGIN_INSTR(LoadProperty)
            d << instr.base.dump(nFormals) << "[" << instr.name << "]";
        MOTH_END_INSTR(LoadProperty)

        MOTH_BEGIN_INSTR(LoadPropertyA)
            d << "acc[" << instr.name << "]";
        MOTH_END_INSTR(LoadElementA)

        MOTH_BEGIN_INSTR(GetLookup)
            d << instr.base.dump(nFormals) << "(" << instr.index << ")";
        MOTH_END_INSTR(GetLookup)

        MOTH_BEGIN_INSTR(GetLookupA)
            d << "acc(" << instr.index << ")";
        MOTH_END_INSTR(GetLookupA)

        MOTH_BEGIN_INSTR(StoreProperty)
            d << instr.base.dump(nFormals) << "[" << instr.name<< "]";
        MOTH_END_INSTR(StoreProperty)

        MOTH_BEGIN_INSTR(SetLookup)
            d << instr.base.dump(nFormals);
        MOTH_END_INSTR(SetLookup)

        MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
            d << instr.base.dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(StoreScopeObjectProperty)

        MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
            d << instr.base.dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadScopeObjectProperty)

        MOTH_BEGIN_INSTR(StoreContextObjectProperty)
            d << instr.base.dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(StoreContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadContextObjectProperty)
            d << instr.base.dump(nFormals) << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadIdObject)
            d << instr.base.dump(nFormals) << "[" << instr.index << "]";
        MOTH_END_INSTR(LoadIdObject)

        MOTH_BEGIN_INSTR(CallValue)
            d << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CallValue)

        MOTH_BEGIN_INSTR(CallProperty)
            d << instr.base.dump(nFormals) << "." << instr.name << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CallProperty)

        MOTH_BEGIN_INSTR(CallPropertyLookup)
            d << instr.lookupIndex << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CallPropertyLookup)

        MOTH_BEGIN_INSTR(CallElement)
            d << instr.base.dump(nFormals) << "[" << instr.index.dump(nFormals) << "]" << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CallElement)

        MOTH_BEGIN_INSTR(CallName)
            d << instr.name << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CallName)

        MOTH_BEGIN_INSTR(CallGlobalLookup)
            d << instr.index << "(" << instr.callData.dump(nFormals) << ")";
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
            d << instr.reg.dump(nFormals) << ", " << instr.name;
        MOTH_END_INSTR(PushCatchContext)

        MOTH_BEGIN_INSTR(PushWithContext)
            d << instr.reg.dump(nFormals);
        MOTH_END_INSTR(PushWithContext)

        MOTH_BEGIN_INSTR(PopContext)
            d << instr.reg.dump(nFormals);
        MOTH_END_INSTR(PopContext)

        MOTH_BEGIN_INSTR(ForeachIteratorObject)
        MOTH_END_INSTR(ForeachIteratorObject)

        MOTH_BEGIN_INSTR(ForeachNextPropertyName)
        MOTH_END_INSTR(ForeachNextPropertyName)

        MOTH_BEGIN_INSTR(DeleteMember)
            d << instr.base.dump(nFormals) << "[" << instr.member << "]";
        MOTH_END_INSTR(DeleteMember)

        MOTH_BEGIN_INSTR(DeleteSubscript)
            d << instr.base.dump(nFormals) << "[" << instr.index.dump(nFormals) << "]";
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
            d << instr.args.dump(nFormals) << ", " << instr.argc;
        MOTH_END_INSTR(DefineArray)

        MOTH_BEGIN_INSTR(DefineObjectLiteral)
            d << instr.args.dump(nFormals)
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

        MOTH_BEGIN_INSTR(CreateValue)
            d << "new" << instr.func.dump(nFormals) << "(" << instr.callData.dump(nFormals) << ")";
        MOTH_END_INSTR(CreateValue)

        MOTH_BEGIN_INSTR(CreateProperty)
            d << "new" << instr.name << "(" << instr.callData.dump(nFormals) << instr.argc << ")";
        MOTH_END_INSTR(CreateProperty)

        MOTH_BEGIN_INSTR(ConstructPropertyLookup)
            d << "new" << instr.index << "(" << instr.callData.dump(nFormals) << instr.argc << ")";
        MOTH_END_INSTR(ConstructPropertyLookup)

        MOTH_BEGIN_INSTR(CreateName)
            d << "new" << instr.name << "(" << instr.callData.dump(nFormals) << instr.argc << ")";
        MOTH_END_INSTR(CreateName)

        MOTH_BEGIN_INSTR(ConstructGlobalLookup)
            d << "new" << instr.index << "(" << instr.callData.dump(nFormals) << instr.argc << ")";
        MOTH_END_INSTR(ConstructGlobalLookup)

        MOTH_BEGIN_INSTR(Jump)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(Jump)

        MOTH_BEGIN_INSTR(JumpEq)
            d << "acc  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpEq)

        MOTH_BEGIN_INSTR(JumpNe)
            d << "acc  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpNe)

        MOTH_BEGIN_INSTR(CmpJmpEq)
            d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpEq)

        MOTH_BEGIN_INSTR(CmpJmpNe)
                d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpNe)

        MOTH_BEGIN_INSTR(CmpJmpGt)
                d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpGt)

        MOTH_BEGIN_INSTR(CmpJmpGe)
                d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpGe)

        MOTH_BEGIN_INSTR(CmpJmpLt)
                d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpLt)

        MOTH_BEGIN_INSTR(CmpJmpLe)
                d << instr.lhs.dump(nFormals) << ", " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(CmpJmpLe)

        MOTH_BEGIN_INSTR(JumpStrictEqual)
            d << instr.lhs.dump(nFormals) << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictEqual)

        MOTH_BEGIN_INSTR(JumpStrictNotEqual)
            d << instr.lhs.dump(nFormals) << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictNotEqual)

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
            d << instr.alu << ", " << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(Binop)

        MOTH_BEGIN_INSTR(Add)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(Add)

        MOTH_BEGIN_INSTR(BitAnd)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOr)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXor)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(Shr)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(Shr)

        MOTH_BEGIN_INSTR(Shl)
            d << instr.lhs.dump(nFormals) << ", acc";
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
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(Mul)

        MOTH_BEGIN_INSTR(Sub)
            d << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(BinopContext)
            d << instr.alu << " " << instr.lhs.dump(nFormals) << ", acc";
        MOTH_END_INSTR(BinopContext)

        MOTH_BEGIN_INSTR(Ret)
            d << instr.result.dump(nFormals);
        MOTH_END_INSTR(Ret)

    #ifndef QT_NO_QML_DEBUGGER
        MOTH_BEGIN_INSTR(Debug)
        MOTH_END_INSTR(Debug)

        MOTH_BEGIN_INSTR(Line)
            d << instr.lineNumber;
        MOTH_END_INSTR(Line)
    #endif // QT_NO_QML_DEBUGGER

        MOTH_BEGIN_INSTR(LoadThis)
        MOTH_END_INSTR(LoadThis)

        MOTH_BEGIN_INSTR(LoadQmlContext)
            d << instr.result.dump(nFormals);
        MOTH_END_INSTR(LoadQmlContext)

        MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
            d << instr.result.dump(nFormals);
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
