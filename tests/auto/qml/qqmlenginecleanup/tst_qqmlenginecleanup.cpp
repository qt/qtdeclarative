// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QObject>
#include <QtQml/qqml.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <private/qhashedstring_p.h>
#include <private/qqmlmetatype_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

//Separate test, because if engine cleanup attempts fail they can easily break unrelated tests
class tst_qqmlenginecleanup : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlenginecleanup() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void test_qmlClearTypeRegistrations();
    void test_valueTypeProviderModule(); // QTBUG-43004
    void test_customModuleCleanup();
};

// A wrapper around QQmlComponent to ensure the temporary reference counts
// on the type data as a result of the main thread <> loader thread communication
// are dropped. Regular Synchronous loading will leave us with an event posted
// to the gui thread and an extra refcount that will only be dropped after the
// event delivery. A plain sendPostedEvents() however is insufficient because
// we can't be sure that the event is posted after the constructor finished.
class CleanlyLoadingComponent : public QQmlComponent
{
public:
    CleanlyLoadingComponent(QQmlEngine *engine, const QUrl &url)
        : QQmlComponent(engine, url, QQmlComponent::Asynchronous)
    { waitForLoad(); }
    CleanlyLoadingComponent(QQmlEngine *engine, const QString &fileName)
        : QQmlComponent(engine, fileName, QQmlComponent::Asynchronous)
    { waitForLoad(); }

    void waitForLoad()
    {
        QTRY_VERIFY(status() == QQmlComponent::Ready || status() == QQmlComponent::Error);
    }
};

void tst_qqmlenginecleanup::test_qmlClearTypeRegistrations()
{
    //Test for preventing memory leaks is in tests/manual/qmltypememory
    std::unique_ptr<QQmlEngine> engine;
    std::unique_ptr<CleanlyLoadingComponent> component;
    QUrl testFile = testFileUrl("types.qml");

    const auto qmlTypeForTestType = []() {
        return QQmlMetaType::qmlType(QStringLiteral("TestTypeCpp"), QStringLiteral("Test"),
                                     QTypeRevision::fromVersion(2, 0));
    };

    QVERIFY(!qmlTypeForTestType().isValid());
    qmlRegisterType<QObject>("Test", 2, 0, "TestTypeCpp");
    QVERIFY(qmlTypeForTestType().isValid());

    engine = std::make_unique<QQmlEngine>();
    component = std::make_unique<CleanlyLoadingComponent>(engine.get(), testFile);
    QVERIFY(component->isReady());

    component.reset();
    engine.reset();

    {
        auto cppType = qmlTypeForTestType();

        qmlClearTypeRegistrations();
        QVERIFY(!qmlTypeForTestType().isValid());

        // cppType should hold the last ref, qmlClearTypeRegistration should have wiped
        // all internal references.
        QCOMPARE(QQmlType::refCount(cppType.priv()), 1);
    }

    //2nd run verifies that types can reload after a qmlClearTypeRegistrations
    qmlRegisterType<QObject>("Test", 2, 0, "TestTypeCpp");
    QVERIFY(qmlTypeForTestType().isValid());
    engine = std::make_unique<QQmlEngine>();
    component = std::make_unique<CleanlyLoadingComponent>(engine.get(), testFile);
    QVERIFY(component->isReady());

    component.reset();
    engine.reset();
    qmlClearTypeRegistrations();
    QVERIFY(!qmlTypeForTestType().isValid());

    //3nd run verifies that TestTypeCpp is no longer registered
    engine = std::make_unique<QQmlEngine>();
    component = std::make_unique<CleanlyLoadingComponent>(engine.get(), testFile);
    QVERIFY(component->isError());
    QCOMPARE(component->errorString(),
            testFile.toString() +":8 module \"Test\" is not installed\n");
}

static void cleanState(QQmlEngine **e)
{
    delete *e;
    qmlClearTypeRegistrations();
    *e = new QQmlEngine;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

void tst_qqmlenginecleanup::test_valueTypeProviderModule()
{
    // this test ensures that a module which installs a value type
    // provider can be reinitialized after multiple calls to
    // qmlClearTypeRegistrations() without causing cycles in the
    // value type provider list.
    QQmlEngine *e = nullptr;
    QUrl testFile1 = testFileUrl("testFile1.qml");
    QUrl testFile2 = testFileUrl("testFile2.qml");
    bool noCycles = false;
    for (int i = 0; i < 20; ++i) {
        cleanState(&e);
        QQmlComponent c(e, this);
        c.loadUrl(i % 2 == 0 ? testFile1 : testFile2); // this will hang if cycles exist.
    }
    delete e;
    e = nullptr;
    noCycles = true;
    QVERIFY(noCycles);

    // this test ensures that no crashes occur due to using
    // a dangling QQmlType pointer in the type compiler
    // which results from qmlClearTypeRegistrations()
    QUrl testFile3 = testFileUrl("testFile3.qml");
    bool noDangling = false;
    for (int i = 0; i < 20; ++i) {
        cleanState(&e);
        QQmlComponent c(e, this);
        c.loadUrl(i % 2 == 0 ? testFile1 : testFile3); // this will crash if dangling ptr exists.
    }
    delete e;
    noDangling = true;
    QVERIFY(noDangling);
}

static QByteArray msgModuleCleanupFail(int attempt, const QQmlComponent &c)
{
    return "Attempt #" + QByteArray::number(attempt) + " :"
           + c.errorString().toUtf8();
}

void tst_qqmlenginecleanup::test_customModuleCleanup()
{
    for (int i = 0; i < 5; ++i) {
        qmlClearTypeRegistrations();

        QQmlEngine engine;
        engine.setOutputWarningsToStandardError(true);
        engine.addImportPath(QT_TESTCASE_BUILDDIR);

        QQmlComponent component(&engine);
        component.setData("import CustomModule 1.0\nModuleType {}", QUrl());
        QVERIFY2(component.status() == QQmlComponent::Ready,
                 msgModuleCleanupFail(i, component).constData());

        QScopedPointer<QObject> object(component.create());
        QVERIFY2(!object.isNull(), msgModuleCleanupFail(i, component).constData());
    }
}

QTEST_MAIN(tst_qqmlenginecleanup)

#include "tst_qqmlenginecleanup.moc"
