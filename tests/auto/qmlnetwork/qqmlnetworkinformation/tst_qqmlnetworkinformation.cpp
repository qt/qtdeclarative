// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QQmlEngine>
#include <QQmlComponent>
#include <QtNetwork/qnetworkinformation.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlnetworkinformation : public QQmlDataTest
{
    Q_OBJECT

public:
    explicit tst_qqmlnetworkinformation() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

private Q_SLOTS:
    void networkInformation();
};

void tst_qqmlnetworkinformation::networkInformation()
{
    QNetworkInformation::loadDefaultBackend();
    QNetworkInformation *networkinfo = QNetworkInformation::instance();
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN) || defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN)
    QVERIFY(networkinfo);
#else
    QSKIP("Platform does not provide network information");
#endif

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_networkinformation.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("local").toInt(),
             static_cast<int>(QNetworkInformation::Reachability::Local));
    QCOMPARE(object->property("reachability").toInt(),
             static_cast<int>(networkinfo->reachability()));
    QCOMPARE(object->property("isBehindCaptivePortal").toBool(),
             networkinfo->isBehindCaptivePortal());
    QCOMPARE(object->property("ethernet").toInt(),
             static_cast<int>(QNetworkInformation::TransportMedium::Ethernet));
    QCOMPARE(object->property("transportMedium").toInt(),
             static_cast<int>(networkinfo->transportMedium()));
    QCOMPARE(object->property("isMetered").toBool(), networkinfo->isMetered());
}

QTEST_MAIN(tst_qqmlnetworkinformation)

#include "tst_qqmlnetworkinformation.moc"
