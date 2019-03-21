/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QV4IR_P_H
#define QV4IR_P_H

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

#include <private/qv4function_p.h>
#include <QtCore/qjsondocument.h>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class Dumper;
class Graph;

class Node;
class NodeInfo;

class Function
{
    Q_DISABLE_COPY_MOVE(Function)
public:
    Function(QV4::Function *qv4Function);
    ~Function();

    void verify() const;

    QV4::Function *v4Function() const
    { return qv4Function; }

    QString name() const;

    QQmlJS::MemoryPool *pool()
    { return &m_pool; }

    Graph *graph() const
    { return m_graph; }

    void dump(const QString &description) const;
    void dump() const; // for calling in the debugger
    Dumper *dumper() const;

    using StringId = size_t;
    StringId addString(const QString &s);
    QString string(StringId id) const
    { return m_stringPool[id]; }

    NodeInfo *nodeInfo(Node *n, bool createIfNecessary = true) const;
    void copyBytecodeOffsets(Node *from, Node *to);

    void addUnwindLabelOffset(int absoluteOffset)
    { m_unwindLabelOffsets.push_back(absoluteOffset); }

    const std::vector<int> &unwindLabelOffsets() const
    { return m_unwindLabelOffsets; }

private:
    QV4::Function *qv4Function;
    mutable QQmlJS::MemoryPool m_pool;
    Graph *m_graph;
    mutable Dumper *m_dumper;
    std::vector<QString> m_stringPool;
    mutable std::vector<NodeInfo *> m_nodeInfo; //### move the into the _pool
    std::vector<int> m_unwindLabelOffsets;
};

class Dumper
{
    Q_DISABLE_COPY_MOVE(Dumper)

public:
    Dumper(const Function *f);
    ~Dumper() = default;

    static void dump(const Function *f, const QString &description);
    static void dot(const Function *f, const QString &description);

private:
    QByteArray dump(const Function *f);
    QJsonValue dump(const Node *node, const Function *f);

private:
    QJsonDocument m_doc;
};

class Type
{
    // None is for nodes with no type (e.g. a Return)
    // The others form a lattice:
    // Any -> Object                       -> Invalid
    // ^^^ -> Number -> Integral -> Int32  -> ^^^^^^^
    // ^^^ -> Number -> Integral -> UInt32 -> ^^^^^^^
    // ^^^ -> Number -> Integral -> Bool   -> ^^^^^^^
    // ^^^ -> Number -> Double             -> ^^^^^^^
    // ^^^ -> Undefined                    -> ^^^^^^^
    // ^^^ -> Null                         -> ^^^^^^^
    // ^^^ -> Empty                        -> ^^^^^^^
    enum InternalType: int16_t {
        None = 0,

        Object     = 1 << 0,
        Bool       = 1 << 1,
        Int32      = 1 << 2,
        UInt32     = 1 << 3,
        Double     = 1 << 4,
        Undefined  = 1 << 5,
        Null       = 1 << 6,
        Empty      = 1 << 7,
        RawPointer = 1 << 8,
        Invalid    = -1,

        Integral = Int32 | UInt32 | Bool,
        Number = Integral | Double,
        Any = Object | Number | Undefined | Empty | Null,
    };

    Type(InternalType t) : m_t(t) {}

public:
    Type() = default;

    bool operator==(const Type &other) const
    { return m_t == other.m_t; }

    static Type noneType() { return Type(None); }
    static Type anyType() { return Type(Any); }
    static Type undefinedType() { return Type(Undefined); }
    static Type emptyType() { return Type(Empty); }
    static Type booleanType() { return Type(Bool); }
    static Type int32Type() { return Type(Int32); }
    static Type doubleType() { return Type(Double); }
    static Type numberType() { return Type(Number); }
    static Type nullType() { return Type(Null); }
    static Type objectType() { return Type(Object); }
    static Type rawPointerType() { return Type(RawPointer); }

    bool isAny() const { return m_t == Any; }
    bool isBoolean() const { return m_t == Bool; }
    bool isInt32() const { return m_t == Int32; }
    bool isInvalid() const { return m_t == Invalid; }
    bool isNone() const { return m_t == None; }
    bool isDouble() const { return m_t == Double; }
    bool isUndefined() const { return m_t == Undefined; }
    bool isNull() const { return m_t == Null; }
    bool isEmpty() const { return m_t == Empty; }
    bool isObject() const { return m_t == Object; }
    bool isRawPointer() const { return m_t == RawPointer; }
    bool isIntegral() const { return matches(Integral); }
    bool isNumber() const { return matches(Number); }

    Type operator|(Type other) const
    { return Type(InternalType(int16_t(m_t) | int16_t(other.m_t))); }

    Type &operator|=(Type other)
    {
        m_t = (InternalType(int16_t(m_t) | int16_t(other.m_t)));
        return *this;
    }

    QString debugString() const;

private:
    bool matches(InternalType it) const
    {
        return (m_t & ~it) == 0 && (m_t & it) != 0;
    }

private:
    InternalType m_t = None;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4IR_P_H
