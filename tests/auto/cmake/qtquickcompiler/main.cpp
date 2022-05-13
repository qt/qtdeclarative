// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtQml>
#include <QtTest>

class tst_QQC : public QObject
{
    Q_OBJECT
private slots:
    void packaging();
};

void tst_QQC::packaging()
{
    QVERIFY(QFile::exists(":/Test/main.qml"));
    QVERIFY(QFileInfo(":/Test/main.qml").size() > 0);
    QVERIFY(QFile::exists(":/Test/main.cpp"));
    QVERIFY(QFileInfo(":/Test/main.cpp").size() > 0);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/Test/main.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("success").toInt(), 42);
}

QTEST_MAIN(tst_QQC)

#include "main.moc"
