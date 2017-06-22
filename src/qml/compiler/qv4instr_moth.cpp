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
    result += QLatin1String("const(");
    if (val.isEmpty())
        result += QLatin1String("empty");
    else
        result += val.toQStringNoThrow();
    return result + QLatin1String(")");
#endif // V4_BOOTSTRAP
}

template<typename T>
int absoluteInstructionOffset(const char *codeStart, const T &instr)
{
    return reinterpret_cast<const char *>(&instr) - codeStart + offsetof(T, offset) + instr.offset;
}

#define MOTH_BEGIN_INSTR(I) \
    case Instr::I: {\
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    Q_UNUSED(instr); \
    QDebug d = qDebug(); \
    d.nospace(); \
    d << alignedNumber(code - start).constData() << ":    " << #I << " "; \
    code += InstrMeta<(int)Instr::I>::Size; \

#define MOTH_END_INSTR(I) } break;

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace Moth {

QDebug operator<<(QDebug dbg, const Param &p)
{
    if (p.scope == 0) // const
        dbg << "C" << p.index;
    else if (p.scope == 1) // temp
        dbg << "%" << p.index;
    else if (p.scope == 2) // arg
        dbg << "#" << p.index;
    else if (p.scope == 3) // local
        dbg << "$" << p.index;
    else if (p.scope & 1) // scoped local
        dbg << "$" << p.index << "@" << ((p.scope - 3)/2);
    else // scoped arg
        dbg << "#" << p.index << "@" << ((p.scope - 2)/2);
    return dbg;
}

void dumpBytecode(const char *code, int len)
{
    const char *start = code;
    const char *end = code + len;
    while (code < end) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {

        MOTH_BEGIN_INSTR(Move)
            d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(Move)

        MOTH_BEGIN_INSTR(MoveConst)
            d << instr.result << ", " << toString(instr.source).toUtf8().constData();
        MOTH_END_INSTR(MoveConst)

        MOTH_BEGIN_INSTR(SwapTemps)
            d << instr.left << ", " << instr.right;
        MOTH_END_INSTR(MoveTemp)

        MOTH_BEGIN_INSTR(LoadRuntimeString)
            d << instr.result << ", " << instr.stringId;
        MOTH_END_INSTR(LoadRuntimeString)

        MOTH_BEGIN_INSTR(LoadRegExp)
            d << instr.result << ", " << instr.regExpId;
        MOTH_END_INSTR(LoadRegExp)

        MOTH_BEGIN_INSTR(LoadClosure)
            d << instr.result << ", " << instr.value;
        MOTH_END_INSTR(LoadClosure)

        MOTH_BEGIN_INSTR(LoadName)
            d << instr.result << ", " << instr.name;
        MOTH_END_INSTR(LoadName)

        MOTH_BEGIN_INSTR(GetGlobalLookup)
            d << instr.result << ", " << instr.index;
        MOTH_END_INSTR(GetGlobalLookup)

        MOTH_BEGIN_INSTR(StoreName)
            d << instr.name << ", " << instr.source;
        MOTH_END_INSTR(StoreName)

        MOTH_BEGIN_INSTR(LoadElement)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(LoadElementLookup)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(LoadElementLookup)

        MOTH_BEGIN_INSTR(StoreElement)
            d << instr.base << "[" << instr.index << "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreElement)

        MOTH_BEGIN_INSTR(StoreElementLookup)
            d << instr.base << "[" << instr.index << "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreElementLookup)

        MOTH_BEGIN_INSTR(LoadProperty)
            d << instr.result << ", " << instr.base << "[" << instr.name << "]";
        MOTH_END_INSTR(LoadProperty)

        MOTH_BEGIN_INSTR(GetLookup)
            d << instr.result << ", " << instr.base;
        MOTH_END_INSTR(GetLookup)

        MOTH_BEGIN_INSTR(StoreProperty)
            d << instr.base << "[" << instr.name<< "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreProperty)

        MOTH_BEGIN_INSTR(SetLookup)
            d << instr.base << ", " << instr.source;
        MOTH_END_INSTR(SetLookup)

        MOTH_BEGIN_INSTR(StoreQObjectProperty)
            d << instr.base << "[" << instr.propertyIndex << "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreQObjectProperty)

        MOTH_BEGIN_INSTR(LoadQObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadQObjectProperty)

        MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
            d << instr.base << "[" << instr.propertyIndex << "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreScopeObjectProperty)

        MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadScopeObjectProperty)

        MOTH_BEGIN_INSTR(StoreContextObjectProperty)
            d << instr.base << "[" << instr.propertyIndex << "]" << ", " << instr.source;
        MOTH_END_INSTR(StoreContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadContextObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadIdObject)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(LoadIdObject)

        MOTH_BEGIN_INSTR(LoadAttachedQObjectProperty)
            d << instr.result << ", " << instr.attachedPropertiesId << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadAttachedQObjectProperty)

        MOTH_BEGIN_INSTR(LoadSingletonQObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.propertyIndex << "]";
        MOTH_END_INSTR(LoadSingletonQObjectProperty)

        MOTH_BEGIN_INSTR(Push)
            d << instr.value;
        MOTH_END_INSTR(Push)

        MOTH_BEGIN_INSTR(CallValue)
            d << instr.result << ", " << instr.dest << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallValue)

        MOTH_BEGIN_INSTR(CallProperty)
            d << instr.result << ", " << instr.base<<"."<<instr.name << "(" << instr.callData
              << ", " << instr.argc << ")";
        MOTH_END_INSTR(CallProperty)

        MOTH_BEGIN_INSTR(CallPropertyLookup)
            d << instr.result << ", " << instr.lookupIndex << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallPropertyLookup)

        MOTH_BEGIN_INSTR(CallScopeObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]" << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallScopeObjectProperty)

        MOTH_BEGIN_INSTR(CallContextObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]" << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallContextObjectProperty)

        MOTH_BEGIN_INSTR(CallElement)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]" << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallElement)

        MOTH_BEGIN_INSTR(CallActivationProperty)
            d << instr.result << ", " << instr.name << "(" << instr.callData << ", " << instr.argc << ")";
        MOTH_END_INSTR(CallActivationProperty)

        MOTH_BEGIN_INSTR(CallGlobalLookup)
            d << instr.result << ", " << instr.index << "(" << instr.callData << ")";
        MOTH_END_INSTR(CallGlobalLookup)

        MOTH_BEGIN_INSTR(SetExceptionHandler)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(SetExceptionHandler)

        MOTH_BEGIN_INSTR(CallBuiltinThrow)
            d << instr.arg;
        MOTH_END_INSTR(CallBuiltinThrow)

        MOTH_BEGIN_INSTR(GetException)
            d << instr.result;
        MOTH_END_INSTR(HasException)

        MOTH_BEGIN_INSTR(SetException)
                d << instr.exception;
        MOTH_END_INSTR(SetExceptionFlag)

        MOTH_BEGIN_INSTR(CallBuiltinUnwindException)
            d << instr.result;
        MOTH_END_INSTR(CallBuiltinUnwindException)

        MOTH_BEGIN_INSTR(CallBuiltinPushCatchScope)
            d << instr.name;
        MOTH_END_INSTR(CallBuiltinPushCatchScope)

        MOTH_BEGIN_INSTR(CallBuiltinPushScope)
            d << instr.arg;
        MOTH_END_INSTR(CallBuiltinPushScope)

        MOTH_BEGIN_INSTR(CallBuiltinPopScope)
        MOTH_END_INSTR(CallBuiltinPopScope)

        MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
            d << instr.result;
        MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

        MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
            d << instr.result << ", " << instr.arg;
        MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

        MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
            d << instr.result << ", " << instr.base << "[" << instr.member << "]";
        MOTH_END_INSTR(CallBuiltinDeleteMember)

        MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(CallBuiltinDeleteSubscript)

        MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
            d << instr.result << ", " << instr.name;
        MOTH_END_INSTR(CallBuiltinDeleteName)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofScopeObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(CallBuiltinTypeofMember)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofContextObjectProperty)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(CallBuiltinTypeofMember)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofMember)
            d << instr.result << ", " << instr.base << "[" << instr.member << "]";
        MOTH_END_INSTR(CallBuiltinTypeofMember)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofSubscript)
            d << instr.result << ", " << instr.base << "[" << instr.index << "]";
        MOTH_END_INSTR(CallBuiltinTypeofSubscript)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
            d << instr.result << ", " << instr.name;
        MOTH_END_INSTR(CallBuiltinTypeofName)

        MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
                d << instr.result << ", " << instr.value;
        MOTH_END_INSTR(CallBuiltinTypeofValue)

        MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
            d << instr.isDeletable << ", " << instr.varName;
        MOTH_END_INSTR(CallBuiltinDeclareVar)

        MOTH_BEGIN_INSTR(CallBuiltinDefineArray)
            d << instr.result << ", " << instr.args << ", " << instr.argc;
        MOTH_END_INSTR(CallBuiltinDefineArray)

        MOTH_BEGIN_INSTR(CallBuiltinDefineObjectLiteral)
            d << instr.result << ", " << instr.args
              << ", " << instr.internalClassId
              << ", " << instr.arrayValueCount
              << ", " << instr.arrayGetterSetterCountAndFlags;
        MOTH_END_INSTR(CallBuiltinDefineObjectLiteral)

        MOTH_BEGIN_INSTR(CallBuiltinSetupArgumentsObject)
            d << instr.result;
        MOTH_END_INSTR(CallBuiltinSetupArgumentsObject)

        MOTH_BEGIN_INSTR(CallBuiltinConvertThisToObject)
        MOTH_END_INSTR(CallBuiltinConvertThisToObject)

        MOTH_BEGIN_INSTR(CreateValue)
            d << instr.result << ", new" << instr.func << "(" << instr.callData << instr.argc << ")";
        MOTH_END_INSTR(CreateValue)

        MOTH_BEGIN_INSTR(CreateProperty)
            d << instr.result << ", new" << instr.name << "(" << instr.callData << instr.argc << ")";
        MOTH_END_INSTR(CreateProperty)

        MOTH_BEGIN_INSTR(ConstructPropertyLookup)
            d << instr.result << ", new" << instr.index << "(" << instr.callData << instr.argc << ")";
        MOTH_END_INSTR(ConstructPropertyLookup)

        MOTH_BEGIN_INSTR(CreateActivationProperty)
            d << instr.result << ", new" << instr.name << "(" << instr.callData << instr.argc << ")";
        MOTH_END_INSTR(CreateActivationProperty)

        MOTH_BEGIN_INSTR(ConstructGlobalLookup)
            d << instr.result << ", new" << instr.index << "(" << instr.callData << instr.argc << ")";
        MOTH_END_INSTR(ConstructGlobalLookup)

        MOTH_BEGIN_INSTR(Jump)
            d << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(Jump)

        MOTH_BEGIN_INSTR(JumpEq)
            d << instr.condition << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpEq)

        MOTH_BEGIN_INSTR(JumpNe)
            d << instr.condition << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpNe)

        MOTH_BEGIN_INSTR(JumpStrictEqual)
            d << instr.lhs<< ", " << instr.rhs << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictEqual)

        MOTH_BEGIN_INSTR(JumpStrictNotEqual)
            d << instr.lhs<< ", " << instr.rhs << "  " << absoluteInstructionOffset(start, instr);
        MOTH_END_INSTR(JumpStrictNotEqual)

        MOTH_BEGIN_INSTR(UNot)
            d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UNot)

        MOTH_BEGIN_INSTR(UNotBool)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UNotBool)

        MOTH_BEGIN_INSTR(UPlus)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UPlus)

        MOTH_BEGIN_INSTR(UMinus)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UMinus)

        MOTH_BEGIN_INSTR(UCompl)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UCompl)

        MOTH_BEGIN_INSTR(UComplInt)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(UComplInt)

        MOTH_BEGIN_INSTR(PreIncrement)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(PreIncrement)

        MOTH_BEGIN_INSTR(PreDecrement)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(PreDecrement)

        MOTH_BEGIN_INSTR(PostIncrement)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(PostIncrement)

        MOTH_BEGIN_INSTR(PostDecrement)
                d << instr.result << ", " << instr.source;
        MOTH_END_INSTR(PostDecrement)

        MOTH_BEGIN_INSTR(Binop)
                d << instr.alu << ", " << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Binop)

        MOTH_BEGIN_INSTR(Add)
            d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Add)

        MOTH_BEGIN_INSTR(BitAnd)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOr)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXor)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(Shr)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Shr)

        MOTH_BEGIN_INSTR(Shl)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Shl)

        MOTH_BEGIN_INSTR(BitAndConst)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOrConst)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXorConst)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(ShrConst)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(ShrConst)

        MOTH_BEGIN_INSTR(ShlConst)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(ShlConst)

        MOTH_BEGIN_INSTR(Mul)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Mul)

        MOTH_BEGIN_INSTR(Sub)
                d << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(BinopContext)
                d << instr.alu << instr.result << ", " << instr.lhs << ", " << instr.rhs;
        MOTH_END_INSTR(BinopContext)

        MOTH_BEGIN_INSTR(Ret)
            d << instr.result;
        MOTH_END_INSTR(Ret)

    #ifndef QT_NO_QML_DEBUGGER
        MOTH_BEGIN_INSTR(Debug)
        MOTH_END_INSTR(Debug)

        MOTH_BEGIN_INSTR(Line)
            d << instr.lineNumber;
        MOTH_END_INSTR(Line)
    #endif // QT_NO_QML_DEBUGGER

        MOTH_BEGIN_INSTR(LoadThis)
                d << instr.result;
        MOTH_END_INSTR(LoadThis)

        MOTH_BEGIN_INSTR(LoadQmlContext)
                d << instr.result;
        MOTH_END_INSTR(LoadQmlContext)

        MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
                d << instr.result;
        MOTH_END_INSTR(LoadQmlImportedScripts)

        MOTH_BEGIN_INSTR(LoadQmlSingleton)
                d << instr.result << instr.name;
        MOTH_END_INSTR(LoadQmlSingleton)
        }

    }
}

}
}
QT_END_NAMESPACE
