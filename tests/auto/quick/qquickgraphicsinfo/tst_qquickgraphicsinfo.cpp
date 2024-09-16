// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qsgrendererinterface.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_QQuickGraphicsInfo : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickGraphicsInfo();

private slots:
    void testProperties();
};

tst_QQuickGraphicsInfo::tst_QQuickGraphicsInfo()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickGraphicsInfo::testProperties()
{
    QQuickView view;
    view.setSource(testFileUrl("basic.qml"));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(&view, SIGNAL(sceneGraphInitialized()));
    spy.wait();

    QObject* obj = view.rootObject();
    QVERIFY(obj);

    QSGRendererInterface *rif = view.rendererInterface();
    const int expectedAPI = rif ? rif->graphicsApi() : QSGRendererInterface::Unknown;

    QCOMPARE(obj->property("api").toInt(), expectedAPI);
}

QTEST_MAIN(tst_QQuickGraphicsInfo)

#include "tst_qquickgraphicsinfo.moc"
