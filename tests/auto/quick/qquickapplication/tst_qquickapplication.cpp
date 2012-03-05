/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/qinputmethod.h>

class tst_qquickapplication : public QObject
{
    Q_OBJECT
public:
    tst_qquickapplication();

private slots:
    void active();
    void layoutDirection();
    void inputPanel();
    void inputMethod();

private:
    QQmlEngine engine;
};

tst_qquickapplication::tst_qquickapplication()
{
}

void tst_qquickapplication::active()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property bool active: Qt.application.active }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // not active
    QVERIFY(!item->property("active").toBool());
    QCOMPARE(item->property("active").toBool(), QGuiApplication::activeWindow() != 0);

    // active
    view.show();
    view.requestActivateWindow();
    QTest::qWait(50);
    QEXPECT_FAIL("", "QTBUG-21573", Abort);
    QTRY_COMPARE(view.status(), QQuickView::Ready);
    QCOMPARE(item->property("active").toBool(), QGuiApplication::activeWindow() != 0);

#if 0
    // QGuiApplication has no equivalent of setActiveWindow(0). QTBUG-21573
    // Is this different to clearing the active state of the window or can it be removed?
    // On Mac, setActiveWindow(0) on mac does not deactivate the current application,
    // must switch to a different app or hide the current app to trigger this
    // on mac, setActiveWindow(0) on mac does not deactivate the current application
    // (you have to switch to a different app or hide the current app to trigger this)

    // not active again
    QGuiApplication::setActiveWindow(0);
    QVERIFY(!item->property("active").toBool());
    QCOMPARE(item->property("active").toBool(), QGuiApplication::activeWindow() != 0);
#endif

}

void tst_qquickapplication::layoutDirection()
{

    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property bool layoutDirection: Qt.application.layoutDirection }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // not mirrored
    QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::LeftToRight);

    // mirrored
    QGuiApplication::setLayoutDirection(Qt::RightToLeft);
    QEXPECT_FAIL("", "QTBUG-21573", Abort);
    QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::RightToLeft);

    // not mirrored again
    QGuiApplication::setLayoutDirection(Qt::LeftToRight);
    QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::LeftToRight);
}

void tst_qquickapplication::inputPanel()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property variant inputPanel: Qt.application.inputPanel }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // check that the inputPanel property maches with application's input panel
    QCOMPARE(qvariant_cast<QObject*>(item->property("inputPanel")), qApp->inputMethod());
}

void tst_qquickapplication::inputMethod()
{
    // technically not in QQuickApplication, but testing anyway here
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property variant inputMethod: Qt.inputMethod }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // check that the inputMethod property maches with application's input method
    QCOMPARE(qvariant_cast<QObject*>(item->property("inputMethod")), qApp->inputMethod());
}


QTEST_MAIN(tst_qquickapplication)

#include "tst_qquickapplication.moc"
