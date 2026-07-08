// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4DEBUGGER_H
#define QV4DEBUGGER_H

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

#include "qv4datacollector.h"
#include <private/qv4debugging_p.h>
#include <private/qv4function_p.h>
#include <private/qv4context_p.h>
#include <private/qv4persistent_p.h>
#include <private/qv4executablecompilationunit_p.h>
#include <private/qqmlrefcount_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>

QT_BEGIN_NAMESPACE

class QV4DebugJob;
class QV4DataCollector;
class QV4Debugger : public QV4::Debugging::Debugger
{
    Q_OBJECT
public:
    struct BreakPoint {
        BreakPoint(const QString &fileName, int line);
        QString fileName;
        int lineNumber;
    };

    enum State {
        Running,
        Paused
    };

    enum Speed {
        FullThrottle = 0,
        StepOut,
        StepOver,
        StepIn,

        NotStepping = FullThrottle
    };

    enum PauseReason {
        PauseRequest,
        BreakPointHit,
        Throwing,
        Step
    };

    QV4Debugger(QV4::ExecutionEngine *engine);

    QV4::ExecutionEngine *engine() const;
    const QV4DataCollector *collector() const;
    QV4DataCollector *collector();

    void pause();
    void resume(Speed speed);

    State state() const;

    void addBreakPoint(const QString &fileName, int lineNumber,
                       const QString &condition = QString());
    void removeBreakPoint(const QString &fileName, int lineNumber);

    void setBreakOnThrow(bool onoff);

    void clearPauseRequest();

    // used for testing
    struct ExecutionState
    {
        QString fileName;
        int lineNumber;
    };
    ExecutionState currentExecutionState() const;
    qsizetype executableLinesCacheSize();

    QList<QV4::StackFrame> stackTrace(int frameLimit = -1) const;
    QList<QV4::Heap::ExecutionContext::ContextType> getScopeTypes(int frame = 0) const;

    QV4::Function *getFunction() const;
    void runInEngine(QV4DebugJob *job);

    int resolvedBreakpointLine(int requestedLine);

    // compile-time interface
    void maybeBreakAtInstruction() override;

    // execution hooks
    void enteringFunction() override;
    void leavingFunction(const QV4::ReturnedValue &retVal) override;
    void aboutToThrow() override;

    bool pauseAtNextOpportunity() const override;

Q_SIGNALS:
    void debuggerPaused(QV4Debugger *self, QV4Debugger::PauseReason reason);
    void scheduleJob();

private:
    // requires lock to be held
    void pauseAndWait(PauseReason reason);
    bool reallyHitTheBreakPoint(QV4::Function *function, const QString &filename, int linenr);
    struct ExecutableLine {
        int line;
        int functionStart; // first line of the function or binding it belongs to
        friend bool operator==(const ExecutableLine &a, const ExecutableLine &b)
        {
            return a.line == b.line && a.functionStart == b.functionStart;
        }
        friend bool operator<(const ExecutableLine &a, const ExecutableLine &b)
        {
            return a.line < b.line || (a.line == b.line && a.functionStart < b.functionStart);
        }
    };
    struct ExecutableLines {
        QList<ExecutableLine> lines; // the lines that carry code, sorted
        QList<int> functionStarts;   // sorted
    };
    static ExecutableLines collectExecutableLines(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit);
    static int firstLineAtOrAfter(const ExecutableLines &executable, int requestedLine);
    const ExecutableLines &executableLines(QV4::Function *function);
    void pruneExecutableLines();
    void runInEngine_havingLock(QV4DebugJob *job);
    void runJobUnpaused();

    QV4::ExecutionEngine *m_engine;
    QV4::CppStackFrame *m_currentFrame = 0;
    QMutex m_lock;
    QWaitCondition m_runningCondition;
    State m_state;
    Speed m_stepping;
    bool m_pauseRequested;
    bool m_haveBreakPoints;
    bool m_breakPointsChanged = false;
    bool m_breakOnThrow;

    struct BreakPointData {
        QString target; // as given by the client, possibly with a path
        QString condition;
    };
    QHash<BreakPoint, BreakPointData> m_breakPoints;
    struct UnitLines {
        // Keeps another unit from reusing the address while it is cached. This is
        // the base unit: pinning the executable unit would keep the engine from
        // trimming it, so a reloaded ES module could come back stale. The type
        // loader still keeps the type of a pinned unit, so units are only
        // cached while their file has breakpoints.
        QQmlRefPointer<QV4::CompiledData::CompilationUnit> unit;
        ExecutableLines lines;
    };
    QList<UnitLines> m_executableLines; // most recently used first
    QV4::PersistentValue m_returnedValue;

    QV4DebugJob *m_gatherSources;
    QV4DebugJob *m_runningJob;
    QV4DataCollector m_collector;
    QWaitCondition m_jobIsRunning;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QV4Debugger::PauseReason)
Q_DECLARE_METATYPE(QV4Debugger*)

#endif // QV4DEBUGGER_H
