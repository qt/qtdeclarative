/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4instruction_p.h"
#include "qv4bindings_p.h"

#include <QtCore/qdebug.h>
#include <private/qdeclarativeglobal_p.h>

// Define this to do a test dump of all the instructions at startup.  This is 
// helpful to test that each instruction's Instr::dump() case uses the correct
// number of tabs etc and otherwise looks correct.
// #define DEBUG_INSTR_DUMP

QT_BEGIN_NAMESPACE

namespace QDeclarativeJS {

#ifdef DEBUG_INSTR_DUMP
static struct DumpInstrAtStartup {
    DumpInstrAtStartup() {
        Bytecode bc;
#define DUMP_INSTR_AT_STARTUP(I, FMT) { V4InstrData<V4Instr::I> i; bc.append(i); }
        FOR_EACH_V4_INSTR(DUMP_INSTR_AT_STARTUP);
#undef DUMP_INSTR_AT_STARTUP
        const char *start = bc.constData();
        const char *end = start + bc.size();
        bc.dump(start, end);
    }
} dump_instr_at_startup;
#endif

int V4Instr::size(Type type)
{
#define V4_RETURN_INSTR_SIZE(I, FMT) case I: return QML_V4_INSTR_SIZE(I, FMT);
    switch (type) {
    FOR_EACH_V4_INSTR(V4_RETURN_INSTR_SIZE)
    }
#undef V4_RETURN_INSTR_SIZE
    return 0;
}

void Bytecode::dump(const V4Instr *i, int address) const
{
    QByteArray leading;
    if (address != -1) {
        leading = QByteArray::number(address);
        leading.prepend(QByteArray(8 - leading.count(), ' '));
        leading.append("\t");
    }

#define INSTR_DUMP qWarning().nospace() << leading.constData() 

    switch (instructionType(i)) {
    case V4Instr::Noop:
        INSTR_DUMP << "\t" << "Noop";
        break;
    case V4Instr::BindingId:
        INSTR_DUMP << i->id.line << ":" << i->id.column << ":";
        break;
    case V4Instr::Subscribe:
        INSTR_DUMP << "\t" << "Subscribe" << "\t\t" << "Object_Reg(" << i->subscribeop.reg << ") Notify_Signal(" << i->subscribeop.index << ") -> Subscribe_Slot(" << i->subscribeop.offset << ")";
        break;
    case V4Instr::SubscribeId:
        INSTR_DUMP << "\t" << "SubscribeId" << "\t\t" << "Id_Offset(" << i->subscribeop.index << ") -> Subscribe_Slot(" << i->subscribeop.offset << ")";
        break;
    case V4Instr::FetchAndSubscribe:
        INSTR_DUMP << "\t" << "FetchAndSubscribe" << "\t" << "Object_Reg(" << i->fetchAndSubscribe.reg << ") Fast_Accessor(" << i->fetchAndSubscribe.property.accessors << ") -> Output_Reg(" << i->fetchAndSubscribe.reg << ") Subscription_Slot(" <<  i->fetchAndSubscribe.subscription << ")";
        break;
    case V4Instr::LoadId:
        INSTR_DUMP << "\t" << "LoadId" << "\t\t\t" << "Id_Offset(" << i->load.index << ") -> Output_Reg(" << i->load.reg << ")";
        break;
    case V4Instr::LoadScope:
        INSTR_DUMP << "\t" << "LoadScope" << "\t\t" << "-> Output_Reg(" << i->load.reg << ")";
        break;
    case V4Instr::LoadRoot:
        INSTR_DUMP << "\t" << "LoadRoot" << "\t\t" << "-> Output_Reg(" << i->load.reg << ")";
        break;
    case V4Instr::LoadAttached:
        INSTR_DUMP << "\t" << "LoadAttached" << "\t\t" << "Object_Reg(" << i->attached.reg << ") Attached_Index(" << i->attached.id << ") -> Output_Reg(" << i->attached.output << ")";
        break;
    case V4Instr::UnaryNot:
        INSTR_DUMP << "\t" << "UnaryNot" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::UnaryMinusReal:
        INSTR_DUMP << "\t" << "UnaryMinusReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::UnaryMinusInt:
        INSTR_DUMP << "\t" << "UnaryMinusInt" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::UnaryPlusReal:
        INSTR_DUMP << "\t" << "UnaryPlusReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::UnaryPlusInt:
        INSTR_DUMP << "\t" << "UnaryPlusInt" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertBoolToInt:
        INSTR_DUMP << "\t" << "ConvertBoolToInt" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertBoolToReal:
        INSTR_DUMP << "\t" << "ConvertBoolToReal" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertBoolToString:
        INSTR_DUMP << "\t" << "ConvertBoolToString" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertIntToBool:
        INSTR_DUMP << "\t" << "ConvertIntToBool" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertIntToReal:
        INSTR_DUMP << "\t" << "ConvertIntToReal" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertIntToString:
        INSTR_DUMP << "\t" << "ConvertIntToString" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertRealToBool:
        INSTR_DUMP << "\t" << "ConvertRealToBool" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertRealToInt:
        INSTR_DUMP << "\t" << "ConvertRealToInt" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertRealToString:
        INSTR_DUMP << "\t" << "ConvertRealToString" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertStringToBool:
        INSTR_DUMP << "\t" << "ConvertStringToBool" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertStringToInt:
        INSTR_DUMP << "\t" << "ConvertStringToInt" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertStringToReal:
        INSTR_DUMP << "\t" << "ConvertStringToReal" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertStringToUrl:
        INSTR_DUMP << "\t" << "ConvertStringToUrl" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertUrlToBool:
        INSTR_DUMP << "\t" << "ConvertUrlToBool" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ConvertUrlToString:
        INSTR_DUMP << "\t" << "ConvertUrlToString" << "\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::ResolveUrl:
        INSTR_DUMP << "\t" << "ResolveUrl" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::MathSinReal:
        INSTR_DUMP << "\t" << "MathSinReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::MathCosReal:
        INSTR_DUMP << "\t" << "MathCosReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::MathRoundReal:
        INSTR_DUMP << "\t" << "MathRoundReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::MathFloorReal:
        INSTR_DUMP << "\t" << "MathFloorReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::MathPIReal:
        INSTR_DUMP << "\t" << "MathPIReal" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ")";
        break;
    case V4Instr::LoadReal:
        INSTR_DUMP << "\t" << "LoadReal" << "\t\t" << "Constant(" << i->real_value.value << ") -> Output_Reg(" << i->real_value.reg << ")";
        break;
    case V4Instr::LoadInt:
        INSTR_DUMP << "\t" << "LoadInt" << "\t\t\t" << "Constant(" << i->int_value.value << ") -> Output_Reg(" << i->int_value.reg << ")";
        break;
    case V4Instr::LoadBool:
        INSTR_DUMP << "\t" << "LoadBool" << "\t\t" << "Constant(" << i->bool_value.value << ") -> Output_Reg(" << i->bool_value.reg << ")";
        break;
    case V4Instr::LoadString:
        INSTR_DUMP << "\t" << "LoadString" << "\t\t" << "String_DataIndex(" << i->string_value.offset << ") String_Length(" << i->string_value.length << ") -> Output_Register(" << i->string_value.reg << ")";
        break;
    case V4Instr::EnableV4Test:
        INSTR_DUMP << "\t" << "EnableV4Test" << "\t\t" << "String_DataIndex(" << i->string_value.offset << ") String_Length(" << i->string_value.length << ")";
        break;
    case V4Instr::TestV4Store:
        INSTR_DUMP << "\t" << "TestV4Store" << "\t\t" << "Input_Reg(" << i->storetest.reg << ") Reg_Type(" << i->storetest.regType << ")";
        break;
    case V4Instr::BitAndInt:
        INSTR_DUMP << "\t" << "BitAndInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::BitOrInt:
        INSTR_DUMP << "\t" << "BitOrInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::BitXorInt:
        INSTR_DUMP << "\t" << "BitXorInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::AddReal:
        INSTR_DUMP << "\t" << "AddReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::AddString:
        INSTR_DUMP << "\t" << "AddString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::SubReal:
        INSTR_DUMP << "\t" << "SubReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::MulReal:
        INSTR_DUMP << "\t" << "MulReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::DivReal:
        INSTR_DUMP << "\t" << "DivReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::ModReal:
        INSTR_DUMP << "\t" << "ModReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::LShiftInt:
        INSTR_DUMP << "\t" << "LShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::RShiftInt:
        INSTR_DUMP << "\t" << "RShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::URShiftInt:
        INSTR_DUMP << "\t" << "URShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::GtReal:
        INSTR_DUMP << "\t" << "GtReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::LtReal:
        INSTR_DUMP << "\t" << "LtReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::GeReal:
        INSTR_DUMP << "\t" << "GeReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::LeReal:
        INSTR_DUMP << "\t" << "LeReal" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::EqualReal:
        INSTR_DUMP << "\t" << "EqualReal" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::NotEqualReal:
        INSTR_DUMP << "\t" << "NotEqualReal" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::StrictEqualReal:
        INSTR_DUMP << "\t" << "StrictEqualReal" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::StrictNotEqualReal:
        INSTR_DUMP << "\t" << "StrictNotEqualReal" << "\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::GtString:
        INSTR_DUMP << "\t" << "GtString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::LtString:
        INSTR_DUMP << "\t" << "LtString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::GeString:
        INSTR_DUMP << "\t" << "GeString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::LeString:
        INSTR_DUMP << "\t" << "LeString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::EqualString:
        INSTR_DUMP << "\t" << "EqualString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::NotEqualString:
        INSTR_DUMP << "\t" << "NotEqualString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::StrictEqualString:
        INSTR_DUMP << "\t" << "StrictEqualString" << "\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::StrictNotEqualString:
        INSTR_DUMP << "\t" << "StrictNotEqualString" << "\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ")";
        break;
    case V4Instr::NewString:
        INSTR_DUMP << "\t" << "NewString" << "\t\t" << "Register(" << i->construct.reg << ")";
        break;
    case V4Instr::NewUrl:
        INSTR_DUMP << "\t" << "NewUrl" << "\t\t\t" << "Register(" << i->construct.reg << ")";
        break;
    case V4Instr::CleanupRegister:
        INSTR_DUMP << "\t" << "CleanupRegister" << "\t\t" << "Register(" << i->cleanup.reg << ")";
        break;
    case V4Instr::Fetch:
        INSTR_DUMP << "\t" << "Fetch" << "\t\t\t" << "Object_Reg(" << i->fetch.reg << ") Property_Index(" << i->fetch.index << ") -> Output_Reg(" << i->fetch.reg << ")";
        break;
    case V4Instr::Store:
        INSTR_DUMP << "\t" << "Store" << "\t\t\t" << "Input_Reg(" << i->store.reg << ") -> Object_Reg(" << i->store.output << ") Property_Index(" << i->store.index << ")";
        break;
    case V4Instr::Copy:
        INSTR_DUMP << "\t" << "Copy" << "\t\t\t" << "Input_Reg(" << i->copy.src << ") -> Output_Reg(" << i->copy.reg << ")";
        break;
    case V4Instr::Jump:
        if (i->jump.reg != -1) {
            INSTR_DUMP << "\t" << "Jump" << "\t\t\t" << "Address(" << (address + size() + i->jump.count) << ") [if false == Input_Reg(" << i->jump.reg << ")]";
        } else {
            INSTR_DUMP << "\t" << "Jump" << "\t\t\t" << "Address(" << (address + size() + i->jump.count) << ")";
        }
        break;
    case V4Instr::BranchFalse:
        INSTR_DUMP << "\t" << "BranchFalse" << "\t\t" << "Address(" << (address + size() + i->branchop.offset) << ") [if false == Input_Reg(" << i->branchop.reg << ")]";
        break;
    case V4Instr::BranchTrue:
        INSTR_DUMP << "\t" << "BranchTrue" << "\t\t" << "Address(" << (address + size() + i->branchop.offset) << ") [if true == Input_Reg(" << i->branchop.reg << ")]";
        break;
    case V4Instr::Branch:
        INSTR_DUMP << "\t" << "Branch" << "\t\t\t" << "Address(" << (address + size() + i->branchop.offset) << ")";
        break;
    case V4Instr::InitString:
        INSTR_DUMP << "\t" << "InitString" << "\t\t" << "String_DataIndex(" << i->initstring.dataIdx << ") -> String_Slot(" << i->initstring.offset << ")";
        break;
    case V4Instr::Block:
        INSTR_DUMP << "\t" << "Block" << "\t\t\t" << "Mask(" << QByteArray::number(i->blockop.block, 16).constData()  << ")";
        break;
    default:
        INSTR_DUMP << "\t" << "Unknown";
        break;
    }
}

void Bytecode::dump(const char *start, const char *end) const
{
    const char *code = start;
    while (code < end) {
        const V4Instr *instr = reinterpret_cast<const V4Instr *>(code);
        dump(instr, code - start);
        code += V4Instr::size(instructionType(instr));
    }
}

Bytecode::Bytecode()
{
#ifdef QML_THREADED_INTERPRETER
    decodeInstr = QV4Bindings::getDecodeInstrTable();
#endif
}

V4Instr::Type Bytecode::instructionType(const V4Instr *instr) const
{
#ifdef QML_THREADED_INTERPRETER
    void *code = instr->common.code;

#  define CHECK_V4_INSTR_CODE(I, FMT) \
    if (decodeInstr[static_cast<int>(V4Instr::I)] == code) \
        return V4Instr::I;

    FOR_EACH_V4_INSTR(CHECK_V4_INSTR_CODE)
    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid instruction address");
    return static_cast<V4Instr::Type>(0);
#  undef CHECK_V4_INSTR_CODE
#else
    return static_cast<V4Instr::Type>(instr->common.type);
#endif

}

void Bytecode::append(V4Instr::Type type, V4Instr &instr)
{
#ifdef QML_THREADED_INTERPRETER
    instr.common.code = decodeInstr[static_cast<int>(type)];
#else
    instr.common.type = type;
#endif
    d.append(reinterpret_cast<const char *>(&instr), V4Instr::size(type));
}

int Bytecode::remove(int offset)
{
    const V4Instr *instr = reinterpret_cast<const V4Instr *>(d.begin() + offset);
    const int instrSize = V4Instr::size(instructionType(instr));
    d.remove(offset, instrSize);
    return instrSize;
}

const V4Instr &Bytecode::operator[](int offset) const
{
    return *(reinterpret_cast<const V4Instr *>(d.begin() + offset));
}

V4Instr &Bytecode::operator[](int offset)
{
    return *(reinterpret_cast<V4Instr *>(d.begin() + offset));
}

}

QT_END_NAMESPACE
