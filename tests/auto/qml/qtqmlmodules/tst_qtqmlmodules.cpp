// Copyright (C) 2016 Research in Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qtqmlmodules : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qtqmlmodules() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void baseTypes();
    void modelsTypes();
    void unavailableTypes();
};

void tst_qtqmlmodules::baseTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("base.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QVERIFY(object->property("success").toBool());

    delete object;
}

void tst_qtqmlmodules::modelsTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("models.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QVERIFY(object->property("success").toBool());

    delete object;
}

void tst_qtqmlmodules::unavailableTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("unavailable.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QVERIFY(object->property("success").toBool());

    delete object;
}

QTEST_MAIN(tst_qtqmlmodules)

#include "tst_qtqmlmodules.moc"
