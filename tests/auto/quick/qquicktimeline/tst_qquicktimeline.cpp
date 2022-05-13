// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <private/qquicktimeline_p_p.h>
#include <limits>

class tst_QQuickTimeLine : public QObject
{
    Q_OBJECT

private slots:
    void overflow();
};

void tst_QQuickTimeLine::overflow()
{
    // Test ensures that time value used in QQuickTimeLine::accel methods is always positive.
    // On platforms where casting qreal value infinity to int yields a positive value this is
    // always the case and the test would fail. Strictly speaking, the cast is undefined behavior.
    if (static_cast<int>(qInf()) > 0)
        QSKIP("Test is not applicable on this platform");

    QQuickTimeLine timeline;
    QQuickTimeLineValue value;

    // overflow -> negative time -> assertion failure (QTBUG-35046)
    QCOMPARE(timeline.accel(value, std::numeric_limits<qreal>::max(), 0.0001f), -1);
    QCOMPARE(timeline.accel(value, std::numeric_limits<qreal>::max(), 0.0001f, 0.0001f), -1);
    QCOMPARE(timeline.accelDistance(value, 0.0001f, std::numeric_limits<qreal>::max()), -1);
}

QTEST_MAIN(tst_QQuickTimeLine)

#include "tst_qquicktimeline.moc"
