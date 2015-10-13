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

#ifndef QV4DEBUGGERAGENT_H
#define QV4DEBUGGERAGENT_H

#include <private/qv4debugging_p.h>

QT_BEGIN_NAMESPACE

class QV4DebugServiceImpl;

class QV4DebuggerAgent : public QObject
{
    Q_OBJECT
public:
    QV4DebuggerAgent(QV4DebugServiceImpl *m_debugService);

    QV4::Debugging::V4Debugger *firstDebugger() const;
    bool isRunning() const;

    void addDebugger(QV4::Debugging::V4Debugger *debugger);
    void removeDebugger(QV4::Debugging::V4Debugger *debugger);

    void pause(QV4::Debugging::V4Debugger *debugger) const;
    void pauseAll() const;
    void resumeAll() const;
    int addBreakPoint(const QString &fileName, int lineNumber, bool enabled = true, const QString &condition = QString());
    void removeBreakPoint(int id);
    void removeAllBreakPoints();
    void enableBreakPoint(int id, bool onoff);
    QList<int> breakPointIds(const QString &fileName, int lineNumber) const;

    bool breakOnThrow() const { return m_breakOnThrow; }
    void setBreakOnThrow(bool onoff);

public slots:
    void debuggerPaused(QV4::Debugging::V4Debugger *debugger, QV4::Debugging::PauseReason reason);
    void sourcesCollected(QV4::Debugging::V4Debugger *debugger, const QStringList &sources,
                          int requestSequenceNr);
    void handleDebuggerDeleted(QObject *debugger);

private:
    QList<QV4::Debugging::V4Debugger *> m_debuggers;

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
