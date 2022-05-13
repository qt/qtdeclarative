// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtTest/qtest.h>

class tst_empty_qmldir : public QObject
{
    Q_OBJECT
private slots:
    void canSelfImport();
};

void tst_empty_qmldir::canSelfImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/flee.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(!obj.isNull(), qPrintable(component.errorString()));
    QCOMPARE(obj->property("success").toInt(), -11);
    QObject *enemy = qvariant_cast<QObject *>(obj->property("other"));
    QVERIFY(enemy != nullptr);
    QCOMPARE(enemy->property("name").toString(), QStringLiteral("evil"));
}

QTEST_MAIN(tst_empty_qmldir)

#include "main.moc"
