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

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include "qv4global_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"

#include <QHash>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Function;

namespace Debugging {

class DebuggerAgent;

class Q_QML_EXPORT Debugger
{
public:
    enum State {
        Running,
        Paused
    };

    Debugger(ExecutionEngine *_engine);
    ~Debugger();

    void attachToAgent(DebuggerAgent *agent);
    void detachFromAgent();

    void pause();
    void resume();

    State state() const { return m_state; }

    void addBreakPoint(const QString &fileName, int lineNumber);
    void removeBreakPoint(const QString &fileName, int lineNumber);

    struct ExecutionState
    {
        ExecutionState() : lineNumber(-1), function(0) {}
        QString fileName;
        int lineNumber;
        Function *function;
    };

    ExecutionState currentExecutionState(const uchar *code = 0) const;

    bool pauseAtNextOpportunity() const {
        return m_pauseRequested || m_havePendingBreakPoints;
    }
    void setPendingBreakpoints(Function *function);

public: // compile-time interface
    void maybeBreakAtInstruction(const uchar *code, bool breakPointHit);

public: // execution hooks
    void aboutToThrow(const ValueRef value);

private:
    // requires lock to be held
    void pauseAndWait();

    void applyPendingBreakPoints();

    struct BreakPoints : public QHash<QString, QList<int> >
    {
        void add(const QString &fileName, int lineNumber);
        bool remove(const QString &fileName, int lineNumber);
        bool contains(const QString &fileName, int lineNumber) const;
        void applyToFunction(Function *function, bool removeBreakPoints);
    };

    QV4::ExecutionEngine *_engine;
    DebuggerAgent *m_agent;
    QMutex m_lock;
    QWaitCondition m_runningCondition;
    State m_state;
    bool m_pauseRequested;
    bool m_havePendingBreakPoints;
    BreakPoints m_pendingBreakPointsToAdd;
    BreakPoints m_pendingBreakPointsToAddToFutureCode;
    BreakPoints m_pendingBreakPointsToRemove;
    const uchar *m_currentInstructionPointer;
};

class Q_QML_EXPORT DebuggerAgent : public QObject
{
    Q_OBJECT
public:
    ~DebuggerAgent();

    void addDebugger(Debugger *debugger);
    void removeDebugger(Debugger *debugger);

    void pause(Debugger *debugger) const;
    void pauseAll() const;
    void addBreakPoint(const QString &fileName, int lineNumber) const;
    void removeBreakPoint(const QString &fileName, int lineNumber) const;

    Q_INVOKABLE virtual void debuggerPaused(QV4::Debugging::Debugger *debugger) = 0;

protected:
    QList<Debugger *> m_debuggers;
};

} // namespace Debugging
} // namespace QV4

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QV4::Debugging::Debugger*)

#endif // DEBUGGING_H
