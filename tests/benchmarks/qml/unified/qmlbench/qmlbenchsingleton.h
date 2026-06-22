// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMLBENCHSINGLETON_H
#define QMLBENCHSINGLETON_H

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtQml/qqmlregistration.h>

// Reproduces the QmlBench singleton from the qmlbench tool: a deterministic
// pseudo-random source so benchmarks that scatter items around with
// QmlBench.getRandom() are reproducible from run to run.
class QmlBench : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QmlBench(QObject *parent = nullptr);

    Q_INVOKABLE qreal getRandom();

private:
    QList<qreal> m_pregeneratedRandomNumbers;
    int m_currentRandomNumberIndex = 0;
};

#endif // QMLBENCHSINGLETON_H
