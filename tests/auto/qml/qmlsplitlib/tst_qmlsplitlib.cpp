// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlEngine>
#include <QQmlComponent>
#include <QObject>
#include <qtest.h>
#include "lib.h"
#include "lib2.h"

class tst_splitlib : public QObject
{
    Q_OBJECT
private slots:
    void verifyComponent();
    void verifyComponent2();
};

void tst_splitlib::verifyComponent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QStringLiteral("qrc:/qt/qml/SplitLib/main.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());
    auto lib = qobject_cast<SplitLib *>(o.get());
    QVERIFY(lib);
}

void tst_splitlib::verifyComponent2()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QStringLiteral("qrc:/qt/qml/SplitLib/main2.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());
    auto lib = qobject_cast<SplitLib2 *>(o.get());
    QVERIFY(lib);
}

QTEST_MAIN(tst_splitlib)
#include "tst_qmlsplitlib.moc"
