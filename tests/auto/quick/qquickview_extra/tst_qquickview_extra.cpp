// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/qqmlengine.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
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

tst_QQuickViewExtra::tst_QQuickViewExtra() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

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
        deletionSpy.reset(new QSignalSpy(children[0], SIGNAL(destroyed(QObject*))));
    }
    QCOMPARE(deletionSpy->size(), 1);
}

QTEST_APPLESS_MAIN(tst_QQuickViewExtra)

#include "tst_qquickview_extra.moc"
