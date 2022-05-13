// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtQml>
#include <QtTest>

class tst_QQC : public QObject
{
    Q_OBJECT
private slots:
    void staticBuildTest();
};

void tst_QQC::staticBuildTest()
{
#ifdef QT_STATIC
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/main.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("success").toInt(), 42);
#endif
}

QTEST_MAIN(tst_QQC)

#include "main.moc"
