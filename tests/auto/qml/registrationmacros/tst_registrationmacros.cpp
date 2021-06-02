/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
