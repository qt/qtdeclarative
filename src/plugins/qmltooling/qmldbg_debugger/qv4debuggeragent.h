// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4DEBUGGERAGENT_H
#define QV4DEBUGGERAGENT_H

#include "qv4debugger.h"

QT_BEGIN_NAMESPACE

class QV4DebugServiceImpl;

class QV4DebuggerAgent : public QObject
{
    Q_OBJECT
public:
    QV4DebuggerAgent(QV4DebugServiceImpl *debugService) : m_debugService(debugService) {}

    QV4Debugger *pausedDebugger() const;
    bool isRunning() const;

    void addDebugger(QV4Debugger *debugger);
    void removeDebugger(QV4Debugger *debugger);
    const QList<QV4Debugger *> &debuggers();

    void pause(QV4Debugger *debugger) const;
    void pauseAll() const;
    void resumeAll() const;
    int addBreakPoint(const QString &fileName, int lineNumber, bool enabled = true, const QString &condition = QString());
    void removeBreakPoint(int id);
    void removeAllBreakPoints();
    void enableBreakPoint(int id, bool onoff);
    QList<int> breakPointIds(const QString &fileName, int lineNumber) const;

    bool breakOnThrow() const { return m_breakOnThrow; }
    void setBreakOnThrow(bool onoff);
    void clearAllPauseRequests();

    void debuggerPaused(QV4Debugger *debugger, QV4Debugger::PauseReason reason);
    void handleDebuggerDeleted(QObject *debugger);

private:
    QList<QV4Debugger *> m_debuggers;

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
    int m_lastBreakPointId = 0;
    bool m_breakOnThrow = false;
    QV4DebugServiceImpl *m_debugService = nullptr;
};

QT_END_NAMESPACE

#endif // QV4DEBUGGERAGENT_H
