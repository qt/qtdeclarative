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
    engine.loadFromModule("TranslatedCpp", "Main");
    app.exec();

    QString expected = qgetenv("LOADFROMMODULE_TEST_EXPECTED_OUTPUT");
    QString actual = QObject::tr("Hello");

    if (actual == expected)
        return 0;

    return actual[0].toLatin1();
}
