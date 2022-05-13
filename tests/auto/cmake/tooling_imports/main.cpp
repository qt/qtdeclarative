// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtQml>
#include <QtTest>

class tst_cmake_qmllint : public QObject
{
    Q_OBJECT
private slots:
    void canImport();
};

void tst_cmake_qmllint::canImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/main.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(!obj.isNull(), qPrintable(component.errorString()));
    QCOMPARE(obj->property("result").toInt(), 97);
}

QTEST_MAIN(tst_cmake_qmllint)

#include "main.moc"
