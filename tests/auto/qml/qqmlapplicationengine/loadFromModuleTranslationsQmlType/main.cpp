// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QLocale::setDefault(QLocale(QLocale::Language(qEnvironmentVariableIntValue("qtlang"))));
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
            &app, []() { QCoreApplication::exit(-1); },
            Qt::QueuedConnection);
    engine.loadFromModule("TranslatedQml", "Main");

    QString expected = qgetenv("LOADFROMMODULE_TEST_EXPECTED_OUTPUT");
    auto *root = engine.rootObjects().first();
    root->setProperty("expected", expected);
    root->metaObject()->invokeMethod(root, "f");
    return app.exec();
}
