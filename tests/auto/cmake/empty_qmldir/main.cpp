/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
