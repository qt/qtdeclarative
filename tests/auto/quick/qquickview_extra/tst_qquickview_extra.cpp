/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/qqmlengine.h>
#include "../../shared/util.h"
#include <QtCore/QDebug>
#include <QtCore/QTimer>

// Extra app-less tests
class tst_QQuickViewExtra : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickViewExtra();

private slots:
    void qtbug_87228();
};

tst_QQuickViewExtra::tst_QQuickViewExtra() { }

void tst_QQuickViewExtra::qtbug_87228()
{
    QScopedPointer<QSignalSpy> deletionSpy;
    {
        int argc = 0;
        QGuiApplication app(argc, nullptr);
        QQuickView view;

        view.setSource(testFileUrl("qtbug_87228.qml"));
        view.show();
        QTimer::singleShot(500, &app, QCoreApplication::quit);
        app.exec();

        QObject *listView = view.findChild<QObject *>("listView");
        QVERIFY(listView);
        QQuickItem *contentItem = listView->property("contentItem").value<QQuickItem *>();
        QVERIFY(contentItem);
        auto children = contentItem->childItems();
        QVERIFY(children.size() > 0);
        // for the sake of this test, any child would be suitable, so pick first
        deletionSpy.reset(new QSignalSpy(children[0], SIGNAL(destroyed(QObject *))));
    }
    QCOMPARE(deletionSpy->count(), 1);
}

QTEST_APPLESS_MAIN(tst_QQuickViewExtra)

#include "tst_qquickview_extra.moc"
