/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4instruction_p.h"
#include "qv4bindings_p.h"

#include <QtCore/qdebug.h>
#include <private/qqmlglobal_p.h>

// Define this to do a test dump of all the instructions at startup.  This is 
// helpful to test that each instruction's Instr::dump() case uses the correct
// number of tabs etc and otherwise looks correct.
// #define DEBUG_INSTR_DUMP

QT_BEGIN_NAMESPACE

namespace QQmlJS {

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
        leading.append('\t');
    }

#define INSTR_DUMP qWarning().nospace() << leading.constData() 

    switch (instructionType(i)) {
    case V4Instr::Noop:
        INSTR_DUMP << '\t' << "Noop";
        break;
    case V4Instr::BindingId:
        INSTR_DUMP << i->id.line << ':' << i->id.column << ':';
        break;
    case V4Instr::SubscribeId:
        INSTR_DUMP << '\t' << "SubscribeId" << "\t\t" << "Id_Offset(" << i->subscribeop.index << ") -> Subscribe_Slot(" << i->subscribeop.offset << ')';
        break;
    case V4Instr::FetchAndSubscribe:
        INSTR_DUMP << '\t' << "FetchAndSubscribe" << '\t' << "Object_Reg(" << i->fetchAndSubscribe.reg << ") Fast_Accessor(" << i->fetchAndSubscribe.property.accessors << ") -> Output_Reg(" << i->fetchAndSubscribe.reg << ") Subscription_Slot(" <<  i->fetchAndSubscribe.subscription << ')';
        break;
    case V4Instr::LoadId:
        INSTR_DUMP << '\t' << "LoadId" << "\t\t\t" << "Id_Offset(" << i->load.index << ") -> Output_Reg(" << i->load.reg << ')';
        break;
    case V4Instr::LoadScope:
        INSTR_DUMP << '\t' << "LoadScope" << "\t\t" << "-> Output_Reg(" << i->load.reg << ')';
        break;
    case V4Instr::LoadRoot:
        INSTR_DUMP << '\t' << "LoadRoot" << "\t\t" << "-> Output_Reg(" << i->load.reg << ')';
        break;
    case V4Instr::LoadSingletonObject:
        INSTR_DUMP << '\t' << "LoadSingletonObject" << "\t\t" << ") -> Output_Reg(" << i->load.reg << ')';
        break;
    case V4Instr::LoadAttached:
        INSTR_DUMP << '\t' << "LoadAttached" << "\t\t" << "Object_Reg(" << i->attached.reg << ") Attached_Index(" << i->attached.id << ") -> Output_Reg(" << i->attached.output << ')';
        break;
    case V4Instr::UnaryNot:
        INSTR_DUMP << '\t' << "UnaryNot" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::UnaryMinusNumber:
        INSTR_DUMP << '\t' << "UnaryMinusNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::UnaryMinusInt:
        INSTR_DUMP << '\t' << "UnaryMinusInt" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::UnaryPlusNumber:
        INSTR_DUMP << '\t' << "UnaryPlusNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::UnaryPlusInt:
        INSTR_DUMP << '\t' << "UnaryPlusInt" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToInt:
        INSTR_DUMP << '\t' << "ConvertBoolToInt" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToJSValue:
        INSTR_DUMP << '\t' << "ConvertBoolToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToNumber:
        INSTR_DUMP << '\t' << "ConvertBoolToNumber" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToString:
        INSTR_DUMP << '\t' << "ConvertBoolToString" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToVariant:
        INSTR_DUMP << '\t' << "ConvertBoolToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertBoolToVar:
        INSTR_DUMP << '\t' << "ConvertBoolToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToBool:
        INSTR_DUMP << '\t' << "ConvertIntToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToJSValue:
        INSTR_DUMP << '\t' << "ConvertIntToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToNumber:
        INSTR_DUMP << '\t' << "ConvertIntToNumber" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToString:
        INSTR_DUMP << '\t' << "ConvertIntToString" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToVariant:
        INSTR_DUMP << '\t' << "ConvertIntToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertIntToVar:
        INSTR_DUMP << '\t' << "ConvertIntToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertJSValueToVar:
        INSTR_DUMP << '\t' << "ConvertJSValueToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToBool:
        INSTR_DUMP << '\t' << "ConvertNumberToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToInt:
        INSTR_DUMP << '\t' << "ConvertNumberToInt" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToJSValue:
        INSTR_DUMP << '\t' << "ConvertNumberToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToString:
        INSTR_DUMP << '\t' << "ConvertNumberToString" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToVariant:
        INSTR_DUMP << '\t' << "ConvertNumberToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNumberToVar:
        INSTR_DUMP << '\t' << "ConvertNumberToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToBool:
        INSTR_DUMP << '\t' << "ConvertStringToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToInt:
        INSTR_DUMP << '\t' << "ConvertStringToInt" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToJSValue:
        INSTR_DUMP << '\t' << "ConvertStringToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToNumber:
        INSTR_DUMP << '\t' << "ConvertStringToNumber" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToUrl:
        INSTR_DUMP << '\t' << "ConvertStringToUrl" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToColor:
        INSTR_DUMP << '\t' << "ConvertStringToColor" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToVariant:
        INSTR_DUMP << '\t' << "ConvertStringToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertStringToVar:
        INSTR_DUMP << '\t' << "ConvertStringToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertUrlToBool:
        INSTR_DUMP << '\t' << "ConvertUrlToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertUrlToJSValue:
        INSTR_DUMP << '\t' << "ConvertUrlToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertUrlToString:
        INSTR_DUMP << '\t' << "ConvertUrlToString" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertUrlToVariant:
        INSTR_DUMP << '\t' << "ConvertUrlToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertUrlToVar:
        INSTR_DUMP << '\t' << "ConvertUrlToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertColorToBool:
        INSTR_DUMP << '\t' << "ConvertColorToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertColorToJSValue:
        INSTR_DUMP << '\t' << "ConvertColorToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertColorToString:
        INSTR_DUMP << '\t' << "ConvertColorToString" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertColorToVariant:
        INSTR_DUMP << '\t' << "ConvertColorToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertColorToVar:
        INSTR_DUMP << '\t' << "ConvertColorToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertObjectToBool:
        INSTR_DUMP << '\t' << "ConvertObjectToBool" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertObjectToJSValue:
        INSTR_DUMP << '\t' << "ConvertObjectToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertObjectToVariant:
        INSTR_DUMP << '\t' << "ConvertObjectToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertObjectToVar:
        INSTR_DUMP << '\t' << "ConvertObjectToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertVarToJSValue:
        INSTR_DUMP << '\t' << "ConvertVarToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNullToJSValue:
        INSTR_DUMP << '\t' << "ConvertNullToJSValue" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNullToObject:
        INSTR_DUMP << '\t' << "ConvertNullToObject" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNullToVariant:
        INSTR_DUMP << '\t' << "ConvertNullToVariant" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ConvertNullToVar:
        INSTR_DUMP << '\t' << "ConvertNullToVar" << '\t' << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::ResolveUrl:
        INSTR_DUMP << '\t' << "ResolveUrl" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathSinNumber:
        INSTR_DUMP << '\t' << "MathSinNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathCosNumber:
        INSTR_DUMP << '\t' << "MathCosNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathAbsNumber:
        INSTR_DUMP << '\t' << "MathAbsNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathRoundNumber:
        INSTR_DUMP << '\t' << "MathRoundNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathFloorNumber:
        INSTR_DUMP << '\t' << "MathFloorNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathCeilNumber:
        INSTR_DUMP << '\t' << "MathCeilNumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::MathPINumber:
        INSTR_DUMP << '\t' << "MathPINumber" << "\t\t" << "Input_Reg(" << i->unaryop.src << ") -> Output_Reg(" << i->unaryop.output << ')';
        break;
    case V4Instr::LoadNull:
        INSTR_DUMP << '\t' << "LoadNull" << "\t\t" << "Constant(null) -> Output_Reg(" << i->null_value.reg << ')';
        break;
    case V4Instr::LoadNumber:
        INSTR_DUMP << '\t' << "LoadNumber" << "\t\t" << "Constant(" << i->number_value.value << ") -> Output_Reg(" << i->number_value.reg << ')';
        break;
    case V4Instr::LoadInt:
        INSTR_DUMP << '\t' << "LoadInt" << "\t\t\t" << "Constant(" << i->int_value.value << ") -> Output_Reg(" << i->int_value.reg << ')';
        break;
    case V4Instr::LoadBool:
        INSTR_DUMP << '\t' << "LoadBool" << "\t\t" << "Constant(" << i->bool_value.value << ") -> Output_Reg(" << i->bool_value.reg << ')';
        break;
    case V4Instr::LoadString:
        INSTR_DUMP << '\t' << "LoadString" << "\t\t" << "String_DataIndex(" << i->string_value.offset << ") String_Length(" << i->string_value.length << ") -> Output_Register(" << i->string_value.reg << ')';
        break;
    case V4Instr::EnableV4Test:
        INSTR_DUMP << '\t' << "EnableV4Test" << "\t\t" << "String_DataIndex(" << i->string_value.offset << ") String_Length(" << i->string_value.length << ')';
        break;
    case V4Instr::TestV4Store:
        INSTR_DUMP << '\t' << "TestV4Store" << "\t\t" << "Input_Reg(" << i->storetest.reg << ") Reg_Type(" << i->storetest.regType << ')';
        break;
    case V4Instr::BitAndInt:
        INSTR_DUMP << '\t' << "BitAndInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::BitOrInt:
        INSTR_DUMP << '\t' << "BitOrInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::BitXorInt:
        INSTR_DUMP << '\t' << "BitXorInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::AddNumber:
        INSTR_DUMP << '\t' << "AddNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::AddString:
        INSTR_DUMP << '\t' << "AddString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::SubNumber:
        INSTR_DUMP << '\t' << "SubNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::MulNumber:
        INSTR_DUMP << '\t' << "MulNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::DivNumber:
        INSTR_DUMP << '\t' << "DivNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::ModNumber:
        INSTR_DUMP << '\t' << "ModNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::LShiftInt:
        INSTR_DUMP << '\t' << "LShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::RShiftInt:
        INSTR_DUMP << '\t' << "RShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::URShiftInt:
        INSTR_DUMP << '\t' << "URShiftInt" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::GtNumber:
        INSTR_DUMP << '\t' << "GtNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::LtNumber:
        INSTR_DUMP << '\t' << "LtNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::GeNumber:
        INSTR_DUMP << '\t' << "GeNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::LeNumber:
        INSTR_DUMP << '\t' << "LeNumber" << "\t\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::EqualNumber:
        INSTR_DUMP << '\t' << "EqualNumber" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::NotEqualNumber:
        INSTR_DUMP << '\t' << "NotEqualNumber" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictEqualNumber:
        INSTR_DUMP << '\t' << "StrictEqualNumber" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictNotEqualNumber:
        INSTR_DUMP << '\t' << "StrictNotEqualNumber" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::GtString:
        INSTR_DUMP << '\t' << "GtString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::LtString:
        INSTR_DUMP << '\t' << "LtString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::GeString:
        INSTR_DUMP << '\t' << "GeString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::LeString:
        INSTR_DUMP << '\t' << "LeString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::EqualString:
        INSTR_DUMP << '\t' << "EqualString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::NotEqualString:
        INSTR_DUMP << '\t' << "NotEqualString" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictEqualString:
        INSTR_DUMP << '\t' << "StrictEqualString" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictNotEqualString:
        INSTR_DUMP << '\t' << "StrictNotEqualString" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::EqualObject:
        INSTR_DUMP << '\t' << "EqualObject" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::NotEqualObject:
        INSTR_DUMP << '\t' << "NotEqualObject" << "\t\t" << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictEqualObject:
        INSTR_DUMP << '\t' << "StrictEqualObject" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::StrictNotEqualObject:
        INSTR_DUMP << '\t' << "StrictNotEqualObject" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::MathMaxNumber:
        INSTR_DUMP << '\t' << "MathMaxNumber" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::MathMinNumber:
        INSTR_DUMP << '\t' << "MathMinNumber" << '\t' << "Input_Reg(" << i->binaryop.left << ") Input_Reg(" << i->binaryop.right << ") -> Output_Reg(" << i->binaryop.output << ')';
        break;
    case V4Instr::NewString:
        INSTR_DUMP << '\t' << "NewString" << "\t\t" << "Register(" << i->construct.reg << ')';
        break;
    case V4Instr::NewUrl:
        INSTR_DUMP << '\t' << "NewUrl" << "\t\t\t" << "Register(" << i->construct.reg << ')';
        break;
    case V4Instr::CleanupRegister:
        INSTR_DUMP << '\t' << "CleanupRegister" << "\t\t" << "Register(" << i->cleanup.reg << ')';
        break;
    case V4Instr::Fetch:
        INSTR_DUMP << '\t' << "Fetch" << "\t\t\t" << "Object_Reg(" << i->fetch.reg << ") Property_Index(" << i->fetch.index << ") -> Output_Reg(" << i->fetch.reg << ')';
        break;
    case V4Instr::Store:
        INSTR_DUMP << '\t' << "Store" << "\t\t\t" << "Input_Reg(" << i->store.reg << ") -> Object_Reg(" << i->store.output << ") Property_Index(" << i->store.index << ')';
        break;
    case V4Instr::Copy:
        INSTR_DUMP << '\t' << "Copy" << "\t\t\t" << "Input_Reg(" << i->copy.src << ") -> Output_Reg(" << i->copy.reg << ')';
        break;
    case V4Instr::Jump:
        if (i->jump.reg != -1) {
            INSTR_DUMP << '\t' << "Jump" << "\t\t\t" << "Address(" << (address + size() + i->jump.count) << ") [if false == Input_Reg(" << i->jump.reg << ")]";
        } else {
            INSTR_DUMP << '\t' << "Jump" << "\t\t\t" << "Address(" << (address + size() + i->jump.count) << ')';
        }
        break;
    case V4Instr::BranchFalse:
        INSTR_DUMP << '\t' << "BranchFalse" << "\t\t" << "Address(" << (address + size() + i->branchop.offset) << ") [if false == Input_Reg(" << i->branchop.reg << ")]";
        break;
    case V4Instr::BranchTrue:
        INSTR_DUMP << '\t' << "BranchTrue" << "\t\t" << "Address(" << (address + size() + i->branchop.offset) << ") [if true == Input_Reg(" << i->branchop.reg << ")]";
        break;
    case V4Instr::Branch:
        INSTR_DUMP << '\t' << "Branch" << "\t\t\t" << "Address(" << (address + size() + i->branchop.offset) << ')';
        break;
    case V4Instr::Block:
        INSTR_DUMP << '\t' << "Block" << "\t\t\t" << "Mask(" << QByteArray::number(i->blockop.block, 16).constData()  << ')';
        break;
    case V4Instr::Throw:
        INSTR_DUMP << '\t' << "Throw" << "\t\t\t" << "InputReg(" << i->throwop.message  << ')';
        break;
    default:
        INSTR_DUMP << '\t' << "Unknown";
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
