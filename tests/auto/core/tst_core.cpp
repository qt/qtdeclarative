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

#include <QQmlEngine>
#include <QQmlComponent>
#include <QtCore/qsysinfo.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_core : public QQmlDataTest
{
    Q_OBJECT

public:
    explicit tst_core() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private Q_SLOTS:
    void systemInformation();
};

void tst_core::systemInformation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_systeminformation.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("buildCpuArchitecture").toString(), QSysInfo::buildCpuArchitecture());
    QCOMPARE(object->property("currentCpuArchitecture").toString(), QSysInfo::currentCpuArchitecture());
    QCOMPARE(object->property("buildAbi").toString(), QSysInfo::buildAbi());
    QCOMPARE(object->property("kernelType").toString(), QSysInfo::kernelType());
    QCOMPARE(object->property("kernelVersion").toString(), QSysInfo::kernelVersion());
    QCOMPARE(object->property("productType").toString(), QSysInfo::productType());
    QCOMPARE(object->property("productVersion").toString(), QSysInfo::productVersion());
    QCOMPARE(object->property("prettyProductName").toString(), QSysInfo::prettyProductName());
    QCOMPARE(object->property("machineHostName").toString(), QSysInfo::machineHostName());
    QCOMPARE(object->property("machineUniqueId").toByteArray(), QSysInfo::machineUniqueId());
    QCOMPARE(object->property("bootUniqueId").toByteArray(), QSysInfo::bootUniqueId());
}

QTEST_MAIN(tst_core)

#include "tst_core.moc"
