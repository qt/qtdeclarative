// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlcomponent.h>

#include <QtTest/qbenchmark.h>
#include <QtTest/qtest.h>
#include <QtTest/qtestcase.h>

class DeltaBlue : public QObject
{
    Q_OBJECT
public:

private slots:
    void qml() {
        QBENCHMARK {
            QObject *instance = engine.singletonInstance<QObject *>(
                "DeltaBlue", "Main");
            QMetaObject::invokeMethod(instance, "deltaBlue");
            engine.clearSingletons();
        }
    }

    void nicer() {
        QQmlComponent chain(&engine, "DeltaBlue", "ChainTest");
        QQmlComponent projection(&engine, "DeltaBlue", "ProjectionTest");
        QBENCHMARK {
            QScopedPointer<QObject> chainTest(chain.create());
            QMetaObject::invokeMethod(chainTest.data(), "run");

            QScopedPointer<QObject> projectionTest(projection.create());
            QMetaObject::invokeMethod(projectionTest.data(), "run");
        }
    }

    void onlyCreate() {
        QQmlComponent chain(&engine, "DeltaBlue", "ChainTest");
        QQmlComponent projection(&engine, "DeltaBlue", "ProjectionTest");
        QBENCHMARK {
            QScopedPointer<QObject> chainTest(chain.create());
            QScopedPointer<QObject> projectionTest(projection.create());
        }
    }

    void js() {
        QQmlComponent main2(&engine, "DeltaBlue", "Main2");
        QBENCHMARK {
            QScopedPointer<QObject> instance(main2.create());
            QMetaObject::invokeMethod(instance.data(), "deltaBlue");
        }
    }

    void jsOnlyCreate() {
        QQmlComponent main2(&engine, "DeltaBlue", "Main2");
        QBENCHMARK {
            QScopedPointer<QObject> instance(main2.create());
        }
    }

private:
    QQmlApplicationEngine engine;
};

QTEST_MAIN(DeltaBlue)

#include "main.moc"
