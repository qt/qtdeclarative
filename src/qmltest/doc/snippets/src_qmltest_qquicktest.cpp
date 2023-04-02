// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [2]
// src_qmltest_qquicktest.cpp
#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public slots:
    void applicationAvailable()
    {
        // Initialization that only requires the QGuiApplication object to be available
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        // Initialization requiring the QQmlEngine to be constructed
        engine->rootContext()->setContextProperty("myContextProperty", QVariant(true));
    }

    void cleanupTestCase()
    {
        // Implement custom resource cleanup
    }
};

QUICK_TEST_MAIN_WITH_SETUP(mytest, Setup)

#include "src_qmltest_qquicktest.moc"
//! [2]
