// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlEngine>
#include <QQmlComponent>
#include <QtCore/qsysinfo.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlsysteminformation : public QQmlDataTest
{
    Q_OBJECT

public:
    explicit tst_qqmlsysteminformation() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private Q_SLOTS:
    void systemInformation();
};

void tst_qqmlsysteminformation::systemInformation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_systeminformation.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("wordSize").toInt(), QSysInfo::WordSize);
    QCOMPARE(object->property("byteOrder").toInt(), QSysInfo::ByteOrder);
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

QTEST_MAIN(tst_qqmlsysteminformation)

#include "tst_qqmlsysteminformation.moc"
