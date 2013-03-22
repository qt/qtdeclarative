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

#ifndef QV4INSTRUCTION_P_H
#define QV4INSTRUCTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvector.h>
#include <QtCore/qvarlengtharray.h>

#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE

#define FOR_EACH_V4_INSTR(F) \
    F(Noop, common) \
    F(BindingId, id) \
    F(SubscribeId, subscribeop) \
    F(FetchAndSubscribe, fetchAndSubscribe) \
    F(LoadId, load) \
    F(LoadScope, load) \
    F(LoadRoot, load) \
    F(LoadSingletonObject, load) \
    F(LoadAttached, attached) \
    F(UnaryNot, unaryop) \
    F(UnaryMinusNumber, unaryop) \
    F(UnaryMinusInt, unaryop) \
    F(UnaryPlusNumber, unaryop) \
    F(UnaryPlusInt, unaryop) \
    F(ConvertBoolToInt, unaryop) \
    F(ConvertBoolToJSValue, unaryop) \
    F(ConvertBoolToNumber, unaryop) \
    F(ConvertBoolToString, unaryop) \
    F(ConvertBoolToVariant, unaryop) \
    F(ConvertBoolToVar, unaryop) \
    F(ConvertIntToBool, unaryop) \
    F(ConvertIntToJSValue, unaryop) \
    F(ConvertIntToNumber, unaryop) \
    F(ConvertIntToString, unaryop) \
    F(ConvertIntToVariant, unaryop) \
    F(ConvertIntToVar, unaryop) \
    F(ConvertJSValueToVar, unaryop) \
    F(ConvertNumberToBool, unaryop) \
    F(ConvertNumberToInt, unaryop) \
    F(ConvertNumberToJSValue, unaryop) \
    F(ConvertNumberToString, unaryop) \
    F(ConvertNumberToVariant, unaryop) \
    F(ConvertNumberToVar, unaryop) \
    F(ConvertStringToBool, unaryop) \
    F(ConvertStringToInt, unaryop) \
    F(ConvertStringToJSValue, unaryop) \
    F(ConvertStringToNumber, unaryop) \
    F(ConvertStringToUrl, unaryop) \
    F(ConvertStringToColor, unaryop) \
    F(ConvertStringToVariant, unaryop) \
    F(ConvertStringToVar, unaryop) \
    F(ConvertUrlToBool, unaryop) \
    F(ConvertUrlToJSValue, unaryop) \
    F(ConvertUrlToString, unaryop) \
    F(ConvertUrlToVariant, unaryop) \
    F(ConvertUrlToVar, unaryop) \
    F(ConvertColorToBool, unaryop) \
    F(ConvertColorToJSValue, unaryop) \
    F(ConvertColorToString, unaryop) \
    F(ConvertColorToVariant, unaryop) \
    F(ConvertColorToVar, unaryop) \
    F(ConvertObjectToBool, unaryop) \
    F(ConvertObjectToJSValue, unaryop) \
    F(ConvertObjectToVariant, unaryop) \
    F(ConvertObjectToVar, unaryop) \
    F(ConvertVarToJSValue, unaryop) \
    F(ConvertNullToJSValue, unaryop) \
    F(ConvertNullToObject, unaryop) \
    F(ConvertNullToVariant, unaryop) \
    F(ConvertNullToVar, unaryop) \
    F(ResolveUrl, unaryop) \
    F(MathSinNumber, unaryop) \
    F(MathCosNumber, unaryop) \
    F(MathAbsNumber, unaryop) \
    F(MathRoundNumber, unaryop) \
    F(MathFloorNumber, unaryop) \
    F(MathCeilNumber, unaryop) \
    F(MathPINumber, unaryop) \
    F(LoadNull, null_value) \
    F(LoadNumber, number_value) \
    F(LoadInt, int_value) \
    F(LoadBool, bool_value) \
    F(LoadString, string_value) \
    F(EnableV4Test, string_value) \
    F(TestV4Store, storetest) \
    F(BitAndInt, binaryop) \
    F(BitOrInt, binaryop) \
    F(BitXorInt, binaryop) \
    F(AddNumber, binaryop) \
    F(AddString, binaryop) \
    F(SubNumber, binaryop) \
    F(MulNumber, binaryop) \
    F(DivNumber, binaryop) \
    F(ModNumber, binaryop) \
    F(LShiftInt, binaryop) \
    F(RShiftInt, binaryop) \
    F(URShiftInt, binaryop) \
    F(GtNumber, binaryop) \
    F(LtNumber, binaryop) \
    F(GeNumber, binaryop) \
    F(LeNumber, binaryop) \
    F(EqualNumber, binaryop) \
    F(NotEqualNumber, binaryop) \
    F(StrictEqualNumber, binaryop) \
    F(StrictNotEqualNumber, binaryop) \
    F(GtString, binaryop) \
    F(LtString, binaryop) \
    F(GeString, binaryop) \
    F(LeString, binaryop) \
    F(EqualString, binaryop) \
    F(NotEqualString, binaryop) \
    F(StrictEqualString, binaryop) \
    F(StrictNotEqualString, binaryop) \
    F(EqualObject, binaryop) \
    F(NotEqualObject, binaryop) \
    F(StrictEqualObject, binaryop) \
    F(StrictNotEqualObject, binaryop) \
    F(MathMaxNumber, binaryop) \
    F(MathMinNumber, binaryop) \
    F(NewString, construct) \
    F(NewUrl, construct) \
    F(CleanupRegister, cleanup) \
    F(Copy, copy) \
    F(Fetch, fetch) \
    F(Store, store) \
    F(Jump, jump) \
    F(BranchTrue, branchop) \
    F(BranchFalse, branchop) \
    F(Branch, branchop) \
    F(Block, blockop) \
    F(Throw, throwop)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define QML_THREADED_INTERPRETER
#endif

#ifdef Q_ALIGNOF
#  define QML_V4_INSTR_ALIGN_MASK (Q_ALIGNOF(V4Instr) - 1)
#else
#  define QML_V4_INSTR_ALIGN_MASK (sizeof(void *) - 1)
#endif

#define QML_V4_INSTR_ENUM(I, FMT) I,
#define QML_V4_INSTR_ADDR(I, FMT) &&op_##I,
#define QML_V4_INSTR_SIZE(I, FMT) ((sizeof(V4Instr::instr_##FMT) + QML_V4_INSTR_ALIGN_MASK) & ~QML_V4_INSTR_ALIGN_MASK)

#ifdef QML_THREADED_INTERPRETER
#  define QML_V4_BEGIN_INSTR(I,FMT) op_##I:
#  define QML_V4_END_INSTR(I,FMT) code += QML_V4_INSTR_SIZE(I, FMT); instr = (const V4Instr *) code; goto *instr->common.code;
#  define QML_V4_INSTR_HEADER void *code;
#else
#  define QML_V4_BEGIN_INSTR(I,FMT) case V4Instr::I:
#  define QML_V4_END_INSTR(I,FMT) code += QML_V4_INSTR_SIZE(I, FMT); instr = (const V4Instr *) code; break;
#  define QML_V4_INSTR_HEADER quint8 type;
#endif

class QObject;
class QQmlNotifier;

namespace QQmlJS {

union Q_AUTOTEST_EXPORT V4Instr {
    enum Type {
        FOR_EACH_V4_INSTR(QML_V4_INSTR_ENUM)
    };

    static int size(Type type);

    struct instr_common {
        QML_V4_INSTR_HEADER
    };

    struct instr_id {
        QML_V4_INSTR_HEADER
        quint16 column;
        quint16 line;
    };

    struct instr_init {
        QML_V4_INSTR_HEADER
        quint16 subscriptions;
        quint16 identifiers;
    };

    struct instr_subscribeop {
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint16 offset;
        quint32 index;
    };

    struct instr_load {
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint32 index;
    };

    struct instr_attached {
        QML_V4_INSTR_HEADER
        qint8 output;
        qint8 reg;
        quint8 exceptionId;
        quint32 id;
    };

    struct instr_store {
        QML_V4_INSTR_HEADER
        qint8 output;
        qint8 reg;
        quint8 exceptionId;
        quint8 valueType;
        quint32 index;
    };

    struct instr_storetest {
        QML_V4_INSTR_HEADER
        qint8 reg;
        qint32 regType;
    };

    struct instr_fetchAndSubscribe {
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint8 exceptionId;
        quint8 valueType;
        quint16 subscription;
        QQmlPropertyRawData property;
    };

    struct instr_fetch{
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint8 exceptionId;
        quint8 valueType;
        quint32 index;
        quint16 subOffset;
        quint32 subIndex;
    };

    struct instr_copy {
        QML_V4_INSTR_HEADER
        qint8 reg;
        qint8 src;
    };

    struct instr_construct {
        QML_V4_INSTR_HEADER
        qint8 reg;
    };

    struct instr_null_value {
        QML_V4_INSTR_HEADER
        qint8 reg;
    };

    struct instr_number_value {
        QML_V4_INSTR_HEADER
        qint8 reg;
        double value; // XXX Makes the instruction 12 bytes
    };

    struct instr_int_value {
        QML_V4_INSTR_HEADER
        qint8 reg;
        int value;
    };

    struct instr_bool_value {
        QML_V4_INSTR_HEADER
        qint8 reg;
        bool value;
    };

    struct instr_string_value {
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint16 length;
        quint32 offset;
    };

    struct instr_binaryop {
        QML_V4_INSTR_HEADER
        qint8 output;
        qint8 left;
        qint8 right;
    };

    struct instr_unaryop {
        QML_V4_INSTR_HEADER
        qint8 output;
        qint8 src;
    };

    struct instr_jump {
        QML_V4_INSTR_HEADER
        qint8 reg;
        quint32 count;
    };

    struct instr_find {
        QML_V4_INSTR_HEADER
        qint8 reg;
        qint8 src;
        quint8 exceptionId;
        quint16 name;
        quint16 subscribeIndex;
    };

    struct instr_cleanup {
        QML_V4_INSTR_HEADER
        qint8 reg;
    };

    struct instr_initstring {
        QML_V4_INSTR_HEADER
        quint16 offset;
        quint32 dataIdx;
    };

    struct instr_branchop {
        QML_V4_INSTR_HEADER
        quint8 reg;
        qint16 offset;
    };

    struct instr_blockop {
        QML_V4_INSTR_HEADER
        quint32 block;
    };

    struct instr_throwop {
        QML_V4_INSTR_HEADER
        quint8 exceptionId;
        quint32 message;
    };

    instr_common common;
    instr_id id;
    instr_init init;
    instr_subscribeop subscribeop;
    instr_load load;
    instr_attached attached;
    instr_store store;
    instr_storetest storetest;
    instr_fetchAndSubscribe fetchAndSubscribe;
    instr_fetch fetch;
    instr_copy copy;
    instr_construct construct;
    instr_null_value null_value;
    instr_number_value number_value;
    instr_int_value int_value;
    instr_bool_value bool_value;
    instr_string_value string_value;
    instr_binaryop binaryop;
    instr_unaryop unaryop;
    instr_jump jump;
    instr_find find;
    instr_cleanup cleanup;
    instr_initstring initstring;
    instr_branchop branchop;
    instr_blockop blockop;
    instr_throwop throwop;
};

template<int N>
struct V4InstrMeta {
};

#define QML_V4_INSTR_META_TEMPLATE(I, FMT) \
    template<> struct V4InstrMeta<(int)V4Instr::I> { \
        enum { Size = QML_V4_INSTR_SIZE(I, FMT) }; \
        typedef V4Instr::instr_##FMT DataType; \
        static const DataType &data(const V4Instr &instr) { return instr.FMT; } \
        static void setData(V4Instr &instr, const DataType &v) { instr.FMT = v; } \
    };
FOR_EACH_V4_INSTR(QML_V4_INSTR_META_TEMPLATE);
#undef QML_V4_INSTR_META_TEMPLATE

template<int Instr>
class Q_AUTOTEST_EXPORT V4InstrData : public V4InstrMeta<Instr>::DataType
{
};

class Q_AUTOTEST_EXPORT Bytecode
{
    Q_DISABLE_COPY(Bytecode)

public:
    Bytecode();

    const char *constData() const { return d.constData(); }
    int size() const { return d.size(); }
    int count() const { return d.count(); }
    void clear() { d.clear(); }
    bool isEmpty() const { return d.isEmpty(); }
    V4Instr::Type instructionType(const V4Instr *instr) const;

    template <int Instr>
    void append(const V4InstrData<Instr> &data)
    {
        V4Instr genericInstr;
        V4InstrMeta<Instr>::setData(genericInstr, data);
        return append(static_cast<V4Instr::Type>(Instr), genericInstr);
    }
    void append(V4Instr::Type type, V4Instr &instr);

    int remove(int index);

    const V4Instr &operator[](int offset) const;
    V4Instr &operator[](int offset);

    void dump(const char *start, const char *end) const;

private:
    void dump(const V4Instr *instr, int = -1) const;

    QVarLengthArray<char, 4 * 1024> d;
#ifdef QML_THREADED_INTERPRETER
    void **decodeInstr;
#endif
};

}

QT_END_NAMESPACE

#endif // QV4INSTRUCTION_P_H

