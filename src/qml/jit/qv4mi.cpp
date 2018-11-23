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

#include <QtCore/qloggingcategory.h>
#include <private/qqmlglobal_p.h>

#include "qv4mi_p.h"
#include "qv4node_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcMI, "qt.v4.ir.mi")

QString MIOperand::debugString() const
{
    switch (kind()) {
    case Invalid: return QStringLiteral("<<INVALID>>");
    case Constant: return ConstantPayload::debugString(constantValue());
    case VirtualRegister: return QStringLiteral("vreg%1").arg(virtualRegister());
    case EngineRegister: return QStringLiteral("engine");
    case CppFrameRegister: return QStringLiteral("cppFrame");
    case Function: return QStringLiteral("function");
    case JSStackSlot: return QStringLiteral("jsstack[%1]").arg(stackSlot());
    case BoolStackSlot: return QStringLiteral("bstack[%1]").arg(stackSlot());
    case JumpTarget: return targetBlock() ? QStringLiteral("L%1").arg(targetBlock()->index())
                                          : QStringLiteral("<<INVALID BLOCK>>");
    default: Q_UNREACHABLE();
    }
}

MIInstr *MIInstr::create(QQmlJS::MemoryPool *pool, Node *irNode, unsigned nOperands)
{
    return pool->New<MIInstr>(irNode, pool, nOperands);
}

static QString commentIndent(const QString &line)
{
    int spacing = std::max(70 - line.length(), 1);
    return line + QString(spacing, QLatin1Char(' '));
}

static QString indent(int nr)
{
    QString s = nr == -1 ? QString() : QString::number(nr);
    int padding = 6 - s.size();
    if (padding > 0)
        s = QString(padding, QLatin1Char(' ')) + s;
    return s;
}

MIFunction::MIFunction(Function *irFunction)
    : m_irFunction(irFunction)
{}

void MIFunction::renumberBlocks()
{
    for (size_t i = 0, ei = m_blocks.size(); i != ei; ++i) {
        MIBlock *b = m_blocks[i];
        b->setIndex(unsigned(i));
    }
}

void MIFunction::renumberInstructions()
{
    int pos = 0;
    for (MIBlock *b : m_blocks) {
        for (MIInstr &instr : b->instructions()) {
            pos += 2;
            instr.setPosition(pos);
        }
    }
}

void MIFunction::dump(const QString &description) const
{
    if (!lcMI().isDebugEnabled())
        return;

    QString s = description + QLatin1String(":\n");
    QString name;
    if (auto n = irFunction()->v4Function()->name())
        name = n->toQString();
    if (name.isEmpty())
        QString::asprintf("%p", static_cast<void *>(irFunction()->v4Function()));
    QString line = QStringLiteral("function %1 {").arg(name);
    auto loc = irFunction()->v4Function()->sourceLocation();
    s += commentIndent(line) + QStringLiteral("; %1:%2:%3\n").arg(loc.sourceFile,
                                                                  QString::number(loc.line),
                                                                  QString::number(loc.column));
    for (const MIBlock *b : blocks()) {
        line = QStringLiteral("L%1").arg(b->index());
        bool first = true;
        if (!b->arguments().empty()) {
            line += QLatin1Char('(');
            for (const MIOperand &arg : b->arguments()) {
                if (first)
                    first = false;
                else
                    line += QStringLiteral(", ");
                line += arg.debugString();
            }
            line += QLatin1Char(')');
        }
        line += QLatin1Char(':');
        line = commentIndent(line) + QStringLiteral("; preds: ");
        if (b->inEdges().isEmpty()) {
            line += QStringLiteral("<none>");
        } else {
            bool first = true;
            for (MIBlock *in : b->inEdges()) {
                if (first)
                    first = false;
                else
                    line += QStringLiteral(", ");
                line += QStringLiteral("L%1").arg(in->index());
            }
        }
        s += line + QLatin1Char('\n');
        for (const MIInstr &i : b->instructions()) {
            line = indent(i.position()) + QLatin1String(": ");
            if (i.hasDestination())
                line += i.destination().debugString() + QStringLiteral(" = ");
            line += i.irNode()->operation()->debugString();
            bool first = true;
            for (const MIOperand &op : i.operands()) {
                if (first)
                    first = false;
                else
                    line += QLatin1Char(',');
                line += QLatin1Char(' ') + op.debugString();
            }
            line = commentIndent(line) + QStringLiteral("; node-id: %1").arg(i.irNode()->id());
            if (i.irNode()->operation()->needsBytecodeOffsets())
                line += QStringLiteral(", bytecode-offset: %1").arg(irFunction()->nodeInfo(i.irNode())->currentInstructionOffset());
            s += line + QLatin1Char('\n');
        }
        s += commentIndent(QString()) + QStringLiteral("; succs: ");
        if (b->outEdges().isEmpty()) {
            s += QStringLiteral("<none>");
        } else {
            bool first = true;
            for (MIBlock *succ : b->outEdges()) {
                if (first)
                    first = false;
                else
                    s += QStringLiteral(", ");
                s += QStringLiteral("L%1").arg(succ->index());
            }
        }
        s += QLatin1Char('\n');
    }
    s += QLatin1Char('}');

    for (const QStringRef &line : s.splitRef('\n'))
        qCDebug(lcMI).noquote().nospace() << line;
}

unsigned MIFunction::extraJSSlots() const
{
    uint interpreterFrameSize = CppStackFrame::requiredJSStackFrameSize(irFunction()->v4Function());
    if (m_jsSlotCount <= interpreterFrameSize)
        return 0;
    return m_jsSlotCount - interpreterFrameSize;
}

void MIFunction::setStartBlock(MIBlock *newStartBlock)
{
    auto it = std::find(m_blocks.begin(), m_blocks.end(), newStartBlock);
    Q_ASSERT(it != m_blocks.end());
    std::swap(*m_blocks.begin(), *it);
}

void MIFunction::setStackSlotCounts(unsigned dword, unsigned qword, unsigned js)
{
    m_vregCount = 0;
    m_dwordSlotCount = dword;
    m_qwordSlotCount = qword;
    m_jsSlotCount = js;
}

void MIFunction::verifyCFG() const
{
    if (block(MIFunction::StartBlockIndex)->instructions().front().opcode() != Meta::Start)
        qFatal("MIFunction block 0 is not the start block");

    for (MIBlock *b : m_blocks) {
        for (MIBlock *in : b->inEdges()) {
            if (!in->outEdges().contains(b))
                qFatal("block %u has incoming edge from block %u, "
                       "but does not appear in that block's outgoing edges",
                       b->index(), in->index());
        }
        for (MIBlock *out : b->outEdges()) {
            if (!out->inEdges().contains(b))
                qFatal("block %u has outgoing edge from block %u, "
                       "but does not appear in that block's incoming edges",
                       b->index(), out->index());
        }
    }
}

MIBlock *MIBlock::findEdgeTo(Operation::Kind target) const
{
    for (MIBlock *outEdge : outEdges()) {
        if (outEdge->instructions().front().opcode() == target)
            return outEdge;
    }
    return nullptr;
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
