// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quicktest_p.h"
#include "quicktestresult_p.h"
#include <QtTest/qtestsystem.h>
#include "qtestoptions_p.h"
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qqmlpropertymap.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/qquickitem.h>
#include <qopengl.h>
#include <QtCore/qurl.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qtimer.h>
#include <QtGui/qtextdocument.h>
#include <stdio.h>
#include <QtGui/QGuiApplication>
#include <QtCore/QTranslator>
#include <QtTest/QSignalSpy>
#include <QtQml/QQmlFileSelector>

#include <private/qqmlcomponent_p.h>
#include <private/qv4resolvedtypereference_p.h>

QT_BEGIN_NAMESPACE

/*!
    \since 5.13

    Returns \c true if \l {QQuickItem::}{updatePolish()} has not been called
    on \a item since the last call to \l {QQuickItem::}{polish()},
    otherwise returns \c false.

    When assigning values to properties in QML, any layouting the item
    must do as a result of the assignment might not take effect immediately,
    but can instead be postponed until the item is polished. For these cases,
    you can use this function to ensure that the item has been polished
    before the execution of the test continues. For example:

    \code
        QVERIFY(QQuickTest::qIsPolishScheduled(item));
        QVERIFY(QQuickTest::qWaitForItemPolished(item));
    \endcode

    Without the call to \c qIsPolishScheduled() above, the
    call to \c qWaitForItemPolished() might see that no polish
    was scheduled and therefore pass instantly, assuming that
    the item had already been polished. This function
    makes it obvious why an item wasn't polished and allows tests to
    fail early under such circumstances.

    The QML equivalent of this function is
    \l {TestCase::}{isPolishScheduled()}.

    \sa QQuickItem::polish(), QQuickItem::updatePolish()
*/
bool QQuickTest::qIsPolishScheduled(const QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->polishScheduled;
}

/*!
    \since 6.4
    \overload qIsPolishScheduled()

    Returns \c true if there are any items managed by this window for
    which \c qIsPolishScheduled(item) returns \c true, otherwise
    returns \c false.

    For example, if an item somewhere within the scene may or may not
    be polished, but you need to wait for it if it is, you can use
    the following code:

    \code
        if (QQuickTest::qIsPolishScheduled(window))
            QVERIFY(QQuickTest::qWaitForPolish(window));
    \endcode

    The QML equivalent of this function is
    \l [QML]{TestCase::}{isPolishScheduled()}.

    \sa QQuickItem::polish(), QQuickItem::updatePolish(),
        QQuickTest::qWaitForPolish()
*/
bool QQuickTest::qIsPolishScheduled(const QQuickWindow *window)
{
    return !QQuickWindowPrivate::get(window)->itemsToPolish.isEmpty();
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
#if QT_DEPRECATED_SINCE(6, 4)
/*!
    \since 5.13
    \deprecated [6.4] Use \l qWaitForPolish() instead.

    Waits for \a timeout milliseconds or until
    \l {QQuickItem::}{updatePolish()} has been called on \a item.

    Returns \c true if \c updatePolish() was called on \a item within
    \a timeout milliseconds, otherwise returns \c false.

    The QML equivalent of this function is
    \l {TestCase::}{waitForItemPolished()}.

    \sa QQuickItem::polish(), QQuickItem::updatePolish(),
        QQuickTest::qIsPolishScheduled()
*/
bool QQuickTest::qWaitForItemPolished(const QQuickItem *item, int timeout)
{
    return qWaitForPolish(item, timeout);
}
#endif
#endif

/*!
    \since 6.4

    Waits for \a timeout milliseconds or until
    \l {QQuickItem::}{updatePolish()} has been called on \a item.

    Returns \c true if \c updatePolish() was called on \a item within
    \a timeout milliseconds, otherwise returns \c false.

    \sa QQuickItem::polish(), QQuickItem::updatePolish(),
        QQuickTest::qIsPolishScheduled()
*/
bool QQuickTest::qWaitForPolish(const QQuickItem *item, int timeout)
{
    return QTest::qWaitFor([&]() { return !QQuickItemPrivate::get(item)->polishScheduled; }, timeout);
}

/*!
    \since 6.4

    Waits for \a timeout milliseconds or until \c qIsPolishScheduled(item)
    returns \c false for all items managed by \a window.

    Returns \c true if \c qIsPolishScheduled(item) returns false for all items
    within \a timeout milliseconds, otherwise returns \c false.

    The QML equivalent of this function is
    \l [QML]{TestCase::}{waitForPolish()}.

    \sa QQuickItem::polish(), QQuickItem::updatePolish(),
        QQuickTest::qIsPolishScheduled()
*/
bool QQuickTest::qWaitForPolish(const QQuickWindow *window, int timeout)
{
    return QTest::qWaitFor([&]() { return QQuickWindowPrivate::get(window)->itemsToPolish.isEmpty(); }, timeout);
}

static inline QString stripQuotes(const QString &s)
{
    if (s.size() >= 2 && s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"')))
        return s.mid(1, s.size() - 2);
    else
        return s;
}

static void handleCompileErrors(
        const QFileInfo &fi, const QList<QQmlError> &errors, QQmlEngine *engine,
        QQuickView *view = nullptr)
{
    // Error compiling the test - flag failure in the log and continue.
    QuickTestResult results;
    results.setTestCaseName(fi.baseName());
    results.startLogging();
    results.setFunctionName(QLatin1String("compile"));
    // Verbose warning output of all messages and relevant parameters
    QString message;
    QTextStream str(&message);
    str << "\n  " << QDir::toNativeSeparators(fi.absoluteFilePath()) << " produced "
        << errors.size() << " error(s):\n";
    for (const QQmlError &e : errors) {
        str << "    ";
        if (e.url().isLocalFile()) {
            str << QDir::toNativeSeparators(e.url().toLocalFile());
        } else {
            str << e.url().toString();
        }
        if (e.line() > 0)
            str << ':' << e.line() << ',' << e.column();
        str << ": " << e.description() << '\n';
    }
    str << "  Working directory: " << QDir::toNativeSeparators(QDir::current().absolutePath()) << '\n';
    if (engine) {
        str << "  ";
        if (view)
            str << "View: " << view->metaObject()->className() << ", ";
        str << "Import paths:\n";
        const auto importPaths = engine->importPathList();
        for (const QString &i : importPaths)
            str << "    '" << QDir::toNativeSeparators(i) << "'\n";
        const QStringList pluginPaths = engine->pluginPathList();
        str << "  Plugin paths:\n";
        for (const QString &p : pluginPaths)
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

class SimpleReceiver : public QObject {
    Q_OBJECT
public:
    bool signalReceived = false;
public slots:
    void slotFun() { signalReceived = true; }
};

bool qWaitForSignal(QObject *obj, const char* signal, int timeout)
{
    if (!obj || !signal) {
        qWarning("qWaitForSignal: invalid arguments");
        return false;
    }
    if (((signal[0] - '0') & 0x03) != QSIGNAL_CODE) {
        qWarning("qWaitForSignal: not a valid signal, use the SIGNAL macro");
        return false;
    }

    int sig = obj->metaObject()->indexOfSignal(signal + 1);
    if (sig == -1) {
        const QByteArray ba = QMetaObject::normalizedSignature(signal + 1);
        sig = obj->metaObject()->indexOfSignal(ba.constData());
        if (sig == -1) {
            qWarning("qWaitForSignal: no such signal %s::%s", obj->metaObject()->className(),
                     signal);
            return false;
        }
    }

    SimpleReceiver receiver;
    static int slot = receiver.metaObject()->indexOfSlot("slotFun()");
    if (!QMetaObject::connect(obj, sig, &receiver, slot)) {
        qWarning("qWaitForSignal: failed to connect to signal %s::%s",
                 obj->metaObject()->className(), signal);
        return false;
    }

    return QTest::qWaitFor([&]() { return receiver.signalReceived; }, timeout);
}

template <typename... Args>
void maybeInvokeSetupMethod(QObject *setupObject, const char *member, Args &&... args)
{
    // It's OK if it doesn't exist: since we have more than one callback that
    // can be called, it makes sense if the user only implements one of them.
    // We do this the long way rather than just calling the static
    // QMetaObject::invokeMethod(), because that will issue a warning if the
    // function doesn't exist, which we don't want.
    const QMetaObject *setupMetaObject = setupObject->metaObject();
    const int methodIndex = setupMetaObject->indexOfMethod(member);
    if (methodIndex != -1) {
        const QMetaMethod method = setupMetaObject->method(methodIndex);
        method.invoke(setupObject, std::forward<Args>(args)...);
    }
}

using namespace QV4::CompiledData;

class TestCaseCollector
{
public:
    typedef QList<QString> TestCaseList;

    TestCaseCollector(const QFileInfo &fileInfo, QQmlEngine *engine)
    {
        QString path = fileInfo.absoluteFilePath();
        if (path.startsWith(QLatin1String(":/")))
            path.prepend(QLatin1String("qrc"));

        QQmlComponent component(engine, path);
        m_errors += component.errors();

        if (component.isReady()) {
            QQmlRefPointer<QV4::ExecutableCompilationUnit> rootCompilationUnit
                    = QQmlComponentPrivate::get(&component)->compilationUnit;
            TestCaseEnumerationResult result = enumerateTestCases(rootCompilationUnit.data());
            m_testCases = result.testCases + result.finalizedPartialTestCases();
            m_errors += result.errors;
        }
    }

    TestCaseList testCases() const { return m_testCases; }
    QList<QQmlError> errors() const { return m_errors; }

private:
    TestCaseList m_testCases;
    QList<QQmlError> m_errors;

    struct TestCaseEnumerationResult
    {
        TestCaseList testCases;
        QList<QQmlError> errors;

        // Partially constructed test cases
        bool isTestCase = false;
        TestCaseList testFunctions;
        QString testCaseName;

        TestCaseList finalizedPartialTestCases() const
        {
            TestCaseList result;
            for (const QString &function : testFunctions)
                result << QString(QStringLiteral("%1::%2")).arg(testCaseName).arg(function);
            return result;
        }

        TestCaseEnumerationResult &operator<<(const TestCaseEnumerationResult &other)
        {
            testCases += other.testCases + other.finalizedPartialTestCases();
            errors += other.errors;
            return *this;
        }
    };

    TestCaseEnumerationResult enumerateTestCases(
            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const Object *object = nullptr)
    {
        QQmlType testCaseType;
        for (quint32 i = 0, count = compilationUnit->importCount(); i < count; ++i) {
            const Import *import = compilationUnit->importAt(i);
            if (compilationUnit->stringAt(import->uriIndex) != QLatin1String("QtTest"))
                continue;

            QString testCaseTypeName(QStringLiteral("TestCase"));
            QString typeQualifier = compilationUnit->stringAt(import->qualifierIndex);
            if (!typeQualifier.isEmpty())
                testCaseTypeName = typeQualifier % QLatin1Char('.') % testCaseTypeName;

            testCaseType = compilationUnit->typeNameCache->query(testCaseTypeName).type;
            if (testCaseType.isValid())
                break;
        }

        TestCaseEnumerationResult result;

        if (!object) // Start at root of compilation unit if not enumerating a specific child
            object = compilationUnit->objectAt(0);
        if (object->hasFlag(Object::IsInlineComponentRoot))
            return result;

        if (const auto superTypeUnit = compilationUnit->resolvedTypes.value(
                    object->inheritedTypeNameIndex)->compilationUnit()) {
            // We have a non-C++ super type, which could indicate we're a subtype of a TestCase
            if (testCaseType.isValid() && superTypeUnit->url() == testCaseType.sourceUrl())
                result.isTestCase = true;
            else if (superTypeUnit->url() != compilationUnit->url()) { // urls are the same for inline component, avoid infinite recursion
                result = enumerateTestCases(superTypeUnit);
            }

            if (result.isTestCase) {
                // Look for override of name in this type
                for (auto binding = object->bindingsBegin(); binding != object->bindingsEnd(); ++binding) {
                    if (compilationUnit->stringAt(binding->propertyNameIndex) == QLatin1String("name")) {
                        if (binding->type() == QV4::CompiledData::Binding::Type_String) {
                            result.testCaseName = compilationUnit->stringAt(binding->stringIndex);
                        } else {
                            QQmlError error;
                            error.setUrl(compilationUnit->url());
                            error.setLine(binding->location.line());
                            error.setColumn(binding->location.column());
                            error.setDescription(QStringLiteral("the 'name' property of a TestCase must be a literal string"));
                            result.errors << error;
                        }
                        break;
                    }
                }

                // Look for additional functions in this type
                auto functionsEnd = compilationUnit->objectFunctionsEnd(object);
                for (auto function = compilationUnit->objectFunctionsBegin(object); function != functionsEnd; ++function) {
                    QString functionName = compilationUnit->stringAt(function->nameIndex);
                    if (!(functionName.startsWith(QLatin1String("test_")) || functionName.startsWith(QLatin1String("benchmark_"))))
                        continue;

                    if (functionName.endsWith(QLatin1String("_data")))
                        continue;

                    result.testFunctions << functionName;
                }
            }
        }

        for (auto binding = object->bindingsBegin(); binding != object->bindingsEnd(); ++binding) {
            if (binding->type() == QV4::CompiledData::Binding::Type_Object) {
                const Object *child = compilationUnit->objectAt(binding->value.objectIndex);
                result << enumerateTestCases(compilationUnit, child);
            }
        }

        return result;
    }
};

int quick_test_main(int argc, char **argv, const char *name, const char *sourceDir)
{
    return quick_test_main_with_setup(argc, argv, name, sourceDir, nullptr);
}

int quick_test_main_with_setup(int argc, char **argv, const char *name, const char *sourceDir, QObject *setup)
{
    QScopedPointer<QCoreApplication> app;
    if (!QCoreApplication::instance())
        app.reset(new QGuiApplication(argc, argv));

    if (setup)
        maybeInvokeSetupMethod(setup, "applicationAvailable()");

    // Look for QML-specific command-line options.
    //      -import dir         Specify an import directory.
    //      -plugins dir        Specify a directory where to search for plugins.
    //      -input dir          Specify the input directory for test cases.
    //      -translation file   Specify the translation file.
    //      -file-selector      Specify a file selector
    QStringList imports;
    QStringList pluginPaths;
    QString testPath;
    QString translationFile;
    QStringList fileSelectors;
    int index = 1;
    QScopedArrayPointer<char *> testArgV(new char *[argc + 1]);
    testArgV[0] = argv[0];
    int testArgC = 1;
    while (index < argc) {
        if (strcmp(argv[index], "-import") == 0 && (index + 1) < argc) {
            imports += stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-plugins") == 0 && (index + 1) < argc) {
            pluginPaths += stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-input") == 0 && (index + 1) < argc) {
            testPath = stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-opengl") == 0) {
            ++index;
        } else if (strcmp(argv[index], "-translation") == 0 && (index + 1) < argc) {
            translationFile = stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else if (strcmp(argv[index], "-file-selector") == 0 && (index + 1) < argc) {
            fileSelectors += stripQuotes(QString::fromLocal8Bit(argv[index + 1]));
            index += 2;
        } else {
            testArgV[testArgC++] = argv[index++];
        }
    }
    testArgV[testArgC] = 0;

    // Setting currentAppname and currentTestObjectName (via setProgramName) are needed
    // for the code coverage analysis. Must be done before parseArgs is called.
    QuickTestResult::setCurrentAppname(argv[0]);
    QuickTestResult::setProgramName(name);

    QuickTestResult::parseArgs(testArgC, testArgV.data());

#if QT_CONFIG(translation)
    QTranslator translator;
    if (!translationFile.isEmpty()) {
        if (translator.load(translationFile)) {
            app->installTranslator(&translator);
        } else {
            qWarning("Could not load the translation file '%s'.", qPrintable(translationFile));
        }
    }
#endif

    // Determine where to look for the test data.
    if (testPath.isEmpty() && sourceDir) {
        const QString s = QString::fromLocal8Bit(sourceDir);
        if (QFile::exists(s))
            testPath = s;
    }

#if defined(Q_OS_ANDROID) || defined(Q_OS_INTEGRITY)
            if (testPath.isEmpty())
                    testPath = QLatin1String(":/");
#endif

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
        if (testPath.endsWith(QLatin1String(".qml"))) {
            files << testPath;
        } else if (testPath.endsWith(QLatin1String(".qmltests"))) {
            QFile file(testPath);
            if (file.open(QIODevice::ReadOnly)) {
                while (!file.atEnd()) {
                    const QString filePath = testPathInfo.dir()
                                                     .filePath(QString::fromUtf8(file.readLine()))
                                                     .trimmed();
                    const QFileInfo f(filePath);
                    if (f.exists())
                        files.append(filePath);
                    else
                        qWarning("The test file '%s' does not exists", qPrintable(filePath));
                }
                file.close();
                files.sort();
                if (files.isEmpty()) {
                    qWarning("The file '%s' does not contain any tests files",
                             qPrintable(testPath));
                    return 1;
                }
            } else {
                qWarning("Could not read '%s'", qPrintable(testPath));
            }
        } else {
            qWarning("'%s' does not have the suffix '.qml' or '.qmltests'.", qPrintable(testPath));
            return 1;
        }
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

    qputenv("QT_QTESTLIB_RUNNING", "1");

    QSet<QString> commandLineTestFunctions(QTest::testFunctions.cbegin(), QTest::testFunctions.cend());
    const bool filteringTestFunctions = !commandLineTestFunctions.isEmpty();

    // Scan through all of the "tst_*.qml" files and run each of them
    // in turn with a separate QQuickView (for test isolation).
    for (const QString &file : std::as_const(files)) {
        const QFileInfo fi(file);
        if (!fi.exists())
            continue;

        QQmlEngine engine;
        for (const QString &path : std::as_const(imports))
            engine.addImportPath(path);
        for (const QString &path : std::as_const(pluginPaths))
            engine.addPluginPath(path);

        if (!fileSelectors.isEmpty()) {
            QQmlFileSelector* const qmlFileSelector = new QQmlFileSelector(&engine, &engine);
            qmlFileSelector->setExtraSelectors(fileSelectors);
        }

        // Do this down here so that import paths, plugin paths, file selectors, etc. are available
        // in case the user needs access to them. Do it _before_ the TestCaseCollector parses the
        // QML files though, because it attempts to import modules, which might not be available
        // if qmlRegisterType()/QQmlEngine::addImportPath() are called in qmlEngineAvailable().
        if (setup)
            maybeInvokeSetupMethod(setup, "qmlEngineAvailable(QQmlEngine*)", Q_ARG(QQmlEngine*, &engine));

        TestCaseCollector testCaseCollector(fi, &engine);
        if (!testCaseCollector.errors().isEmpty()) {
            handleCompileErrors(fi, testCaseCollector.errors(), &engine);
            continue;
        }

        TestCaseCollector::TestCaseList availableTestFunctions = testCaseCollector.testCases();
        if (QTest::printAvailableFunctions) {
            for (const QString &function : availableTestFunctions)
                qDebug("%s()", qPrintable(function));
            continue;
        }

        const QSet<QString> availableTestSet(availableTestFunctions.cbegin(), availableTestFunctions.cend());
        if (filteringTestFunctions && !availableTestSet.intersects(commandLineTestFunctions))
            continue;
        commandLineTestFunctions.subtract(availableTestSet);

        QQuickView view(&engine, nullptr);
        view.setFlags(Qt::Window | Qt::WindowSystemMenuHint
                         | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint
                         | Qt::WindowCloseButtonHint);
        QEventLoop eventLoop;
        QObject::connect(view.engine(), SIGNAL(quit()),
                         QTestRootObject::instance(), SLOT(quit()));
        QObject::connect(view.engine(), SIGNAL(quit()),
                         &eventLoop, SLOT(quit()));
        view.rootContext()->setContextProperty
            (QLatin1String("qtest"), QTestRootObject::instance()); // Deprecated. Use QTestRootObject from QtTest instead

        view.setObjectName(fi.baseName());
        view.setTitle(view.objectName());
        QTestRootObject::instance()->init();
        QString path = fi.absoluteFilePath();
        if (path.startsWith(QLatin1String(":/")))
            view.setSource(QUrl(QLatin1String("qrc:") + QStringView{path}.mid(1)));
        else
            view.setSource(QUrl::fromLocalFile(path));

        while (view.status() == QQuickView::Loading)
            QTest::qWait(10);
        if (view.status() == QQuickView::Error) {
            handleCompileErrors(fi, view.errors(), view.engine(), &view);
            continue;
        }

        view.setFramePosition(QPoint(50, 50));
        if (view.size().isEmpty()) { // Avoid hangs with empty windows.
            view.resize(200, 200);
        }
        view.show();
        if (!QTest::qWaitForWindowExposed(&view)) {
            qWarning().nospace()
                << "Test '" << QDir::toNativeSeparators(path) << "' window not exposed after show().";
        }
        view.requestActivate();
        if (!QTest::qWaitForWindowActive(&view)) {
            qWarning().nospace()
                << "Test '" << QDir::toNativeSeparators(path) << "' window not active after requestActivate().";
        }
        if (view.isExposed()) {
            // Defer property update until event loop has started
            QTimer::singleShot(0, []() {
                QTestRootObject::instance()->setWindowShown(true);
            });
        } else {
            qWarning().nospace()
                << "Test '" << QDir::toNativeSeparators(path) << "' window was never exposed! "
                << "If the test case was expecting windowShown, it will hang.";
        }
        if (!QTestRootObject::instance()->hasQuit && QTestRootObject::instance()->hasTestCase())
            eventLoop.exec();
    }

    if (setup)
        maybeInvokeSetupMethod(setup, "cleanupTestCase()");

    // Flush the current logging stream.
    QuickTestResult::setProgramName(nullptr);
    app.reset();

    // Check that all test functions passed on the command line were found
    if (!commandLineTestFunctions.isEmpty()) {
        qWarning() << "Could not find the following test functions:";
        for (const QString &functionName : std::as_const(commandLineTestFunctions))
            qWarning("    %s()", qUtf8Printable(functionName));
        return commandLineTestFunctions.size();
    }

    // Return the number of failures as the exit code.
    return QuickTestResult::exitCode();
}

QT_END_NAMESPACE

#include "moc_quicktest_p.cpp"
#include "quicktest.moc"
