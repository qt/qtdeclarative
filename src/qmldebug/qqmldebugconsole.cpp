// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmldebugconsole_p.h"
#include "qqmldebugcommandlistener_p.h"

#include <iostream>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlDebugConsole::QQmlDebugConsole(QObject *parent) : QObject(parent)
{
    registerCommand({ "help"_L1, "h"_L1 }, {}, tr("Show the list of available commands."),
                    [this](const QStringList &) {
                        printHelp();
                        prompt();
                    });
}

QQmlDebugConsole::~QQmlDebugConsole()
{
    stop();
}

void QQmlDebugConsole::registerCommand(const QStringList &names, const QString &argsHint,
                                       const QString &description, CommandHandler handler)
{
    m_commands.append(Command{ names, argsHint, description, std::move(handler) });
}

void QQmlDebugConsole::start()
{
    Q_ASSERT(!m_listenerThread.isRunning());

    m_listener = new QQmlDebugCommandListener;
    m_listener->moveToThread(&m_listenerThread);
    connect(m_listener, &QQmlDebugCommandListener::command, this, &QQmlDebugConsole::handleLine);
    connect(m_listener, &QQmlDebugCommandListener::endOfInput, this, &QQmlDebugConsole::endOfInput);
    connect(this, &QQmlDebugConsole::requestCommand, m_listener,
            &QQmlDebugCommandListener::readCommand);

    m_listenerThread.start();
}

void QQmlDebugConsole::stop()
{
    // The listener thread is idle in its event loop between commands, so quitting
    // it from here (after the last command has been processed) unblocks wait().
    if (m_listenerThread.isRunning()) {
        m_listenerThread.quit();
        m_listenerThread.wait();
    }

    delete std::exchange(m_listener, nullptr);
}

void QQmlDebugConsole::prompt(const QString &message, bool ready)
{
    if (!message.isEmpty())
        std::cerr << qPrintable(message) << std::endl;
    std::cerr << "> ";
    if (ready)
        emit requestCommand();
}

void QQmlDebugConsole::printLine(const QString &message)
{
    std::cerr << qPrintable(message) << std::endl;
}

void QQmlDebugConsole::askConfirmation(const QString &question, std::function<void()> onConfirmed,
                                       std::function<void()> onDeclined)
{
    m_pendingQuestion = question;
    m_onConfirmed = std::move(onConfirmed);
    m_onDeclined = std::move(onDeclined);
    prompt(question);
}

void QQmlDebugConsole::handleLine(const QString &line)
{
    const QStringList args = line.split(QChar::Space, Qt::SkipEmptyParts);
    if (args.isEmpty()) {
        prompt();
        return;
    }

    const QString keyword = args.first().toLower();
    const QStringList rest = args.mid(1);

    if (!m_pendingQuestion.isEmpty()) {
        if (keyword == "y"_L1 || keyword == "yes"_L1) {
            m_pendingQuestion.clear();
            m_onDeclined = {};
            if (auto onConfirmed = std::exchange(m_onConfirmed, {}))
                onConfirmed();
        } else if (keyword == "n"_L1 || keyword == "no"_L1) {
            m_pendingQuestion.clear();
            m_onConfirmed = {};
            if (auto onDeclined = std::exchange(m_onDeclined, {}))
                onDeclined();
            else
                prompt();
        } else {
            prompt(m_pendingQuestion);
        }
        return;
    }

    for (const Command &cmd : std::as_const(m_commands)) {
        if (cmd.names.contains(keyword)) {
            cmd.handler(rest);
            return;
        }
    }

    printHelp();
    prompt();
}

void QQmlDebugConsole::printHelp()
{
    printLine(tr("The following commands are available:"));
    for (const Command &cmd : std::as_const(m_commands)) {
        QStringList quoted;
        quoted.reserve(cmd.names.size());
        for (const QString &name : cmd.names)
            quoted << u'\'' + name + u'\'';
        QString header = quoted.join(", "_L1);
        if (!cmd.argsHint.isEmpty())
            header += u' ' + cmd.argsHint;
        printLine(header);
        printLine("    "_L1 + cmd.description);
    }
}

QT_END_NAMESPACE

#include "moc_qqmldebugconsole_p.cpp"
