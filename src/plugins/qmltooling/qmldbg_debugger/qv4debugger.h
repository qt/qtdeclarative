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

#include <private/qv4debugging_p.h>
#include <private/qv4function_p.h>
#include <private/qv4context_p.h>
#include <private/qv4persistent_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>

QT_BEGIN_NAMESPACE

class QV4Debugger : public QV4::Debugging::Debugger
{
    Q_OBJECT
public:
    struct BreakPoint {
        BreakPoint(const QString &fileName, int line);
        QString fileName;
        int lineNumber;
    };

    class Job
    {
    public:
        virtual ~Job();
        virtual void run() = 0;
    };

    class JavaScriptJob: public Job
    {
        QV4::ExecutionEngine *engine;
        int frameNr;
        const QString &script;
        bool resultIsException;

    public:
        JavaScriptJob(QV4::ExecutionEngine *engine, int frameNr, const QString &script);
        void run();
        bool hasExeption() const;

    protected:
        virtual void handleResult(QV4::ScopedValue &result) = 0;
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

    QVector<QV4::StackFrame> stackTrace(int frameLimit = -1) const;
    QVector<QV4::Heap::ExecutionContext::ContextType> getScopeTypes(int frame = 0) const;

    QV4::Function *getFunction() const;
    void runInEngine(Job *job);

    // compile-time interface
    void maybeBreakAtInstruction() Q_DECL_OVERRIDE;

    // execution hooks
    void enteringFunction() Q_DECL_OVERRIDE;
    void leavingFunction(const QV4::ReturnedValue &retVal) Q_DECL_OVERRIDE;
    void aboutToThrow() Q_DECL_OVERRIDE;

    bool pauseAtNextOpportunity() const Q_DECL_OVERRIDE;

signals:
    void debuggerPaused(QV4Debugger *self, QV4Debugger::PauseReason reason);

private:
    // requires lock to be held
    void pauseAndWait(PauseReason reason);
    bool reallyHitTheBreakPoint(const QString &filename, int linenr);
    void runInEngine_havingLock(QV4Debugger::Job *job);

    QV4::ExecutionEngine *m_engine;
    QV4::PersistentValue m_currentContext;
    QMutex m_lock;
    QWaitCondition m_runningCondition;
    State m_state;
    Speed m_stepping;
    bool m_pauseRequested;
    bool m_haveBreakPoints;
    bool m_breakOnThrow;

    QHash<BreakPoint, QString> m_breakPoints;
    QV4::PersistentValue m_returnedValue;

    Job *m_gatherSources;
    Job *m_runningJob;
    QWaitCondition m_jobIsRunning;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QV4Debugger::PauseReason)
Q_DECLARE_METATYPE(QV4Debugger*)

#endif // QV4DEBUGGER_H
