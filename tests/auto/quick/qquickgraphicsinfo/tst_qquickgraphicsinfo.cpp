/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    view.setSource(QUrl("data/basic.qml"));

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
