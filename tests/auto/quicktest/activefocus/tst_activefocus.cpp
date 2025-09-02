// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qregularexpression.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace QQuickVisualTestUtils;

class tst_ActiveFocus : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_ActiveFocus();

private slots:
    void failureMessage();
};

tst_ActiveFocus::tst_ActiveFocus()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_ActiveFocus::failureMessage()
{
    QQuickApplicationHelper helper(this, "activeFocus.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    auto *item = window->property("item").value<QQuickItem*>();
    QVERIFY(item);

    const QByteArray failureMessageWithWindow = QQuickTest::Private::qActiveFocusFailureMessage(item);
    QVERIFY2(QRegularExpression("item: .* focusPolicy: .* window's activeFocusItem: .*")
        .match(failureMessageWithWindow).hasMatch(), ("Failure message: " + failureMessageWithWindow).constData());

    item->setParentItem(nullptr);
    const QByteArray failureMessageWithoutWindow = QQuickTest::Private::qActiveFocusFailureMessage(item);
    QVERIFY2(QRegularExpression("item: .* focusPolicy: .* window's activeFocusItem: \\(unknown; item has no window\\)")
        .match(failureMessageWithoutWindow).hasMatch(), ("Failure message: " + failureMessageWithoutWindow).constData());
}

QTEST_MAIN(tst_ActiveFocus)

#include "tst_activefocus.moc"
