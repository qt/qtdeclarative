// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGCONSOLE_P_H
#define QQMLDEBUGCONSOLE_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QQmlDebugCommandListener;

// A simple interactive command line for QML debugging tools. It reads commands
// from standard input on a worker thread, dispatches them to registered
// handlers, and provides a reusable yes/no confirmation flow. Both qmlprofiler
// and qmlpreview drive their interactive modes through this class.
class QQmlDebugConsole : public QObject
{
    Q_OBJECT
public:
    using CommandHandler = std::function<void(const QStringList &args)>;

    explicit QQmlDebugConsole(QObject *parent = nullptr);
    ~QQmlDebugConsole();

    void registerCommand(const QStringList &names, const QString &argsHint,
                         const QString &description, CommandHandler handler);

    void start();
    void stop();

    void prompt(const QString &message = QString(), bool ready = true);
    void printLine(const QString &message);

    void askConfirmation(const QString &question, std::function<void()> onConfirmed,
                         std::function<void()> onDeclined = {});

Q_SIGNALS:
    void requestCommand();
    // Emitted when standard input is closed (EOF). Consumers typically quit.
    void endOfInput();

private:
    struct Command
    {
        QStringList names;
        QString argsHint;
        QString description;
        CommandHandler handler;
    };

    void handleLine(const QString &line);
    void printHelp();

    QThread m_listenerThread;
    QQmlDebugCommandListener *m_listener = nullptr;

    QList<Command> m_commands;

    QString m_pendingQuestion;
    std::function<void()> m_onConfirmed;
    std::function<void()> m_onDeclined;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCONSOLE_P_H
