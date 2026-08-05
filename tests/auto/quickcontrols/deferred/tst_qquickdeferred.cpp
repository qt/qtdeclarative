// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qtest.h>
#include <QQmlEngine>
#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>
#include <QQmlIncubator>
#include <QtQml/private/qqmlengine_p.h>
#include <QQmlParserStatus>

class DeferredPropertyTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *objectProperty READ objectProperty WRITE setObjectProperty NOTIFY objectChanged)
    Q_CLASSINFO("DeferredPropertyNames", "objectProperty")

public:
    DeferredPropertyTester() {}


    QQuickItem *objectProperty() {

        if (!m_object.wasExecuted()) {
            quickBeginDeferred(this, "objectProperty", m_object);
            quickCompleteDeferred(this, "objectProperty", m_object);
        }

        return m_object;
    }
    void setObjectProperty(QQuickItem *obj) {
        if (m_object == obj)
            return;
        m_object = obj;
        if (!m_object.isExecuting()) // first read
            emit objectChanged();
    }

signals:
    void objectChanged();

private:
    QQuickDeferredPointer<QQuickItem> m_object = nullptr;
};

// Mirrors how QQuickControl handles its deferred contentItem.
class ReentrantDeferredTester : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQuickItem *objectProperty READ objectProperty WRITE setObjectProperty NOTIFY objectChanged)
    Q_CLASSINFO("DeferredPropertyNames", "objectProperty")

public:
    void classBegin() override { m_object = new QQuickItem; } // the default a style would set
    void componentComplete() override { execute(true); }

    QQuickItem *objectProperty()
    {
        execute(false);
        return m_object;
    }

    void setObjectProperty(QQuickItem *obj)
    {
        if (m_object == obj)
            return;
        m_object = obj;

        // Stands in for setContentItem_helper(), where writing the property updates the implicit
        // size, which notifies observers, and a binding among them reads the property back.
        if (m_object.isExecuting())
            objectProperty();
        else
            emit objectChanged();
    }

signals:
    void objectChanged();

private:
    void execute(bool complete)
    {
        if (m_object.wasExecuted())
            return;
        if (!m_object || complete)
            quickBeginDeferred(this, "objectProperty", m_object);
        if (complete)
            quickCompleteDeferred(this, "objectProperty", m_object);
    }

    QQuickDeferredPointer<QQuickItem> m_object = nullptr;
};

class tst_qquickdeferred : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdeferred() : QQmlDataTest(QT_QMLTEST_DATADIR) {}
private slots:
    void noSpuriousBinding();
    void abortedIncubation();
    void reentrantExecution();
};



void tst_qquickdeferred::noSpuriousBinding() {
    QTest::failOnWarning(QRegularExpression(QLatin1StringView(".*Binding loop detected.*")));
    qmlRegisterType<DeferredPropertyTester>("test", 1, 0, "DeferredPropertyTester");

    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("noSpuriousBinding.qml"));
    std::unique_ptr<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));
    root->setProperty("toggle", false);
}

// QTBUG-116828
// This test checks the case where we cancel incubation of a componet with a deferred property
// Components that have deferred properties should also provide an itemDestoryed method that
// that resets the deferred property to null to prevent issues with dangling pointers.
void tst_qquickdeferred::abortedIncubation()
{
    QQmlEngine engine;
    QQmlIncubationController controller;
    engine.setIncubationController(&controller);

    {
        QQmlIncubator incubator;
        QQmlComponent componet(&engine, testFileUrl("abortedIncubation.qml"));
        componet.create(incubator);
        controller.incubateFor(1);
        incubator.clear(); // abort incubation (and dont crash)
    }
}

// Reading the property back while it is being populated must not start a second deferred
// creation: the outer one would be orphaned and never completed.
void tst_qquickdeferred::reentrantExecution()
{
    qmlRegisterType<ReentrantDeferredTester>("test", 1, 0, "ReentrantDeferredTester");

    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("reentrantExecution.qml"));
    std::unique_ptr<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    QCOMPARE(QQmlEnginePrivate::get(&engine)->inProgressCreations, 0);
}

QTEST_MAIN(tst_qquickdeferred)

#include "tst_qquickdeferred.moc"
