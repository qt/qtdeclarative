// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlbenchsingleton.h"

#include <QtCore/qrandom.h>

static constexpr int PregeneratedCount = 10001;

QmlBench::QmlBench(QObject *parent)
    : QObject(parent)
{
    m_pregeneratedRandomNumbers.reserve(PregeneratedCount);
    QRandomGenerator *randomGenerator = QRandomGenerator::global();
    for (int i = 0; i < PregeneratedCount; ++i)
        m_pregeneratedRandomNumbers.append(randomGenerator->generateDouble());
}

qreal QmlBench::getRandom()
{
    return m_pregeneratedRandomNumbers.at((m_currentRandomNumberIndex++) % PregeneratedCount);
}
