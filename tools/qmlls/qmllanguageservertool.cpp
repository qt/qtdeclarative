/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qqmllanguageserver.h"
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include "../shared/qqmltoolingsettings.h"
#include <QtCore/qdiriterator.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qmutex.h>
#include <QtCore/QMutexLocker>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qtimer.h>

#include <QtQmlCompiler/private/qqmljsresourcefilemapper_p.h>
#include <QtQmlCompiler/private/qqmljscompiler_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#if QT_CONFIG(commandlineparser)
#    include <QtCore/qcommandlineparser.h>
#endif

#ifndef QT_BOOTSTRAPPED
#    include <QtCore/qlibraryinfo.h>
#endif

#include "qlanguageserver_p.h"

#include <iostream>
#ifdef Q_OS_WIN32
#    include <fcntl.h>
#    include <io.h>
#endif

using namespace QmlLsp;

QFile *logFile = nullptr;
QBasicMutex *logFileLock = nullptr;

class StdinReader : public QObject
{
    Q_OBJECT
public:
    void run()
    {
        auto guard = qScopeGuard([this]() { emit eof(); });
        char data[256];
        auto buffer = static_cast<char *>(data);
        while (std::cin.get(buffer[0])) { // should poll/select and process events
            const int read = std::cin.readsome(buffer + 1, 255) + 1;
            emit receivedData(QByteArray(buffer, read));
        }
    }
signals:
    void receivedData(const QByteArray &data);
    void eof();
};

// To debug:
//
// * simple logging can be redirected to a file
//   passing -l <file> to the qmlls command
//
// * more complex debugging can use named pipes:
//
//     mkfifo qmllsIn
//     mkfifo qmllsOut
//
// this together with a qmllsEcho script that can be defined as
//
//     #!/bin/sh
//     cat -u < ~/qmllsOut &
//     cat -u > ~/qmllsIn
//
// allows to use qmllsEcho as lsp server, and still easily start
// it in a terminal
//
//     qmlls < ~/qmllsIn > ~/qmllsOut
//
// * statup can be slowed down to have the time to attach via the
//   -w <nSeconds> flag.

int main(int argv, char *argc[])
{
#ifdef Q_OS_WIN32
    // windows does not open stdin/stdout in binary mode by default
    int err = _setmode(_fileno(stdout), _O_BINARY);
    if (err == -1)
        perror("Cannot set mode for stdout");
    err = _setmode(_fileno(stdin), _O_BINARY);
    if (err == -1)
        perror("Cannot set mode for stdin");
#endif

    qSetGlobalQHashSeed(0);
    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmllanguageserver");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    QQmlToolingSettings settings(QLatin1String("qmllanguageserver"));
    parser.setApplicationDescription(QLatin1String(R"(QML languageserver)"));

    QCommandLineOption waitOption(QStringList() << "w"
                                                << "wait",
                                  QLatin1String("Waits the given number of seconds before startup"),
                                  QLatin1String("waitSeconds"));
    parser.addOption(waitOption);

    QCommandLineOption verboseOption(
            QStringList() << "v"
                          << "verbose",
            QLatin1String("Outputs extra information on the operations being performed"));
    parser.addOption(verboseOption);

    QCommandLineOption logFileOption(QStringList() << "l"
                                                   << "log-file",
                                     QLatin1String("Writes logging to the given file"),
                                     QLatin1String("logFile"));
    parser.addOption(logFileOption);

    parser.process(app);
    if (parser.isSet(logFileOption)) {
        QString fileName = parser.value(logFileOption);
        qInfo() << "will log to" << fileName;
        logFile = new QFile(fileName);
        logFileLock = new QMutex;
        logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Text);
        qInstallMessageHandler([](QtMsgType t, const QMessageLogContext &, const QString &msg) {
            QMutexLocker l(logFileLock);
            logFile->write(QString::number(int(t)).toUtf8());
            logFile->write(" ");
            logFile->write(msg.toUtf8());
            logFile->write("\n");
            logFile->flush();
        });
    }
    if (parser.isSet(verboseOption))
        QLoggingCategory::setFilterRules("qt.languageserver*.debug=true\n");
    if (parser.isSet(waitOption)) {
        int waitSeconds = parser.value(waitOption).toInt();
        if (waitSeconds > 0)
            qDebug() << "waiting";
        QThread::sleep(waitSeconds);
        qDebug() << "starting";
    }
#endif
    QMutex writeMutex;
    QQmlLanguageServer qmlServer([&writeMutex](const QByteArray &data) {
        QMutexLocker l(&writeMutex);
        std::cout.write(data.constData(), data.length());
        std::cout.flush();
    });
    StdinReader *r = new StdinReader;
    QObject::connect(r, &StdinReader::receivedData, qmlServer.server(),
                     &QLanguageServer::receiveData);
    QObject::connect(r, &StdinReader::eof, &app, []() {
        QTimer::singleShot(100, []() {
            QCoreApplication::processEvents();
            QCoreApplication::exit();
        });
    });
    QThreadPool::globalInstance()->start([r]() { r->run(); });
    app.exec();
    return qmlServer.returnValue();
}

#include "qmllanguageservertool.moc"
