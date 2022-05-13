// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>

#include "types.h"

class tst_registrationmacros : public QObject
{
  Q_OBJECT

  private slots:
    void checkExtraVersions_data();
    void checkExtraVersions();
};



void tst_registrationmacros::checkExtraVersions_data()
{
    QTest::addColumn<int>("major");
    QTest::addColumn<int>("minor");
    QTest::addColumn<bool>("shouldWork");

    QTest::addRow("1.1") << 1 << 1 << true;
    QTest::addRow("2.0") << 2 << 0 << true;
    QTest::addRow("3.0") << 3 << 0 << true;
    QTest::addRow("6.2") << 6 << 2 << true;
}

void tst_registrationmacros::checkExtraVersions()
{
    QFETCH(int, minor);
    QFETCH(int, major);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    QString data = QString::asprintf("import test %d.%d\n Test {}", major, minor);

    component.setData(data.toUtf8(), QUrl());
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QVERIFY(qobject_cast<Test *>(root.get()));
}

QTEST_GUILESS_MAIN(tst_registrationmacros)
#include "tst_registrationmacros.moc"
