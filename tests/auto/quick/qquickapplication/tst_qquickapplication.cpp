/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include <qpa/qwindowsysteminterface.h>

class tst_qquickapplication : public QObject
{
    Q_OBJECT
public:
    tst_qquickapplication();

private slots:
    void active();
    void layoutDirection();
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
    component.setData("import QtQuick 2.0; "
                      "Item { "
                      "    property bool active: Qt.application.active; "
                      "    property bool active2: false; "
                      "    Connections { "
                      "        target: Qt.application; "
                      "        onActiveChanged: active2 = Qt.application.active; "
                      "    } "
                      "}", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickWindow window;
    item->setParentItem(window.contentItem());

    // not active
    QVERIFY(!item->property("active").toBool());
    QVERIFY(!item->property("active2").toBool());

    // active
    window.show();
    window.requestActivate();
    QTest::qWaitForWindowActive(&window);
    QVERIFY(QGuiApplication::focusWindow() == &window);
    QVERIFY(item->property("active").toBool());
    QVERIFY(item->property("active2").toBool());

    // not active again
    QWindowSystemInterface::handleWindowActivated(0);

    QTRY_VERIFY(QGuiApplication::focusWindow() != &window);
    QVERIFY(!item->property("active").toBool());
    QVERIFY(!item->property("active2").toBool());
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
