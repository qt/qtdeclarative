// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [2]
// src_qmltest_qquicktest.cpp
#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty("myContextProperty", QVariant(true));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(mytest, Setup)

#include "src_qmltest_qquicktest.moc"
//! [2]
