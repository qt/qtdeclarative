/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "../shared/visualtestutil.h"

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickscrollbar_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>

using namespace QQuickVisualTestUtil;

class tst_scrollbar : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void cursorShape();
};

// QTBUG-59629
void tst_scrollbar::cursorShape()
{
    // Ensure that the mouse cursor has the correct shape when over a scrollbar
    // which is itself over a text area with IBeamCursor.
    QQuickApplicationHelper helper(this, QStringLiteral("cursor.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickScrollBar *scrollBar = helper.appWindow->property("scrollBar").value<QQuickScrollBar*>();
    QVERIFY(scrollBar);

    QQuickTextArea *textArea = helper.appWindow->property("textArea").value<QQuickTextArea*>();
    QVERIFY(textArea);

    textArea->setText(QString("\n").repeated(100));

    const QPoint textAreaPos(window->width() / 2, window->height() / 2);
    QTest::mouseMove(window, textAreaPos);
    QCOMPARE(window->cursor().shape(), textArea->cursor().shape());

    const QPoint scrollBarPos(window->width() - scrollBar->width() / 2, window->height() / 2);
    QTest::mouseMove(window, scrollBarPos);
    QVERIFY(scrollBar->isActive());
    QCOMPARE(window->cursor().shape(), scrollBar->cursor().shape());
    QCOMPARE(scrollBar->cursor().shape(), Qt::CursorShape::ArrowCursor);
}

QTEST_MAIN(tst_scrollbar)

#include "tst_scrollbar.moc"
