// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PARTICLES_TESTS_SHARED
#define PARTICLES_TESTS_SHARED
#include <QtQuick/QQuickView>
#include <QtTest>
#include <QAbstractAnimation>
#include <QScopedPointer>

const qreal EPSILON = 0.0001;

bool extremelyFuzzyCompare(qreal a, qreal b, qreal e)//For cases which can have larger variances
{
    return (a + e >= b) && (a - e <= b);
}

bool myFuzzyCompare(qreal a, qreal b)//For cases which might be near 0 so qFuzzyCompare fails
{
    return (a + EPSILON > b) && (a - EPSILON < b);
}

bool myFuzzyLEQ(qreal a, qreal b)
{
    return (a - EPSILON < b);
}

bool myFuzzyGEQ(qreal a, qreal b)
{
    return (a + EPSILON > b);
}

QQuickView* createView(const QUrl &filename, int additionalWait=0)
{
    std::unique_ptr<QQuickView> view(new QQuickView(nullptr));

    view->setSource(filename);
    if (view->status() != QQuickView::Ready)
        return nullptr;
    view->show();
    if (!QTest::qWaitForWindowExposed(view.get()))
        return nullptr;
    if (additionalWait)
        QTest::qWait(additionalWait);

    return view.release();
}

void ensureAnimTime(int requiredTime, QAbstractAnimation* anim)//With consistentTiming, who knows how long an animation really takes...
{
    while (anim->currentTime() < requiredTime)
        QTest::qWait(100);
}

#endif
