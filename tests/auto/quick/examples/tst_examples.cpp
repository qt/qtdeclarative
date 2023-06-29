// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QLibraryInfo>
#include <QDir>
#include <QDebug>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>

static QtMessageHandler testlibMsgHandler = nullptr;
void msgHandlerFilter(QtMsgType type, const QMessageLogContext &ctxt, const QString &msg)
{
    if (type == QtCriticalMsg || type == QtFatalMsg)
        (*testlibMsgHandler)(type, ctxt, msg);
}

class tst_examples : public QObject
{
    Q_OBJECT
public:
    tst_examples();
    ~tst_examples();

private slots:
    void init();
    void cleanup();

    void examples_data();
    void examples();
    void snippets_data();
    void snippets();

    void namingConvention();
private:
    QStringList excludedDirs;
    QStringList excludedFiles;

    void namingConvention(const QDir &);
    QStringList findQmlFiles(const QDir &);

    QQmlEngine engine;
};

tst_examples::tst_examples()
{
    // Add files to exclude here
    excludedFiles << "snippets/qml/listmodel/listmodel.qml"; //Just a ListModel, no root QQuickItem
    excludedFiles << "snippets/qml/tablemodel/fruit-example-delegatechooser.qml"; // Requires QtQuick.Controls import.

    // Add directories you want excluded here
    excludedDirs << "shared"; //Not an example
    excludedDirs << "snippets/qml/path"; //No root QQuickItem

    // These snippets are not expected to run on their own.
    excludedDirs << "snippets/qml/visualdatamodel_rootindex";
    excludedDirs << "snippets/qml/qtbinding";
    excludedDirs << "snippets/qml/imports";
    excludedDirs << "examples/quickcontrols/imagine";
    excludedDirs << "examples/quickcontrols/texteditor";
    excludedDirs << "examples/quickcontrols/ios/todolist"; // Must be run via executable.
    excludedFiles << "snippets/qml/image-ext.qml";
    excludedFiles << "examples/quick/shapes/content/main.qml"; // relies on resources
    excludedFiles << "examples/quick/shapes/content/interactive.qml"; // relies on resources

#if !QT_CONFIG(opengl)
    //No support for Particles
    excludedFiles << "examples/qml/dynamicscene/dynamicscene.qml";
    excludedFiles << "examples/quick/animation/basics/color-animation.qml";
    excludedFiles << "examples/quick/particles/affectors/content/age.qml";
    excludedFiles << "examples/quick/pointerhandlers/multiflame.qml";
    excludedDirs << "examples/quick/particles";
    // No Support for ShaderEffect
    excludedFiles << "src/quick/doc/snippets/qml/animators.qml";
#endif

}

tst_examples::~tst_examples()
{
}

void tst_examples::init()
{
    if (!qstrcmp(QTest::currentTestFunction(), "snippets"))
        testlibMsgHandler = qInstallMessageHandler(msgHandlerFilter);
}

void tst_examples::cleanup()
{
    if (!qstrcmp(QTest::currentTestFunction(), "snippets"))
        qInstallMessageHandler(testlibMsgHandler);
}

/*
This tests that the examples follow the naming convention required
to have them tested by the examples() test.
*/
void tst_examples::namingConvention(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.size(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return;
    }

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                    QDir::Files);

    bool seenQml = !files.isEmpty();
    bool seenLowercase = false;

    foreach (const QString &file, files) {
        if (file.at(0).isLower())
            seenLowercase = true;
    }

    if (!seenQml) {
        QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                QDir::NoSymLinks);
        foreach (const QString &dir, dirs) {
            QDir sub = d;
            sub.cd(dir);
            namingConvention(sub);
        }
    } else if(!seenLowercase) {
        // QTBUG-28271 don't fail, but rather warn only
        qWarning() << QString(
            "Directory %1 violates naming convention; expected at least one qml file "
            "starting with lower case, got: %2"
        ).arg(d.absolutePath()).arg(files.join(","));

//        QFAIL(qPrintable(QString(
//            "Directory %1 violates naming convention; expected at least one qml file "
//            "starting with lower case, got: %2"
//        ).arg(d.absolutePath()).arg(files.join(","))));
    }
}

void tst_examples::namingConvention()
{
    QStringList examplesLocations;
    examplesLocations << QLibraryInfo::path(QLibraryInfo::ExamplesPath) + QLatin1String("/qml");
    examplesLocations << QLibraryInfo::path(QLibraryInfo::ExamplesPath) + QLatin1String("/quick");

    foreach(const QString &examples, examplesLocations) {
        QDir d(examples);
        if (d.exists())
            namingConvention(d);
    }
}

QStringList tst_examples::findQmlFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.size(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList cppfiles = d.entryList(QStringList() << QLatin1String("*.cpp"), QDir::Files);
    if (cppfiles.isEmpty()) {
        QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                        QDir::Files);
        foreach (const QString &file, files) {
            if (file.at(0).isLower()) {
                bool superContinue = false;
                for (int ii = 0; ii < excludedFiles.size(); ++ii) {
                    QString e = excludedFiles.at(ii);
                    if (d.absoluteFilePath(file).endsWith(e)) {
                        superContinue = true;
                        break;
                    }
                }
                if (superContinue)
                    continue;
                rv << d.absoluteFilePath(file);
            }
        }
    }


    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findQmlFiles(sub);
    }

    return rv;
}

/*
This test runs all the examples in the QtQml UI source tree and ensures
that they start and exit cleanly.

Examples are any .qml files under the examples/ directory that start
with a lower case letter.
*/
void tst_examples::examples_data()
{
#ifdef Q_OS_ANDROID
    QSKIP("tst_examples::examples_data needs adaptions for Android, QTBUG-102858.");
#endif
    QTest::addColumn<QString>("file");

    const QDir repoSourceDir(QLatin1String(SRCDIR) + "/../../../..");
    QVERIFY2(repoSourceDir.exists(), qPrintable(
        QString::fromLatin1("repoSourceDir %1 doesn't exist").arg(repoSourceDir.path())));

    const QDir examplesDir(repoSourceDir.path() + "/examples");
    QVERIFY2(examplesDir.exists(), qPrintable(
        QStringLiteral("examplesDir %1 doesn't exist").arg(examplesDir.path())));

    QStringList files;
    files << findQmlFiles(examplesDir);

    for (const QString &file : std::as_const(files))
        QTest::newRow(qPrintable(repoSourceDir.relativeFilePath(file))) << file;
}

void tst_examples::examples()
{
    QFETCH(QString, file);
    QQuickWindow window;
    window.setPersistentGraphics(true);
    window.setPersistentSceneGraph(true);

    QQmlComponent component(&engine, QUrl::fromLocalFile(file));
    if (component.status() == QQmlComponent::Error)
        qWarning() << component.errors();
    QCOMPARE(component.status(), QQmlComponent::Ready);

    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QQuickItem *root = qobject_cast<QQuickItem *>(object.data());
    if (!root)
        component.completeCreate();
    QVERIFY(root);

    window.resize(240, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    root->setParentItem(window.contentItem());
    component.completeCreate();

    qApp->processEvents();
}

void tst_examples::snippets_data()
{
#ifdef Q_OS_ANDROID
    QSKIP("tst_examples::snippets_data needs adaptions for Android, QTBUG-102858.");
#endif
    QTest::addColumn<QString>("file");

    // Add QML snippets.
    const QDir repoSourceDir(QLatin1String(SRCDIR) + "/../../../..");
    QVERIFY2(repoSourceDir.exists(), qPrintable(
        QStringLiteral("repoSourceDir %1 doesn't exist").arg(repoSourceDir.path())));

    QDir snippetsDir(repoSourceDir.path() + "/src/qml/doc/snippets/qml");
    QVERIFY2(snippetsDir.exists(), qPrintable(
        QStringLiteral("qml snippetsDir %1 doesn't exist").arg(snippetsDir.path())));

    QStringList files;
    files << findQmlFiles(snippetsDir);
    for (const QString &file : std::as_const(files))
        QTest::newRow(qPrintable(repoSourceDir.relativeFilePath(file))) << file;

    // Add Quick snippets.
    snippetsDir = QDir(repoSourceDir.path() + "/src/quick/doc/snippets/qml");
    QVERIFY2(snippetsDir.exists(), qPrintable(
        QStringLiteral("quick snippetsDir %1 doesn't exist").arg(snippetsDir.path())));

    files.clear();
    files << findQmlFiles(snippetsDir);
    for (const QString &file : std::as_const(files))
        QTest::newRow(qPrintable(repoSourceDir.relativeFilePath(file))) << file;
}

void tst_examples::snippets()
{

    QFETCH(QString, file);

    QQmlComponent component(&engine, QUrl::fromLocalFile(file));
    if (component.status() == QQmlComponent::Error)
        qWarning() << component.errors();
    QCOMPARE(component.status(), QQmlComponent::Ready);

    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QQuickWindow *window = qobject_cast<QQuickWindow*>(object.data());
    QQuickItem *root = qobject_cast<QQuickItem *>(object.data());
    if (!root && !window) {
        component.completeCreate();
        QFAIL("No root and no window");
    }
    if (!window)
        window = new QQuickWindow;

    window->resize(240, 320);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    if (root)
        root->setParentItem(window->contentItem());
    component.completeCreate();

    qApp->processEvents();
    if (root)
        delete window;
}

QTEST_MAIN(tst_examples)

#include "tst_examples.moc"
