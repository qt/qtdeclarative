// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (!app.arguments().isEmpty()) {
        QQuickWindow *win = static_cast<QQuickWindow *>(engine.rootObjects().first());
        auto lastArg = app.arguments().last();
        if (lastArg.endsWith(QLatin1String(".qml"))) {
            auto root = win->findChild<QQuickItem *>("LauncherList");
            int showExampleIdx = -1;
            for (int i = root->metaObject()->methodCount(); showExampleIdx < 0 && i >= 0; --i)
                if (root->metaObject()->method(i).name() == QByteArray("showExample"))
                    showExampleIdx = i;
            QMetaMethod showExampleFn = root->metaObject()->method(showExampleIdx);
            showExampleFn.invoke(root, Q_ARG(QVariant, QVariant(QLatin1String("../../") + lastArg)));
        }
    }

    return app.exec();
}
