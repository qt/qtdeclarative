/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quicktest.h"
#include "quicktestresult_p.h"
#include <QtTest/qtestsystem.h>
#include "qtestoptions_p.h"
#include <QApplication>
#include <QtDeclarative/qdeclarative.h>
#include <QtQuick1/qdeclarativeview.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#if defined(QML_VERSION) && QML_VERSION >= 0x020000
#include <QtDeclarative/qsgview.h>
#define QUICK_TEST_SCENEGRAPH 1
#endif
#include <QtDeclarative/qjsvalue.h>
#include <QtDeclarative/qjsengine.h>
#include <QtGui/qopengl.h>
#include <QtCore/qurl.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtGui/qtextdocument.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

// Copied from qdeclarativedebughelper_p.h in Qt, to avoid a dependency
// on a private header from Qt.
class Q_DECLARATIVE_EXPORT QDeclarativeDebugHelper
{
public:
    static QJSEngine *getScriptEngine(QDeclarativeEngine *engine);
    static void setAnimationSlowDownFactor(qreal factor);
    static void enableDebugging();
};

class QTestRootObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowShown READ windowShown NOTIFY windowShownChanged)
public:
    QTestRootObject(QObject *parent = 0)
        : QObject(parent), hasQuit(false), m_windowShown(false) {}

    bool hasQuit;

    bool windowShown() const { return m_windowShown; }
    void setWindowShown(bool value) { m_windowShown = value; emit windowShownChanged(); }

Q_SIGNALS:
    void windowShownChanged();

private Q_SLOTS:
    void quit() { hasQuit = true; }

private:
    bool m_windowShown;
};

static inline QString stripQuotes(const QString &s)
{
    if (s.length() >= 2 && s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"')))
        return s.mid(1, s.length() - 2);
    else
        return s;
}

int quick_test_main(int argc, char **argv, const char *name, quick_test_viewport_create createViewport, const char *sourceDir)
{
    QApplication* app = 0;
    if (!QCoreApplication::instance()) {
        app = new QApplication(argc, argv);
    }

    // Look for QML-specific command-line options.
    //      -import dir         Specify an import directory.
    //      -input dir          Specify the input directory for test cases.
    //      -qtquick1           Run with QtQuick 1 rather than QtQuick 2.
    QStringList imports;
    QString testPath;
    bool qtQuick2 = true;
    int outargc = 1;
    int index = 1;
    while (index < argc) {
        if (strcmp(argv[index], "-import") == 0 && (index + 1) < argc) {
            imports += stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-input") == 0 && (index + 1) < argc) {
            testPath = stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-opengl") == 0) {
            ++index;
        } else if (strcmp(argv[index], "-qtquick1") == 0) {
            qtQuick2 = false;
            ++index;
        } else if (outargc != index) {
            argv[outargc++] = argv[index++];
        } else {
            ++outargc;
            ++index;
        }
    }
    argv[outargc] = 0;
    argc = outargc;

    // Parse the command-line arguments.
    QuickTestResult::parseArgs(argc, argv);
    QuickTestResult::setProgramName(name);

    // Determine where to look for the test data.
    if (testPath.isEmpty() && sourceDir)
        testPath = QString::fromLocal8Bit(sourceDir);
    if (testPath.isEmpty())
        testPath = QLatin1String(".");

    // Scan the test data directory recursively, looking for "tst_*.qml" files.
    QStringList filters;
    filters += QLatin1String("tst_*.qml");
    QStringList files;
    QDirIterator iter(testPath, filters, QDir::Files,
                      QDirIterator::Subdirectories |
                      QDirIterator::FollowSymlinks);
    while (iter.hasNext())
        files += iter.next();
    files.sort();

    // Bail out if we didn't find any test cases.
    if (files.isEmpty()) {
        qWarning() << argv[0] << ": could not find any test cases under"
                   << testPath;
        return 1;
    }


    // Scan through all of the "tst_*.qml" files and run each of them
    // in turn with a QDeclarativeView.
#ifdef QUICK_TEST_SCENEGRAPH
    if (qtQuick2) {
        foreach (QString file, files) {
            QFileInfo fi(file);
            if (!fi.exists())
                continue;
            QSGView view;
            QTestRootObject rootobj;
            QEventLoop eventLoop;
            QObject::connect(view.engine(), SIGNAL(quit()),
                             &rootobj, SLOT(quit()));
            QObject::connect(view.engine(), SIGNAL(quit()),
                             &eventLoop, SLOT(quit()));
            view.rootContext()->setContextProperty
                (QLatin1String("qtest"), &rootobj);
            foreach (QString path, imports)
                view.engine()->addImportPath(path);
            QString path = fi.absoluteFilePath();
            if (path.startsWith(QLatin1String(":/")))
                view.setSource(QUrl(QLatin1String("qrc:") + path.mid(2)));
            else
                view.setSource(QUrl::fromLocalFile(path));
            if (QTest::printAvailableFunctions)
                continue;
            if (view.status() == QSGView::Error) {
                // Error compiling the test - flag failure in the log and continue.
                QList<QDeclarativeError> errors = view.errors();
                QuickTestResult results;
                results.setTestCaseName(fi.baseName());
                results.startLogging();
                results.setFunctionName(QLatin1String("compile"));
                results.setFunctionType(QuickTestResult::Func);
                results.fail(errors.at(0).description(),
                             errors.at(0).url().toString(),
                             errors.at(0).line());
                results.finishTestFunction();
                results.setFunctionName(QString());
                results.setFunctionType(QuickTestResult::NoWhere);
                results.stopLogging();
                continue;
            }
            if (!rootobj.hasQuit) {
                // If the test already quit, then it was performed
                // synchronously during setSource().  Otherwise it is
                // an asynchronous test and we need to show the window
                // and wait for the quit indication.
                view.show();
                QTest::qWaitForWindowShown(&view);
                rootobj.setWindowShown(true);
                if (!rootobj.hasQuit)
                    eventLoop.exec();
            }
        }
    } else
#endif
    {
        foreach (QString file, files) {
            QFileInfo fi(file);
            if (!fi.exists())
                continue;
            QDeclarativeView view;
            QTestRootObject rootobj;
            QEventLoop eventLoop;
            QObject::connect(view.engine(), SIGNAL(quit()),
                             &rootobj, SLOT(quit()));
            QObject::connect(view.engine(), SIGNAL(quit()),
                             &eventLoop, SLOT(quit()));
            if (createViewport)
                view.setViewport((*createViewport)());
            view.rootContext()->setContextProperty
                (QLatin1String("qtest"), &rootobj);
            foreach (QString path, imports)
                view.engine()->addImportPath(path);
            QString path = fi.absoluteFilePath();
            if (path.startsWith(QLatin1String(":/")))
                view.setSource(QUrl(QLatin1String("qrc:") + path.mid(2)));
            else
                view.setSource(QUrl::fromLocalFile(path));
            if (QTest::printAvailableFunctions)
                continue;
            if (view.status() == QDeclarativeView::Error) {
                // Error compiling the test - flag failure in the log and continue.
                QList<QDeclarativeError> errors = view.errors();
                QuickTestResult results;
                results.setTestCaseName(fi.baseName());
                results.startLogging();
                results.setFunctionName(QLatin1String("compile"));
                results.setFunctionType(QuickTestResult::Func);
                results.fail(errors.at(0).description(),
                             errors.at(0).url().toString(),
                             errors.at(0).line());
                results.finishTestFunction();
                results.setFunctionName(QString());
                results.setFunctionType(QuickTestResult::NoWhere);
                results.stopLogging();
                continue;
            }
            if (!rootobj.hasQuit) {
                // If the test already quit, then it was performed
                // synchronously during setSource().  Otherwise it is
                // an asynchronous test and we need to show the window
                // and wait for the quit indication.
                view.show();
                QTest::qWaitForWindowShown(&view);
                rootobj.setWindowShown(true);
                if (!rootobj.hasQuit)
                    eventLoop.exec();
            }
        }
    }

    // Flush the current logging stream.
    QuickTestResult::setProgramName(0);

    delete app;
    // Return the number of failures as the exit code.
    return QuickTestResult::exitCode();
}

QT_END_NAMESPACE

#include "quicktest.moc"
