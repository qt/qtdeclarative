// # Copyright (C) 2023 The Qt Company Ltd.
// # SPDX-License-Identifier: BSD-3-Clause

//! [setup]
#ifndef SETUP_H
#define SETUP_H

#include <QObject>
#include <QQmlEngine>

class Setup : public QObject
{
    Q_OBJECT
public:
    Setup() = default;

public slots:
    void applicationAvailable();
    void qmlEngineAvailable(QQmlEngine *engine);
    void cleanupTestCase();
};

#endif // SETUP_H
//! [setup]
