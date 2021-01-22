/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QV4DEBUGGERAGENT_H
#define QV4DEBUGGERAGENT_H

#include "qv4debugger.h"

QT_BEGIN_NAMESPACE

class QV4DebugServiceImpl;

class QV4DebuggerAgent : public QObject
{
    Q_OBJECT
public:
    QV4DebuggerAgent(QV4DebugServiceImpl *m_debugService);

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
    bool m_breakOnThrow;
    QV4DebugServiceImpl *m_debugService;
};

QT_END_NAMESPACE

#endif // QV4DEBUGGERAGENT_H
