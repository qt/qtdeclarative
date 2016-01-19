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

#include <qtest.h>

#include <QtQuick>
#include <private/qquickanimator_p.h>

#include <QtQml>

class tst_Animators: public QObject
{
    Q_OBJECT

private slots:
    void testMultiWinAnimator_data();
    void testMultiWinAnimator();
};

void tst_Animators::testMultiWinAnimator_data()
{
    QTest::addColumn<int>("count");

    QTest::newRow("1") << 1;
    QTest::newRow("10") << 10;
}

void tst_Animators::testMultiWinAnimator()
{
    QFETCH(int, count);

    QQmlEngine engine;
    QQmlComponent component(&engine, "data/windowWithAnimator.qml");

    QList<QQuickWindow *> windows;
    for (int i=0; i<count; ++i) {
        QQuickWindow *win = qobject_cast<QQuickWindow *>(component.create());
        windows << win;

        // As the windows are all the same size, if they are positioned at the
        // same place only the top-most one will strictly be "exposed" and rendering
        // for all the others will be disabled. Move the windows a little bit
        // to ensure they are exposed and actually rendering.
        if (i > 0) {
            QPoint pos = win->position();
            if (pos == windows.first()->position())
                pos += QPoint(10 * i, 10 * i);
                win->setPosition(pos);
        }
    }

    // let all animations run their course...
    while (true) {
        QTest::qWait(200);
        bool allDone = true;
        for (int i=0; i<count; ++i) {
            QQuickWindow *win = windows.at(i);
            allDone = win->isExposed() && win->property("animationDone").toBool();
        }

        if (allDone) {
            for (int i=0; i<count; ++i) {
                QQuickWindow *win = windows.at(i);
                delete win;
            }
            break;
        }
    }
    QVERIFY(true);
}

#include "tst_qquickanimators.moc"

QTEST_MAIN(tst_Animators)

