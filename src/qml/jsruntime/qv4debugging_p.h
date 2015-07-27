/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <QtCore/QJsonObject>

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

struct DebuggerBreakPoint {
    DebuggerBreakPoint(const QString &fileName, int line)
        : fileName(fileName), lineNumber(line)
    {}
    QString fileName;
    int lineNumber;
};
inline uint qHash(const DebuggerBreakPoint &b, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(b.fileName, seed) ^ b.lineNumber;
}
inline bool operator==(const DebuggerBreakPoint &a, const DebuggerBreakPoint &b)
{
    return a.lineNumber == b.lineNumber && a.fileName == b.fileName;
}

typedef QHash<DebuggerBreakPoint, QString> BreakPoints;

class Q_QML_PRIVATE_EXPORT DataCollector
{
public:
    typedef uint Ref;
    typedef QVector<uint> Refs;

    DataCollector(QV4::ExecutionEngine *engine);
    ~DataCollector();

    void collect(const QV4::ScopedValue &value);

    QJsonObject lookupRef(Ref ref);

    Ref addFunctionRef(const QString &functionName);
    Ref addScriptRef(const QString &scriptName);

    void collectScope(QJsonObject *dict, QV4::Debugging::Debugger *debugger, int frameNr,
                      int scopeNr);

    QV4::ExecutionEngine *engine() const { return m_engine; }

private:
    friend class RefHolder;

    Ref addRef(QV4::Value value, bool deduplicate = true);
    QV4::ReturnedValue getValue(Ref ref);
    bool lookupSpecialRef(Ref ref, QJsonObject *dict);

    QJsonArray collectProperties(QV4::Object *object);
    QJsonObject collectAsJson(const QString &name, const QV4::ScopedValue &value);

    QV4::ExecutionEngine *m_engine;
    Refs *m_collectedRefs;
    QV4::PersistentValue values;
    typedef QHash<Ref, QJsonObject> SpecialRefs;
    SpecialRefs specialRefs;
};

class RefHolder {
public:
    RefHolder(DataCollector *collector, DataCollector::Refs *target) :
        m_collector(collector), m_previousRefs(collector->m_collectedRefs)
    {
        m_collector->m_collectedRefs = target;
    }

    ~RefHolder()
    {
        std::swap(m_collector->m_collectedRefs, m_previousRefs);
    }

private:
    DataCollector *m_collector;
    DataCollector::Refs *m_previousRefs;
};

class Q_QML_EXPORT Debugger
{
public:
    class Job
    {
    public:
        virtual ~Job() = 0;
        virtual void run() = 0;
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

    // used for testing
    struct ExecutionState
    {
        QString fileName;
        int lineNumber;
    };
    ExecutionState currentExecutionState() const;

    bool pauseAtNextOpportunity() const {
        return m_pauseRequested || m_haveBreakPoints || m_gatherSources || m_stepping >= StepOver;
    }

    QVector<StackFrame> stackTrace(int frameLimit = -1) const;
    void collectArgumentsInContext(DataCollector *collector, QStringList *names, int frameNr = 0,
                                   int scopeNr = 0);
    void collectLocalsInContext(DataCollector *collector, QStringList *names, int frameNr = 0,
                                int scopeNr = 0);
    bool collectThisInContext(DataCollector *collector, int frame = 0);
    bool collectThrownValue(DataCollector *collector);
    QVector<Heap::ExecutionContext::ContextType> getScopeTypes(int frame = 0) const;

    void evaluateExpression(int frameNr, const QString &expression,
                            DataCollector *resultsCollector);

public: // compile-time interface
    void maybeBreakAtInstruction();

public: // execution hooks
    void enteringFunction();
    void leavingFunction(const ReturnedValue &retVal);
    void aboutToThrow();

private:
    Function *getFunction() const;

    // requires lock to be held
    void pauseAndWait(PauseReason reason);

    bool reallyHitTheBreakPoint(const QString &filename, int linenr);

    void runInEngine(Job *job);
    void runInEngine_havingLock(Debugger::Job *job);

private:
    QV4::ExecutionEngine *m_engine;
    QV4::PersistentValue m_currentContext;
    DebuggerAgent *m_agent;
    QMutex m_lock;
    QWaitCondition m_runningCondition;
    State m_state;
    Speed m_stepping;
    bool m_pauseRequested;
    bool m_haveBreakPoints;
    bool m_breakOnThrow;

    BreakPoints m_breakPoints;
    QV4::PersistentValue m_returnedValue;

    Job *m_gatherSources;
    Job *m_runningJob;
    QWaitCondition m_jobIsRunning;
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
