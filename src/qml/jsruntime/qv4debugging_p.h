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
#include "qv4scopedvalue_p.h"

#include <QHash>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Function;

namespace Debugging {

enum PauseReason {
    PauseRequest,
    BreakPoint,
    Throwing,
    Step
};

class DebuggerAgent;

class Q_QML_EXPORT Debugger
{
public:
    class Job
    {
    public:
        virtual ~Job() = 0;
        virtual void run() = 0;
    };

    class Q_QML_EXPORT Collector
    {
    public:
        Collector(ExecutionEngine *engine): m_engine(engine), m_isProperty(false) {}
        virtual ~Collector();

        void collect(const QString &name, const ScopedValue &value);
        void collect(const ObjectRef object);

    protected:
        virtual void addUndefined(const QString &name) = 0;
        virtual void addNull(const QString &name) = 0;
        virtual void addBoolean(const QString &name, bool value) = 0;
        virtual void addString(const QString &name, const QString &value) = 0;
        virtual void addObject(const QString &name, ValueRef value) = 0;
        virtual void addInteger(const QString &name, int value) = 0;
        virtual void addDouble(const QString &name, double value) = 0;

        QV4::ExecutionEngine *engine() const { return m_engine; }

        bool isProperty() const { return m_isProperty; }
        void setIsProperty(bool onoff) { m_isProperty = onoff; }

    private:
        QV4::ExecutionEngine *m_engine;
        bool m_isProperty;
    };

    enum State {
        Running,
        Paused
    };

    enum Speed {
        FullThrottle = 0,
        StepIn,
        StepOut,
        StepOver,

        NotStepping = FullThrottle
    };

    Debugger(ExecutionEngine *engine);
    ~Debugger();

    ExecutionEngine *engine() const
    { return m_engine; }

    void attachToAgent(DebuggerAgent *agent);
    void detachFromAgent();
    DebuggerAgent *agent() const { return m_agent; }

    void gatherSources(int requestSequenceNr);
    void pause();
    void resume(Speed speed);

    State state() const { return m_state; }

    void addBreakPoint(const QString &fileName, int lineNumber, const QString &condition = QString());
    void removeBreakPoint(const QString &fileName, int lineNumber);

    void setBreakOnThrow(bool onoff);

    struct ExecutionState
    {
        ExecutionState() : lineNumber(-1), function(0) {}
        QString fileName;
        int lineNumber;
        Function *function;
    };

    ExecutionState currentExecutionState(const uchar *code = 0) const;

    bool pauseAtNextOpportunity() const {
        return m_pauseRequested || m_havePendingBreakPoints || m_gatherSources;
    }
    void setPendingBreakpoints(Function *function);

    QVector<StackFrame> stackTrace(int frameLimit = -1) const;
    void collectArgumentsInContext(Collector *collector, int frameNr = 0, int scopeNr = 0);
    void collectLocalsInContext(Collector *collector, int frameNr = 0, int scopeNr = 0);
    bool collectThisInContext(Collector *collector, int frame = 0);
    void collectThrownValue(Collector *collector);
    void collectReturnedValue(Collector *collector) const;
    QVector<ExecutionContext::Type> getScopeTypes(int frame = 0) const;

public: // compile-time interface
    void maybeBreakAtInstruction(const uchar *code, bool breakPointHit);

public: // execution hooks
    void enteringFunction();
    void leavingFunction(const ReturnedValue &retVal);
    void aboutToThrow();

private:
    Function *getFunction() const;

    // requires lock to be held
    void pauseAndWait(PauseReason reason);
    // requires lock to be held
    void setTemporaryBreakPointOnNextLine();
    // requires lock to be held
    void clearTemporaryBreakPoints();
    // requires lock to be held
    bool temporaryBreakPointInFunction(ExecutionContext *context) const;

    void applyPendingBreakPoints();
    static void setBreakOnInstruction(Function *function, qptrdiff codeOffset, bool onoff);
    static bool hasBreakOnInstruction(Function *function, qptrdiff codeOffset);
    bool reallyHitTheBreakPoint(const QString &filename, int linenr);

    void runInEngine(Job *job);
    void runInEngine_havingLock(Debugger::Job *job);

private:
    struct BreakPoints : public QHash<QString, QList<int> >
    {
        void add(const QString &fileName, int lineNumber);
        bool remove(const QString &fileName, int lineNumber);
        bool contains(const QString &fileName, int lineNumber) const;
        void applyToFunction(Function *function, bool removeBreakPoints);
    };

    QV4::ExecutionEngine *m_engine;
    DebuggerAgent *m_agent;
    QMutex m_lock;
    QWaitCondition m_runningCondition;
    State m_state;
    bool m_pauseRequested;
    Job *m_gatherSources;
    bool m_havePendingBreakPoints;
    BreakPoints m_pendingBreakPointsToAdd;
    BreakPoints m_pendingBreakPointsToAddToFutureCode;
    BreakPoints m_pendingBreakPointsToRemove;
    const uchar *m_currentInstructionPointer;
    Speed m_stepping;
    bool m_stopForStepping;
    QV4::PersistentValue m_returnedValue;

    struct TemporaryBreakPoint {
        Function *function;
        QVector<qptrdiff> codeOffsets;
        ExecutionContext *context;
        TemporaryBreakPoint(): function(0), context(0) {}
        TemporaryBreakPoint(Function *function, ExecutionContext *context)
            : function(function)
            , context(context)
        {}
    } m_temporaryBreakPoints;

    bool m_breakOnThrow;

    Job *m_runningJob;
    QWaitCondition m_jobIsRunning;

    struct BreakPointConditions: public QHash<QString, QString>
    {
        static QString genKey(const QString &fileName, int lineNumber)
        {
            return fileName + QLatin1Char(':') + QString::number(lineNumber);
        }

        QString condition(const QString &fileName, int lineNumber)
        { return value(genKey(fileName, lineNumber)); }
        void add(const QString &fileName, int lineNumber, const QString &condition)
        { insert(genKey(fileName, lineNumber), condition); }
        void remove(const QString &fileName, int lineNumber)
        { take(genKey(fileName, lineNumber)); }
    };
    BreakPointConditions m_breakPointConditions;
};

class Q_QML_EXPORT DebuggerAgent : public QObject
{
    Q_OBJECT
public:
    DebuggerAgent(): m_breakOnThrow(false) {}
    ~DebuggerAgent();

    void addDebugger(Debugger *debugger);
    void removeDebugger(Debugger *debugger);

    void pause(Debugger *debugger) const;
    void pauseAll() const;
    void resumeAll() const;
    int addBreakPoint(const QString &fileName, int lineNumber, bool enabled = true, const QString &condition = QString());
    void removeBreakPoint(int id);
    void removeAllBreakPoints();
    void enableBreakPoint(int id, bool onoff);
    QList<int> breakPointIds(const QString &fileName, int lineNumber) const;

    bool breakOnThrow() const { return m_breakOnThrow; }
    void setBreakOnThrow(bool onoff);

    Q_INVOKABLE virtual void debuggerPaused(QV4::Debugging::Debugger *debugger,
                                            QV4::Debugging::PauseReason reason) = 0;
    Q_INVOKABLE virtual void sourcesCollected(QV4::Debugging::Debugger *debugger,
                                              QStringList sources, int requestSequenceNr) = 0;

protected:
    QList<Debugger *> m_debuggers;

    struct BreakPoint {
        QString fileName;
        int lineNr;
        bool enabled;
        QString condition;

        BreakPoint(): lineNr(-1), enabled(false) {}
        BreakPoint(const QString &fileName, int lineNr, bool enabled, const QString &condition)
            : fileName(fileName), lineNr(lineNr), enabled(enabled), condition(condition)
        {}

        bool isValid() const { return lineNr >= 0 && !fileName.isEmpty(); }
    };

    QHash<int, BreakPoint> m_breakPoints;
    bool m_breakOnThrow;
};

} // namespace Debugging
} // namespace QV4

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QV4::Debugging::Debugger*)
Q_DECLARE_METATYPE(QV4::Debugging::PauseReason)

#endif // DEBUGGING_H
