/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

#include <QtLabsTemplates/private/qquickapplicationwindow_p.h>
#include <QtLabsTemplates/private/qquickoverlay_p.h>
#include <QtLabsTemplates/private/qquickpopup_p.h>
#include <QtLabsTemplates/private/qquickbutton_p.h>

using namespace QQuickVisualTestUtil;

class tst_popup : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void visible();
    void overlay();
    void closePolicy_data();
    void closePolicy();
};

void tst_popup::visible()
{
    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = helper.window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    QQuickItem *popupItem = popup->popupItem();

    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(window->overlay()->childItems().contains(popupItem));

    popup->close();
    QVERIFY(!popup->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(popupItem));

    popup->setVisible(true);
    QVERIFY(popup->isVisible());
    QVERIFY(window->overlay()->childItems().contains(popupItem));

    popup->setVisible(false);
    QVERIFY(!popup->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(popupItem));
}

void tst_popup::overlay()
{
    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *overlay = window->overlay();
    QSignalSpy overlayPressedSignal(overlay, SIGNAL(pressed()));
    QSignalSpy overlayReleasedSignal(overlay, SIGNAL(released()));
    QVERIFY(overlayPressedSignal.isValid());
    QVERIFY(overlayReleasedSignal.isValid());

    QTest::mousePress(window, Qt::LeftButton);
    QCOMPARE(overlayPressedSignal.count(), 1);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QTest::mouseRelease(window, Qt::LeftButton);
    QCOMPARE(overlayPressedSignal.count(), 1);
    QCOMPARE(overlayReleasedSignal.count(), 0); // no modal popups open

    QQuickPopup *popup = helper.window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QQuickButton *button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    popup->setModal(true);

    popup->open();
    QVERIFY(popup->isVisible());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 2);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 2);
    QCOMPARE(overlayReleasedSignal.count(), 1);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x() + popup->x() + popup->width() / 2,
                                                                     button->y() + popup->y() + popup->height() / 2));
    QCOMPARE(overlayPressedSignal.count(), 2);
    QCOMPARE(overlayReleasedSignal.count(), 1);

    QVERIFY(!popup->isVisible());
}

Q_DECLARE_METATYPE(QQuickPopup::ClosePolicy)

void tst_popup::closePolicy_data()
{
    qRegisterMetaType<QQuickPopup::ClosePolicy>();

    QTest::addColumn<QQuickPopup::ClosePolicy>("closePolicy");

    QTest::newRow("NoAutoClose") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::NoAutoClose);
    QTest::newRow("OnPressOutside") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnPressOutside);
    QTest::newRow("OnPressOutsideParent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnPressOutsideParent);
    QTest::newRow("OnPressOutside|Parent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnPressOutside | QQuickPopup::OnPressOutsideParent);
    QTest::newRow("OnReleaseOutside") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnReleaseOutside);
    QTest::newRow("OnReleaseOutside|Parent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnReleaseOutside | QQuickPopup::OnReleaseOutsideParent);
    QTest::newRow("OnEscape") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::OnEscape);
}

void tst_popup::closePolicy()
{
    QFETCH(QQuickPopup::ClosePolicy, closePolicy);

    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = helper.window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QQuickButton *button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    popup->setModal(true);
    popup->setFocus(true);
    popup->setClosePolicy(closePolicy);

    popup->open();
    QVERIFY(popup->isVisible());

    // press outside popup and its parent
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    if (closePolicy.testFlag(QQuickPopup::OnPressOutside) || closePolicy.testFlag(QQuickPopup::OnPressOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // release outside popup and its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    if (closePolicy.testFlag(QQuickPopup::OnReleaseOutside))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // press outside popup but inside its parent
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x(), button->y()));
    if (closePolicy.testFlag(QQuickPopup::OnPressOutside) && !closePolicy.testFlag(QQuickPopup::OnPressOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // release outside popup but inside its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x(), button->y()));
    if (closePolicy.testFlag(QQuickPopup::OnReleaseOutside) && !closePolicy.testFlag(QQuickPopup::OnReleaseOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // escape
    QTest::keyClick(window, Qt::Key_Escape);
    if (closePolicy.testFlag(QQuickPopup::OnEscape))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());
}

QTEST_MAIN(tst_popup)

#include "tst_popup.moc"
