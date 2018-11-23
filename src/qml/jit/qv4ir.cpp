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

#include <private/qqmlglobal_p.h>
#include "qv4ir_p.h"
#include "qv4node_p.h"
#include "qv4function_p.h"
#include <qv4graph_p.h>
#include "qv4stackframe_p.h"
#include "qv4operation_p.h"
#include "qv4util_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcJsonIR, "qt.v4.ir.json");
Q_LOGGING_CATEGORY(lcDotIR, "qt.v4.ir.dot");
Q_LOGGING_CATEGORY(lcVerify, "qt.v4.ir.verify");

Function::Function(QV4::Function *qv4Function)
    : qv4Function(qv4Function)
    , m_graph(Graph::create(this))
    , m_dumper(nullptr)
    , m_nodeInfo(128, nullptr)
{
}

Function::~Function()
{
    delete m_dumper;
}

QString Function::name() const
{
    QString name;
    if (auto n = v4Function()->name())
        name = n->toQString();
    if (name.isEmpty())
        name.sprintf("%p", static_cast<void *>(v4Function()));
    auto loc = v4Function()->sourceLocation();
    return name + QStringLiteral(" (%1:%2:%3)").arg(loc.sourceFile, QString::number(loc.line),
                                                    QString::number(loc.column));
}

void Function::dump(const QString &description) const
{
    Dumper::dump(this, description);
}

void Function::dump() const
{
    dump(QStringLiteral("Debug:"));
}

Dumper *Function::dumper() const
{
    if (!m_dumper)
        m_dumper = new Dumper(this);
    return m_dumper;
}

Function::StringId Function::addString(const QString &s)
{
    m_stringPool.push_back(s);
    return m_stringPool.size() - 1;
}

NodeInfo *Function::nodeInfo(Node *n, bool createIfNecessary) const
{
    if (n->id() >= m_nodeInfo.size())
        m_nodeInfo.resize(n->id() * 2, nullptr);

    NodeInfo *&info = m_nodeInfo[n->id()];
    if (info == nullptr && createIfNecessary) {
        info = m_pool.New<NodeInfo>();
        info->setType(n->operation()->type());
    }
    return info;
}

void Function::copyBytecodeOffsets(Node *from, Node *to)
{
    auto toInfo = nodeInfo(to);
    if (auto fromInfo = nodeInfo(from)) {
        toInfo->setBytecodeOffsets(fromInfo->currentInstructionOffset(),
                                   fromInfo->nextInstructionOffset());
    }
}

Dumper::Dumper(const Function *f)
{
    if (!f)
        return;
}

void Dumper::dump(const Function *f, const QString &description)
{
    if (false && lcJsonIR().isDebugEnabled()) {
        Dumper *dumper = f->dumper();

        qCDebug(lcJsonIR).noquote().nospace() << description + QLatin1String(":\n");
        for (const auto &line : dumper->dump(f).split('\n'))
            qCDebug(lcJsonIR).noquote().nospace() << line;
    }

    if (lcDotIR().isDebugEnabled())
        dot(f, description);
}

QByteArray Dumper::dump(const Function *f)
{
    QJsonObject fo;

    {
        QString name;
        if (auto n = f->v4Function()->name())
            name = n->toQString();
        fo[QLatin1String("_searchKey")] = QStringLiteral("function %1").arg(name);
        if (name.isEmpty())
            name.sprintf("%p", static_cast<void *>(f->v4Function()));
        fo[QLatin1String("name")] = name;
    }

    auto loc = f->v4Function()->sourceLocation();
    fo[QLatin1String("source")] = loc.sourceFile;
    fo[QLatin1String("line")] = loc.line;
    fo[QLatin1String("column")] = loc.column;

    {
        QJsonArray gn;
        QJsonArray ge;
        NodeCollector nodes(f->graph(), /*collectUses =*/ true);
        nodes.sortById();
        for (Node *n : nodes.reachable()) {
            gn.append(dump(n, f));
            int inputIndex = 0;
            for (Node *input : n->inputs()) {
                QJsonObject edge;
                edge[QLatin1String("from")] = int(input->id());
                edge[QLatin1String("to")] = int(n->id());
                edge[QLatin1String("index")] = inputIndex;
                if (inputIndex < n->operation()->valueInputCount()) {
                    edge[QLatin1String("type")] = QLatin1String("value");
                } else if (inputIndex < n->operation()->valueInputCount()
                           + n->operation()->effectInputCount()) {
                    edge[QLatin1String("type")] = QLatin1String("effect");
                } else {
                    edge[QLatin1String("type")] = QLatin1String("control");
                }
                Q_ASSERT(inputIndex < n->operation()->valueInputCount()
                         + n->operation()->effectInputCount()
                         + n->operation()->controlInputCount());
                ge.append(edge);
                ++inputIndex;
            }
        }
        QJsonObject g;
        g[QLatin1String("nodes")] = gn;
        g[QLatin1String("edges")] = ge;
        fo[QLatin1String("graph")] = g;
    }

    m_doc.setObject(fo);
    return m_doc.toJson(QJsonDocument::Indented);
}

QJsonValue toJSonValue(QV4::Value v)
{
    switch (v.type()) {
    case QV4::Value::Undefined_Type: return QJsonValue(QJsonValue::Undefined);
    case QV4::Value::Null_Type: return QJsonValue(QJsonValue::Null);
    case QV4::Value::Boolean_Type: return QJsonValue(v.booleanValue());
    case QV4::Value::Integer_Type: return QJsonValue(v.int_32());
    case QV4::Value::Managed_Type:
        if (String *s = v.stringValue())
            return QJsonValue(s->toQString());
        else
            return QJsonValue(QLatin1String("<managed>"));
    default: return QJsonValue(v.doubleValue());
    }
}

QJsonValue Dumper::dump(const Node * const node, const Function *f)
{
    QJsonObject n;
    n[QLatin1String("id")] = int(node->id());
    n[QLatin1String("kind")] = node->operation()->debugString();
    switch (node->operation()->kind()) {
    case Meta::Parameter: {
        auto info = ParameterPayload::get(*node->operation());
        n[QLatin1String("name")] = f->string(info->stringId());
        n[QLatin1String("index")] = int(info->parameterIndex());
        break;
    }
    case Meta::Constant: {
        auto info = ConstantPayload::get(*node->operation());
        n[QLatin1String("value")] = toJSonValue(info->value());
        break;
    }
    default:
        break;
    }
    return n;
}

void Dumper::dot(const Function *f, const QString &description)
{
    static const bool skipFramestate = qEnvironmentVariableIsSet("QV4_JIT_DOT_SKIP_FRAMESTATE");

    auto node = [](Node *n) {
        return QStringLiteral("n%1[label=\"%1: %2%3\"];\n").arg(QString::number(n->id()),
                                                                n->operation()->debugString(),
                                                                n->isDead() ? QStringLiteral(" (dead)")
                                                                            : QString());
    };

    Graph *g = f->graph();
    QString out;
    out += QLatin1Char('\n');
    out += QStringLiteral("digraph{root=\"n%1\" label=\"%2\";"
                          "node[shape=rect];"
                          "edge[dir=back fontsize=10];\n")
            .arg(g->startNode()->id())
            .arg(description);
    out += node(g->startNode());
    const bool dumpUses = false; // set to true to see all nodes
    NodeCollector nodes(g, dumpUses, skipFramestate);
    for (Node *n : nodes.reachable()) {
        if (n == g->startNode())
            continue;

        out += node(n);

        unsigned inputIndex = 0;
        for (Node *input : n->inputs()) {
            if (input == nullptr)
                continue;
            out += QStringLiteral("n%2->n%1[style=").arg(QString::number(n->id()),
                                                         QString::number(input->id()));
            if (inputIndex < n->operation()->valueInputCount() ||
                    inputIndex == n->operation()->indexOfFrameStateInput()) {
                out += QStringLiteral("solid headlabel=\"%1\"").arg(inputIndex);
            } else if (inputIndex < unsigned(n->operation()->valueInputCount()
                                             + n->operation()->effectInputCount())) {
                out += QStringLiteral("dotted headlabel=\"%1\"").arg(inputIndex);
            } else {
                out += QStringLiteral("dashed headlabel=\"%1\"").arg(inputIndex);
            }
            out += QStringLiteral("];\n");
            ++inputIndex;
        }
    }
    out += QStringLiteral("}\n");
    qCDebug(lcDotIR).nospace().noquote() << out;

    QFile of(description + QStringLiteral(".dot"));
    of.open(QIODevice::WriteOnly);
    of.write(out.toUtf8());
    of.close();
}

void Function::verify() const
{
#ifndef QT_NO_DEBUG
    unsigned problemsFound = 0;

    auto verifyNodeAgainstOperation = [&problemsFound](const Node *n) {
        const Operation *op = n->operation();
        if (op->totalInputCount() != n->inputCount()) {
            ++problemsFound;
            qCDebug(lcVerify()) << "Node" << n->id() << "has" << n->inputCount()
                                << "inputs, but it's operation" << op->debugString()
                                << "requires" << op->totalInputCount() << "inputs";
        }

        if (n->opcode() == Meta::Phi || n->opcode() == Meta::EffectPhi) {
            if (n->controlInput()->opcode() != Meta::Region) {
                ++problemsFound;
                qCDebug(lcVerify()) << "Control input of phi node" << n->id() << "is not a region";
            }
            if (n->controlInput()->inputCount() + 1 != n->inputCount()) {
                ++problemsFound;
                qCDebug(lcVerify()) << "Control input of phi node" << n->id()
                                    << "has" << n->controlInput()->inputCount()
                                    << "inputs while phi node has" << n->inputCount()
                                    << "inputs";
            }
        }

        //### todo: verify outputs: value outputs are allowed to be unused, but the effect and
        //          control outputs have to be linked up, except:
        //### todo: verify if no use is a nullptr, except for operations that can throw, where the
        //          last one is allowed to be a nullptr when an unwind handler is missing.
    };

    NodeWorkList todo(graph());
    todo.enqueue(graph()->endNode());
    while (Node *n = todo.dequeueNextNodeForVisiting()) {
        todo.enqueueAllInputs(n);
        todo.enqueueAllUses(n);

        verifyNodeAgainstOperation(n);
    }
    //### TODO:
    if (problemsFound != 0) {
        dump(QStringLiteral("Problematic graph"));
        qFatal("Found %u problems during graph verification!", problemsFound);
    }
#endif // QT_NO_xDEBUG
}

QString Type::debugString() const
{
    if (isNone())
        return QStringLiteral("none");
    if (isInvalid())
        return QStringLiteral("invalid");

    QStringList s;
    if (m_t & Bool)
        s += QStringLiteral("boolean");
    if (m_t & Int32)
        s += QStringLiteral("int32");
    if (m_t & Double)
        s += QStringLiteral("double");
    if (m_t & Undefined)
        s += QStringLiteral("undefined");
    if (m_t & Null)
        s += QStringLiteral("null");
    if (m_t & Empty)
        s += QStringLiteral("empty");
    if (m_t & RawPointer)
        s += QStringLiteral("raw pointer");

    return s.join(QLatin1String(" "));
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
