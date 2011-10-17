/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PARTICLES_TESTS_SHARED
#define PARTICLES_TESTS_SHARED
#include <QSGView>
#include <QtTest>
#include <QAbstractAnimation>
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

QSGView* createView(const QString &filename, int additionalWait=0)
{
    QSGView *canvas = new QSGView(0);

    canvas->setSource(QUrl::fromLocalFile(filename));
    if (canvas->status() != QSGView::Ready)
        return 0;
    canvas->show();
    QTest::qWaitForWindowShown(canvas);
    if (additionalWait)
        QTest::qWait(additionalWait);

    return canvas;
}

void ensureAnimTime(int requiredTime, QAbstractAnimation* anim)//With consistentTiming, who knows how long an animation really takes...
{
    while (anim->currentTime() < requiredTime)
        QTest::qWait(100);
}

#endif
