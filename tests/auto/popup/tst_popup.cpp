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

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

using namespace QQuickVisualTestUtil;

class tst_popup : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void visible();
    void overlay();
    void windowChange();
    void closePolicy_data();
    void closePolicy();
    void activeFocusOnClose1();
    void activeFocusOnClose2();
};

void tst_popup::visible()
{
    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
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
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *overlay = window->overlay();
    QSignalSpy overlayPressedSignal(overlay, SIGNAL(pressed()));
    QSignalSpy overlayReleasedSignal(overlay, SIGNAL(released()));
    QVERIFY(overlayPressedSignal.isValid());
    QVERIFY(overlayReleasedSignal.isValid());

    QVERIFY(!overlay->isVisible()); // no popups open

    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(overlayPressedSignal.count(), 0);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QQuickPopup *popup = helper.window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QQuickButton *button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 1);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 1);
    QCOMPARE(overlayReleasedSignal.count(), 0); // no modal-popups open

    popup->close();
    QVERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());

    popup->setModal(true);

    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 2);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), 2);
    QCOMPARE(overlayReleasedSignal.count(), 1);

    QVERIFY(!popup->isVisible());
    QVERIFY(overlay->isVisible());
}

void tst_popup::windowChange()
{
    QQuickPopup popup;
    QSignalSpy spy(&popup, SIGNAL(windowChanged(QQuickWindow*)));
    QVERIFY(spy.isValid());

    QQuickItem item;
    popup.setParentItem(&item);
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 0);

    QQuickWindow window;
    item.setParentItem(window.contentItem());
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 1);

    item.setParentItem(nullptr);
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 2);

    popup.setParentItem(window.contentItem());
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 3);
}

Q_DECLARE_METATYPE(QQuickPopup::ClosePolicy)

void tst_popup::closePolicy_data()
{
    qRegisterMetaType<QQuickPopup::ClosePolicy>();

    QTest::addColumn<QQuickPopup::ClosePolicy>("closePolicy");

    QTest::newRow("NoAutoClose") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::NoAutoClose);
    QTest::newRow("CloseOnPressOutside") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside);
    QTest::newRow("CloseOnPressOutsideParent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("CloseOnPressOutside|Parent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("CloseOnReleaseOutside") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside);
    QTest::newRow("CloseOnReleaseOutside|Parent") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside | QQuickPopup::CloseOnReleaseOutsideParent);
    QTest::newRow("CloseOnEscape") << static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnEscape);
}

void tst_popup::closePolicy()
{
    QFETCH(QQuickPopup::ClosePolicy, closePolicy);

    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
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
    if (closePolicy.testFlag(QQuickPopup::CloseOnPressOutside) || closePolicy.testFlag(QQuickPopup::CloseOnPressOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // release outside popup and its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    if (closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutside))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // press outside popup but inside its parent
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x(), button->y()));
    if (closePolicy.testFlag(QQuickPopup::CloseOnPressOutside) && !closePolicy.testFlag(QQuickPopup::CloseOnPressOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // release outside popup but inside its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x(), button->y()));
    if (closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutside) && !closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutsideParent))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());

    // escape
    QTest::keyClick(window, Qt::Key_Escape);
    if (closePolicy.testFlag(QQuickPopup::CloseOnEscape))
        QVERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());
}

void tst_popup::activeFocusOnClose1()
{
    // Test that a popup that never sets focus: true (e.g. ToolTip) doesn't affect
    // the active focus item when it closes.
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClose1.qml"));
    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *focusedPopup = helper.window->property("focusedPopup").value<QQuickPopup*>();
    QVERIFY(focusedPopup);

    QQuickPopup *nonFocusedPopup = helper.window->property("nonFocusedPopup").value<QQuickPopup*>();
    QVERIFY(nonFocusedPopup);

    focusedPopup->open();
    QVERIFY(focusedPopup->isVisible());
    QVERIFY(focusedPopup->hasActiveFocus());

    nonFocusedPopup->open();
    QVERIFY(nonFocusedPopup->isVisible());
    QVERIFY(focusedPopup->hasActiveFocus());

    nonFocusedPopup->close();
    QVERIFY(!nonFocusedPopup->isVisible());
    QVERIFY(focusedPopup->hasActiveFocus());
}

void tst_popup::activeFocusOnClose2()
{
    // Test that a popup that sets focus: true but relinquishes focus (e.g. by
    // calling forceActiveFocus() on another item) before it closes doesn't
    // affect the active focus item when it closes.
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClose2.qml"));
    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup1 = helper.window->property("popup1").value<QQuickPopup*>();
    QVERIFY(popup1);

    QQuickPopup *popup2 = helper.window->property("popup2").value<QQuickPopup*>();
    QVERIFY(popup2);

    QQuickButton *closePopup2Button = helper.window->property("closePopup2Button").value<QQuickButton*>();
    QVERIFY(closePopup2Button);

    popup1->open();
    QVERIFY(popup1->isVisible());
    QVERIFY(popup1->hasActiveFocus());

    popup2->open();
    QVERIFY(popup2->isVisible());
    QVERIFY(popup2->hasActiveFocus());

    // Causes popup1.contentItem.forceActiveFocus() to be called, then closes popup2.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        closePopup2Button->mapToScene(QPointF(closePopup2Button->width() / 2, closePopup2Button->height() / 2)).toPoint());
    QVERIFY(!popup2->isVisible());
    QVERIFY(popup1->hasActiveFocus());
}

QTEST_MAIN(tst_popup)

#include "tst_popup.moc"
