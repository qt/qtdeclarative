// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlFileSelector>
#include <QQmlApplicationEngine>
#include <QFileSelector>
#include <QQmlContext>
#include <QLoggingCategory>
#include <qqmlinfo.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlfileselector : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlfileselector() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void basicTest();
    void basicTestCached();
    void applicationEngineTest();
    void qmldirCompatibility();
    void selectorOwnership_data();
    void selectorOwnership();
};

// A QFileSelector that reports its own destruction, so that ownership can be
// asserted without relying on a sanitizer to notice a wrong answer.
class DestructionWatcher : public QFileSelector
{
public:
    explicit DestructionWatcher(bool *destroyed) : m_destroyed(destroyed) {}
    ~DestructionWatcher() override { *m_destroyed = true; }

private:
    bool *m_destroyed;
};

void tst_qqmlfileselector::basicTest()
{
    QQmlEngine engine;
    QQmlFileSelector selector(&engine);
    selector.setExtraSelectors(QStringList() << "basic");

    QQmlComponent component(&engine, testFileUrl("basicTest.qml"));
    std::unique_ptr<QObject> object { component.create() };
    QVERIFY(object.get() != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type == QtDebugMsg
            && QByteArray(context.category) == QByteArray("qt.qml.diskcache")
            && message.contains("QML source file has moved to a different location.")) {
        QFAIL(message.toUtf8());
    }
}

void tst_qqmlfileselector::basicTestCached()
{
    basicTest(); // Seed the cache, in case basicTestCached() is run on its own
    QtMessageHandler defaultHandler = qInstallMessageHandler(&messageHandler);
    QLoggingCategory::setFilterRules("qt.qml.diskcache.debug=true");
    basicTest(); // Run again and check that the file is in the cache now
    QLoggingCategory::setFilterRules(QString());
    qInstallMessageHandler(defaultHandler);
}

void tst_qqmlfileselector::applicationEngineTest()
{
    QQmlApplicationEngine engine;
    engine.setExtraFileSelectors(QStringList() << "basic");
    engine.load(testFileUrl("basicTest.qml"));

    QVERIFY(!engine.rootObjects().isEmpty());
    QObject *object = engine.rootObjects().at(0);
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));
}

void tst_qqmlfileselector::qmldirCompatibility()
{
    {
        // No error for multiple files with different selectors, and the matching one is chosen
        // for +macos and +linux selectors.
        QQmlApplicationEngine engine;
        engine.addImportPath(dataDirectory());
        engine.load(testFileUrl("qmldirtest/main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *object = engine.rootObjects().at(0);
        auto color = object->property("color").value<QColor>();
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        QCOMPARE(object->objectName(), "linux");
        QCOMPARE(color, QColorConstants::Svg::blue);
#elif defined(Q_OS_DARWIN)
        QCOMPARE(object->objectName(), "macos");
        QCOMPARE(color, QColorConstants::Svg::yellow);
#else
        QCOMPARE(object->objectName(), "base");
        QCOMPARE(color, QColorConstants::Svg::green);
#endif
    }

    {
        // If nothing matches, the _base_ file is chosen, not the first or the last one.
        // This also holds when using the implicit import.
        QQmlApplicationEngine engine;
        engine.addImportPath(dataDirectory());
        engine.load(testFileUrl("qmldirtest2/main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *object = engine.rootObjects().at(0);
        QCOMPARE(object->property("color").value<QColor>(), QColorConstants::Svg::green);
        QCOMPARE(object->objectName(), "base");
    }
}

enum SetSelectorOp { SetExternal, SetDefault };

void tst_qqmlfileselector::selectorOwnership_data()
{
    QTest::addColumn<QList<int>>("ops");
    QTest::addColumn<bool>("finalIsExternal");

    // The internal QFileSelector is owned and unparented; an externally supplied one is
    // neither owned nor touched. Each row drives QQmlFileSelector through a sequence of
    // setSelector() calls and then destroys it.
    QTest::newRow("internal only")              << QList<int>{}                                  << false;
    QTest::newRow("external")                   << QList<int>{SetExternal}                       << true;
    QTest::newRow("external, then default")     << QList<int>{SetExternal, SetDefault}           << false;
    QTest::newRow("default while default")      << QList<int>{SetDefault}                        << false;
    QTest::newRow("external twice")             << QList<int>{SetExternal, SetExternal}          << true;
    QTest::newRow("external, default, external")<< QList<int>{SetExternal, SetDefault, SetExternal} << true;
}

void tst_qqmlfileselector::selectorOwnership()
{
    QFETCH(QList<int>, ops);
    QFETCH(bool, finalIsExternal);

    QQmlEngine engine;
    bool externalDestroyed = false;
    DestructionWatcher external(&externalDestroyed);

    {
        QQmlFileSelector fileSelector(&engine);

        // A freshly constructed QQmlFileSelector owns an internal selector, and that
        // selector must not be parented to it: the private deletes it directly, so a
        // parent would delete it a second time via QObject::~QObject().
        QVERIFY(fileSelector.selector());
        QCOMPARE(fileSelector.selector()->parent(), nullptr);

        for (int op : ops) {
            switch (op) {
            case SetExternal:
                fileSelector.setSelector(&external);
                QCOMPARE(fileSelector.selector(), &external);
                break;
            case SetDefault:
                fileSelector.setSelector(nullptr);
                QVERIFY(fileSelector.selector());
                QVERIFY(fileSelector.selector() != &external);
                // Same invariant as above: whichever path produced the internal
                // selector, it is unparented.
                QCOMPARE(fileSelector.selector()->parent(), nullptr);
                break;
            }
        }

        if (finalIsExternal)
            QCOMPARE(fileSelector.selector(), &external);
        else
            QVERIFY(fileSelector.selector() != &external);

        QVERIFY(!externalDestroyed);
    }

    // Destroying the QQmlFileSelector must never destroy a selector it does not own,
    // and must destroy the one it does own exactly once. The latter is observable as a
    // double free under a sanitizer; the parent() checks above catch the regression
    // that caused it in a plain build too.
    QVERIFY(!externalDestroyed);
}

QTEST_MAIN(tst_qqmlfileselector)

#include "tst_qqmlfileselector.moc"
