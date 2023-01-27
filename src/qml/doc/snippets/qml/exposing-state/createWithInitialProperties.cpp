// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtGui/qguiapplication.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

//![0]
    QQmlEngine engine;

    QQmlComponent component(&engine, "MyModule", "RequiredProperties");
    QScopedPointer<QObject> o(component.createWithInitialProperties({
            {"thing", 11}
    }));
//![0]

    return app.exec();
}
