/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#if defined(QML_VERSION) && QML_VERSION >= 0x020000
#include <QtQuick/qquickview.h>
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
#include <QtCore/qtextstream.h>
#include <QtGui/qtextdocument.h>
#include <stdio.h>
#include <QtGui/QGuiApplication>
#include <QtCore/QTranslator>
QT_BEGIN_NAMESPACE

static void installCoverageTool(const char * appname, const char * testname)
{
#ifdef __COVERAGESCANNER__
    // Install Coverage Tool
    __coveragescanner_install(appname);
    __coveragescanner_testname(testname);
    __coveragescanner_clear();
#else
    Q_UNUSED(appname);
    Q_UNUSED(testname);
#endif
}

static void saveCoverageTool(const char * appname, bool testfailed)
{
#ifdef __COVERAGESCANNER__
    // install again to make sure the filename is correct.
    // without this, a plugin or similar may have changed the filename.
    __coveragescanner_install(appname);
    __coveragescanner_teststate(testfailed ? "FAILED" : "PASSED");
    __coveragescanner_save();
    __coveragescanner_testname("");
    __coveragescanner_clear();
#else
    Q_UNUSED(appname);
    Q_UNUSED(testfailed);
#endif
}


class QTestRootObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowShown READ windowShown NOTIFY windowShownChanged)
    Q_PROPERTY(bool hasTestCase READ hasTestCase WRITE setHasTestCase NOTIFY hasTestCaseChanged)
public:
    QTestRootObject(QObject *parent = 0)
        : QObject(parent), hasQuit(false), m_windowShown(false), m_hasTestCase(false)  {}

    bool hasQuit:1;
    bool hasTestCase() const { return m_hasTestCase; }
    void setHasTestCase(bool value) { m_hasTestCase = value; emit hasTestCaseChanged(); }

    bool windowShown() const { return m_windowShown; }
    void setWindowShown(bool value) { m_windowShown = value; emit windowShownChanged(); }

Q_SIGNALS:
    void windowShownChanged();
    void hasTestCaseChanged();

private Q_SLOTS:
    void quit() { hasQuit = true; }

private:
    bool m_windowShown : 1;
    bool m_hasTestCase :1;
};

static inline QString stripQuotes(const QString &s)
{
    if (s.length() >= 2 && s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"')))
        return s.mid(1, s.length() - 2);
    else
        return s;
}

template <class View> void handleCompileErrors(const QFileInfo &fi, const View &view)
{
    // Error compiling the test - flag failure in the log and continue.
    const QList<QDeclarativeError> errors = view.errors();
    QuickTestResult results;
    results.setTestCaseName(fi.baseName());
    results.startLogging();
    results.setFunctionName(QLatin1String("compile"));
    // Verbose warning output of all messages and relevant parameters
    QString message;
    QTextStream str(&message);
    str << "\n  " << QDir::toNativeSeparators(fi.absoluteFilePath()) << " produced "
        << errors.size() << " error(s):\n";
    foreach (const QDeclarativeError &e, errors) {
        str << "    ";
        if (e.url().isLocalFile()) {
            str << e.url().toLocalFile();
        } else {
            str << e.url().toString();
        }
        if (e.line() > 0)
            str << ':' << e.line() << ',' << e.column();
        str << ": " << e.description() << '\n';
    }
    str << "  Working directory: " << QDir::toNativeSeparators(QDir::current().absolutePath()) << '\n';
    if (QDeclarativeEngine *engine = view.engine()) {
        str << "  View: " << view.metaObject()->className() << ", import paths:\n";
        foreach (const QString &i, engine->importPathList())
            str << "    '" << QDir::toNativeSeparators(i) << "'\n";
        const QStringList pluginPaths = engine->pluginPathList();
        str << "  Plugin paths:\n";
        foreach (const QString &p, pluginPaths)
            str << "    '" << QDir::toNativeSeparators(p) << "'\n";
    }
    qWarning("%s", qPrintable(message));
    // Fail with error 0.
    results.fail(errors.at(0).description(),
                 errors.at(0).url(), errors.at(0).line());
    results.finishTestData();
    results.finishTestDataCleanup();
    results.finishTestFunction();
    results.setFunctionName(QString());
    results.stopLogging();
}

int quick_test_main(int argc, char **argv, const char *name, quick_test_viewport_create createViewport, const char *sourceDir)
{
    QGuiApplication* app = 0;
    if (!QCoreApplication::instance()) {
        app = new QGuiApplication(argc, argv);
    }

    // Look for QML-specific command-line options.
    //      -import dir         Specify an import directory.
    //      -input dir          Specify the input directory for test cases.
    //      -qtquick1           Run with QtQuick 1 rather than QtQuick 2.
    //      -translation file   Specify the translation file.
    QStringList imports;
    QString testPath;
    QString translationFile;
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
        } else if (strcmp(argv[index], "-translation") == 0 && (index + 1) < argc) {
            translationFile = stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
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

    installCoverageTool(argv[0], name);

    QTranslator translator;
    if (!translationFile.isEmpty()) {
        if (translator.load(translationFile)) {
            app->installTranslator(&translator);
        } else {
            qWarning("Could not load the translation file '%s'.", qPrintable(translationFile));
        }
    }

    // Determine where to look for the test data.
    if (testPath.isEmpty() && sourceDir)
        testPath = QString::fromLocal8Bit(sourceDir);
    if (testPath.isEmpty()) {
        QDir current = QDir::current();
#ifdef Q_OS_WIN
        // Skip release/debug subfolders
        if (!current.dirName().compare(QLatin1String("Release"), Qt::CaseInsensitive)
            || !current.dirName().compare(QLatin1String("Debug"), Qt::CaseInsensitive))
            current.cdUp();
#endif // Q_OS_WIN
        testPath = current.absolutePath();
    }
    QStringList files;

    const QFileInfo testPathInfo(testPath);
    if (testPathInfo.isFile()) {
        if (!testPath.endsWith(QStringLiteral(".qml"))) {
            qWarning("'%s' does not have the suffix '.qml'.", qPrintable(testPath));
            return 1;
        }
        files << testPath;
    } else if (testPathInfo.isDir()) {
        // Scan the test data directory recursively, looking for "tst_*.qml" files.
        const QStringList filters(QStringLiteral("tst_*.qml"));
        QDirIterator iter(testPathInfo.absoluteFilePath(), filters, QDir::Files,
                          QDirIterator::Subdirectories |
                          QDirIterator::FollowSymlinks);
        while (iter.hasNext())
            files += iter.next();
        files.sort();
        if (files.isEmpty()) {
            qWarning("The directory '%s' does not contain any test files matching '%s'",
                     qPrintable(testPath), qPrintable(filters.front()));
            return 1;
        }
    } else {
        qWarning("'%s' does not exist under '%s'.",
                 qPrintable(testPath), qPrintable(QDir::currentPath()));
        return 1;
    }

    // Scan through all of the "tst_*.qml" files and run each of them
    // in turn with a QDeclarativeView.
#ifdef QUICK_TEST_SCENEGRAPH
    if (qtQuick2) {
        QQuickView view;
        QTestRootObject rootobj;
        QEventLoop eventLoop;
        QObject::connect(view.engine(), SIGNAL(quit()),
                         &rootobj, SLOT(quit()));
        QObject::connect(view.engine(), SIGNAL(quit()),
                         &eventLoop, SLOT(quit()));
        view.rootContext()->setContextProperty
            (QLatin1String("qtest"), &rootobj);
        foreach (const QString &path, imports)
            view.engine()->addImportPath(path);

        foreach (QString file, files) {
            QFileInfo fi(file);
            if (!fi.exists())
                continue;

            rootobj.setHasTestCase(false);
            rootobj.setWindowShown(false);
            rootobj.hasQuit = false;
            QString path = fi.absoluteFilePath();
            if (path.startsWith(QLatin1String(":/")))
                view.setSource(QUrl(QLatin1String("qrc:") + path.mid(2)));
            else
                view.setSource(QUrl::fromLocalFile(path));

            if (QTest::printAvailableFunctions)
                continue;
            if (view.status() == QQuickView::Error) {
                handleCompileErrors(fi, view);
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
                if (!rootobj.hasQuit && rootobj.hasTestCase())
                    eventLoop.exec();
            }
        }
    } else
#endif
    {
        qWarning("No suitable QtQuick1 implementation is available!");
        return 1;
    }

    // Flush the current logging stream.
    QuickTestResult::setProgramName(0);

    saveCoverageTool(argv[0], QuickTestResult::exitCode() != 0);

    //Sometimes delete app cause crash here with some qpa plugins,
    //so we comment the follow line out to make them happy.
    //delete app;

    // Return the number of failures as the exit code.
    return QuickTestResult::exitCode();
}

QT_END_NAMESPACE

#include "quicktest.moc"
